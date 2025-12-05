#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Script to run all test scripts in subdirectories and generate JUnit XML report
#
# Usage:
#   1. Source Zephyr environment:
#      source zephyr/zephyr-env.sh
#
#   2. Run the script:
#      ./ci.bt.classic.sh
#
# Environment Variables:
#   ZEPHYR_BASE - Path to Zephyr root directory (required)
#
# Output:
#   - Test logs: test_logs/*.log
#   - Summary: test_logs/test_summary.log
#   - JUnit XML: test_logs/junit.xml

set -euo pipefail

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1" >&2
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1" >&2
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_section() {
    echo -e "${BLUE}[====]${NC} $1" >&2
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Test directories configuration file
TEST_DIRS_FILE="$SCRIPT_DIR/tests.native_sim.txt"

# Create logs directory
LOGS_DIR="$SCRIPT_DIR/test_logs"
mkdir -p "$LOGS_DIR"

# Log file for summary
SUMMARY_LOG="$LOGS_DIR/test_summary.log"
> "$SUMMARY_LOG"  # Clear previous summary

# JUnit XML file
JUNIT_XML="$LOGS_DIR/junit.xml"

# Arrays to track test results
declare -a PASSED_TESTS=()
declare -a FAILED_TESTS=()
declare -a SKIPPED_TESTS=()
declare -A TEST_TIMES=()
declare -A TEST_EXIT_CODES=()
declare -A TEST_MESSAGES=()

# Global variable to track if any test failed
HAS_FAILURE=false

# Function to escape XML special characters
xml_escape() {
    local text="$1"
    text="${text//&/&amp;}"
    text="${text//</&lt;}"
    text="${text//>/&gt;}"
    text="${text//\"/&quot;}"
    text="${text//\'/&apos;}"
    echo "$text"
}

# Global arrays for test directories and scripts
declare -a TEST_DIRECTORIES=()
declare -a TEST_SCRIPTS=()

# Function to read test directories from file
read_test_directories() {
    TEST_DIRECTORIES=()

    if [[ ! -f "$TEST_DIRS_FILE" ]]; then
        log_error "Test directories file not found: $TEST_DIRS_FILE"
        return 1
    fi

    log_info "Reading test directories from: $TEST_DIRS_FILE"

    # Read file line by line, skip comments and empty lines
    while IFS= read -r line || [[ -n "$line" ]]; do
        # Skip empty lines
        [[ -z "$line" ]] && continue

        # Skip comments (lines starting with #)
        [[ "$line" =~ ^[[:space:]]*# ]] && continue

        # Trim whitespace
        line=$(echo "$line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')

        # Skip if empty after trimming
        [[ -z "$line" ]] && continue

        # Convert relative path to absolute path
        local abs_path
        if [[ "$line" = /* ]]; then
            abs_path="$line"
        else
            abs_path="$SCRIPT_DIR/$line"
        fi

        # Check if directory exists
        if [[ -d "$abs_path" ]]; then
            TEST_DIRECTORIES+=("$abs_path")
            log_info "  Added directory: $abs_path"
        else
            log_warn "  Directory not found (skipping): $abs_path"
        fi
    done < "$TEST_DIRS_FILE"

    if [[ ${#TEST_DIRECTORIES[@]} -eq 0 ]]; then
        log_error "No valid directories found in $TEST_DIRS_FILE"
        return 1
    fi

    log_info "Total directories to search: ${#TEST_DIRECTORIES[@]}"
    return 0
}

# Function to find all test scripts
find_test_scripts() {
    TEST_SCRIPTS=()

    log_info "Searching for test scripts in configured directories..."

    if [[ ${#TEST_DIRECTORIES[@]} -eq 0 ]]; then
        log_error "No test directories configured"
        return 1
    fi

    # Search for test scripts in each directory
    for test_dir in "${TEST_DIRECTORIES[@]}"; do
        log_info "Searching in: $test_dir"

        # Find all .sh files in tests_scripts subdirectories
        while IFS= read -r -d '' script; do
            # Check if file is executable
            if [[ -x "$script" ]]; then
                TEST_SCRIPTS+=("$script")
                log_info "  Found executable: $(basename "$script")"
            else
                log_warn "  Found non-executable: $(basename "$script")"

                # Try to make it executable
                if chmod +x "$script" 2>/dev/null; then
                    log_info "  Made executable: $(basename "$script")"
                    TEST_SCRIPTS+=("$script")
                else
                    log_error "  Failed to make executable: $(basename "$script")"
                    local script_name=$(basename "$script")
                    local test_name=$(basename "$(dirname "$(dirname "$script")")")
                    SKIPPED_TESTS+=("$test_name/$script_name")
                    TEST_MESSAGES["$test_name/$script_name"]="Not executable (chmod failed)"
                fi
            fi
        done < <(find "$test_dir" -type d -name "tests_scripts" \
            -exec find {} -maxdepth 1 -type f -name "*.sh" -print0 \;)
    done

    # Sort scripts for consistent execution order
    if [[ ${#TEST_SCRIPTS[@]} -gt 0 ]]; then
        IFS=$'\n' TEST_SCRIPTS=($(sort <<<"${TEST_SCRIPTS[*]}"))
        unset IFS
    fi

    return 0
}

# Function to run a single test script
run_test_script() {
    local script="$1"
    local script_name=$(basename "$script")
    local script_dir=$(dirname "$script")
    local test_name=$(basename "$(dirname "$script_dir")")
    local test_id="$test_name/$script_name"
    local log_file="$LOGS_DIR/${test_id//\//_}.log"

    log_section "Running test: $test_id"
    log_info "Script path: $script"
    log_info "Log file: $log_file"

    # Check if ZEPHYR_BASE is set
    if [[ -z "${ZEPHYR_BASE:-}" ]]; then
        log_error "ZEPHYR_BASE environment variable is not set"
        log_error "Please source zephyr-env.sh or set ZEPHYR_BASE manually"
        FAILED_TESTS+=("$test_id")
        TEST_MESSAGES["$test_id"]="ZEPHYR_BASE not set"
        TEST_TIMES["$test_id"]=0
        TEST_EXIT_CODES["$test_id"]=1
        HAS_FAILURE=true
        return 1
    fi

    # Check if ZEPHYR_BASE directory exists
    if [[ ! -d "$ZEPHYR_BASE" ]]; then
        log_error "ZEPHYR_BASE directory does not exist: $ZEPHYR_BASE"
        FAILED_TESTS+=("$test_id")
        TEST_MESSAGES["$test_id"]="ZEPHYR_BASE directory not found: $ZEPHYR_BASE"
        TEST_TIMES["$test_id"]=0
        TEST_EXIT_CODES["$test_id"]=1
        HAS_FAILURE=true
        return 1
    fi

    log_info "ZEPHYR_BASE: $ZEPHYR_BASE"

    # Save current directory
    local original_dir=$(pwd)
    log_info "Current directory: $original_dir"
    log_info "Execution directory: $ZEPHYR_BASE"

    # Record start time
    local start_time=$(date +%s)

    # Run the script and capture output
    # Use 'set +e' to continue even if script fails
    set +e
    (
        # Change to ZEPHYR_BASE directory
        if ! cd "$ZEPHYR_BASE"; then
            echo "ERROR: Failed to change to ZEPHYR_BASE: $ZEPHYR_BASE" >&2
            exit 1
        fi

        log_info "Executing script from: $(pwd)"

        # Execute the script
        "$script" 2>&1 | tee "$log_file"
        exit "${PIPESTATUS[0]}"
    )
    local exit_code=$?
    set -e

    # Return to original directory
    if ! cd "$original_dir"; then
        log_error "Failed to return to original directory: $original_dir"
        log_warn "Current directory: $(pwd)"
    else
        log_info "Returned to directory: $(pwd)"
    fi

    # Record end time
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # Store test time
    TEST_TIMES["$test_id"]=$duration
    TEST_EXIT_CODES["$test_id"]=$exit_code

    # Check result
    if [[ $exit_code -eq 0 ]]; then
        log_info "✅ PASSED: $test_id (${duration}s)"
        PASSED_TESTS+=("$test_id")
        TEST_MESSAGES["$test_id"]="Test passed successfully"
        echo "PASSED: $test_id (${duration}s)" >> "$SUMMARY_LOG"
        return 0
    else
        log_error "❌ FAILED: $test_id (exit code: $exit_code, ${duration}s)"
        FAILED_TESTS+=("$test_id")
        TEST_MESSAGES["$test_id"]="Test failed with exit code $exit_code"
        echo "FAILED: $test_id (exit code: $exit_code, ${duration}s)" \
            >> "$SUMMARY_LOG"
        HAS_FAILURE=true
        # Don't return error - continue with other tests
        return 0
    fi
}

# Function to generate JUnit XML report
generate_junit_xml() {
    log_info "Generating JUnit XML report..."

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%S")
    local total_tests=$((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]}))
    local total_time=0

    # Calculate total time
    for test_id in "${!TEST_TIMES[@]}"; do
        total_time=$((total_time + TEST_TIMES["$test_id"]))
    done

    # Start XML document
    cat > "$JUNIT_XML" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="Bluetooth Classic Simulation Tests"
            tests="$total_tests"
            failures="${#FAILED_TESTS[@]}"
            errors="0"
            skipped="${#SKIPPED_TESTS[@]}"
            time="$total_time"
            timestamp="$timestamp">
  <testsuite name="Bluetooth Classic Tests"
             tests="$total_tests"
             failures="${#FAILED_TESTS[@]}"
             errors="0"
             skipped="${#SKIPPED_TESTS[@]}"
             time="$total_time"
             timestamp="$timestamp"
             hostname="$(hostname)">
EOF

    # Add passed tests
    for test_id in "${PASSED_TESTS[@]}"; do
        local time="${TEST_TIMES[$test_id]}"
        local classname=$(echo "$test_id" | cut -d'/' -f1)
        local testname=$(echo "$test_id" | cut -d'/' -f2)

        cat >> "$JUNIT_XML" <<EOF
    <testcase name="$(xml_escape "$testname")" classname="$(xml_escape "$classname")" time="$time">
      <system-out>Test passed successfully</system-out>
    </testcase>
EOF
    done

    # Add failed tests
    for test_id in "${FAILED_TESTS[@]}"; do
        local time="${TEST_TIMES[$test_id]}"
        local exit_code="${TEST_EXIT_CODES[$test_id]}"
        local message="${TEST_MESSAGES[$test_id]}"
        local classname=$(echo "$test_id" | cut -d'/' -f1)
        local testname=$(echo "$test_id" | cut -d'/' -f2)
        local log_file="$LOGS_DIR/${test_id//\//_}.log"

        # Read last 50 lines of log for failure details
        local log_content=""
        if [[ -f "$log_file" ]]; then
            log_content=$(cat "$log_file" 2>/dev/null || echo "Log file not found")
        fi

        cat >> "$JUNIT_XML" <<EOF
    <testcase name="$(xml_escape "$testname")" classname="$(xml_escape "$classname")" time="$time">
      <failure message="$(xml_escape "$message")" type="TestFailure">
Exit Code: $exit_code
Test: $test_id
Log File: $log_file

Complete log output:
$(xml_escape "$log_content")
      </failure>
      <system-err>Test failed with exit code $exit_code</system-err>
    </testcase>
EOF
    done

    # Add skipped tests
    for test_id in "${SKIPPED_TESTS[@]}"; do
        local message="${TEST_MESSAGES[$test_id]}"
        local classname=$(echo "$test_id" | cut -d'/' -f1)
        local testname=$(echo "$test_id" | cut -d'/' -f2)

        cat >> "$JUNIT_XML" <<EOF
    <testcase name="$(xml_escape "$testname")" classname="$(xml_escape "$classname")" time="0">
      <skipped message="$(xml_escape "$message")"/>
    </testcase>
EOF
    done

    # Close XML document
    cat >> "$JUNIT_XML" <<EOF
  </testsuite>
</testsuites>
EOF

    log_info "JUnit XML report generated: $JUNIT_XML"
}

# Function to print summary
print_summary() {
    local total_tests=$((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]}))

    echo ""
    log_section "Test Summary"
    echo "============================================"
    echo "Total tests run: $total_tests"
    echo "Passed: ${#PASSED_TESTS[@]}"
    echo "Failed: ${#FAILED_TESTS[@]}"
    echo "Skipped: ${#SKIPPED_TESTS[@]}"
    echo "============================================"

    if [[ ${#PASSED_TESTS[@]} -gt 0 ]]; then
        echo ""
        echo -e "${GREEN}Passed tests:${NC}"
        for test in "${PASSED_TESTS[@]}"; do
            echo "  ✅ $test (${TEST_TIMES[$test]}s)"
        done
    fi

    if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
        echo ""
        echo -e "${RED}Failed tests:${NC}"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  ❌ $test (exit code: ${TEST_EXIT_CODES[$test]}, \
${TEST_TIMES[$test]}s)"
        done
    fi

    if [[ ${#SKIPPED_TESTS[@]} -gt 0 ]]; then
        echo ""
        echo -e "${YELLOW}Skipped tests:${NC}"
        for test in "${SKIPPED_TESTS[@]}"; do
            echo "  ⚠️  $test - ${TEST_MESSAGES[$test]}"
        done
    fi

    echo ""
    echo "Detailed logs available in: $LOGS_DIR"
    echo "Summary log: $SUMMARY_LOG"
    echo "JUnit XML report: $JUNIT_XML"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up..."

    # Kill any remaining processes (try with and without sudo)
    pkill -9 btvirt 2>/dev/null || sudo pkill -9 btvirt 2>/dev/null || true
    pkill -9 btmon 2>/dev/null || sudo pkill -9 btmon 2>/dev/null || true
    pkill -9 zephyr.exe 2>/dev/null || sudo pkill -9 zephyr.exe 2>/dev/null || true

    # Generate JUnit XML report
    generate_junit_xml

    # Print summary
    print_summary

    # Return appropriate exit code
    if [[ "$HAS_FAILURE" == true ]]; then
        log_error "Some tests failed - exiting with error code 1"
        exit 1
    else
        log_info "All tests passed - exiting with code 0"
        exit 0
    fi
}

# Set cleanup on exit
trap cleanup EXIT

# Main function
main() {
    log_section "Bluetooth Classic Simulation Test Runner"
    log_info "Script directory: $SCRIPT_DIR"
    log_info "Test directories file: $TEST_DIRS_FILE"
    log_info "Logs directory: $LOGS_DIR"

    # Check ZEPHYR_BASE early
    if [[ -z "${ZEPHYR_BASE:-}" ]]; then
        log_error "ZEPHYR_BASE environment variable is not set"
        log_error "Please run: source zephyr/zephyr-env.sh"
        HAS_FAILURE=true
        return 1
    fi

    if [[ ! -d "$ZEPHYR_BASE" ]]; then
        log_error "ZEPHYR_BASE directory does not exist: $ZEPHYR_BASE"
        HAS_FAILURE=true
        return 1
    fi

    log_info "ZEPHYR_BASE: $ZEPHYR_BASE"

    if ! read_test_directories; then
        log_error "Failed to read test directories"
        HAS_FAILURE=true
        return 1
    fi

    if ! find_test_scripts; then
        log_error "Failed to find test scripts"
        HAS_FAILURE=true
        return 1
    fi

    if [[ ${#TEST_SCRIPTS[@]} -eq 0 ]]; then
        log_warn "No executable test scripts found in configured directories"

        if [[ ${#SKIPPED_TESTS[@]} -gt 0 ]]; then
            HAS_FAILURE=true
        fi

        return 0
    fi

    log_info "Found ${#TEST_SCRIPTS[@]} executable test script(s)"
    echo ""

    for script in "${TEST_SCRIPTS[@]}"; do
        run_test_script "$script" || true
        echo ""
    done

    # Summary will be printed in cleanup function
    # Exit code will be set based on HAS_FAILURE
}

# Run main function
main "$@"
