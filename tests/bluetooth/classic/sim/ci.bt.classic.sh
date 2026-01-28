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
#   MAX_PARALLEL_JOBS - Maximum number of parallel jobs (default: number of CPU cores)
#
# Output:
#   - Test logs: ${ZEPHYR_BASE}/bt_classic_sim/*.log
#   - Summary: ${ZEPHYR_BASE}/bt_classic_sim/test_summary.log
#   - JUnit XML: ${ZEPHYR_BASE}/bt_classic_sim/junit.xml

set -euo pipefail

# Record script start time in milliseconds
SCRIPT_START_TIME=$(date +%s%3N)

# Add this line to record the start timestamp in ISO format
SCRIPT_START_TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%S")

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="$SCRIPT_DIR/common"

# Source common utilities
source "$COMMON_DIR/test_utils.sh"

# Test directories configuration file
TEST_DIRS_FILE="$SCRIPT_DIR/tests.native_sim.txt"

# Create logs directory
LOGS_DIR="${ZEPHYR_BASE}/bt_classic_sim"

# Clear existing logs directory if it exists
if [[ -d "$LOGS_DIR" ]]; then
    log_info "Clearing existing logs directory: $LOGS_DIR"
    rm -rf "$LOGS_DIR"
fi

mkdir -p "$LOGS_DIR"

# Log file for summary
SUMMARY_LOG="$LOGS_DIR/test_summary.log"
> "$SUMMARY_LOG"  # Clear previous summary

# JUnit XML file
JUNIT_XML="$LOGS_DIR/junit.xml"

# Temporary files for parallel execution
TMP_RESULTS_DIR="$LOGS_DIR/tmp_results"
mkdir -p "$TMP_RESULTS_DIR"

# Arrays to track test results
declare -a PASSED_TESTS=()
declare -a FAILED_TESTS=()
declare -a SKIPPED_TESTS=()
declare -A TEST_TIMES=()
declare -A TEST_EXIT_CODES=()
declare -A TEST_MESSAGES=()

# Global variable to track if any test failed
HAS_FAILURE=false

# Global arrays for test directories and scripts
declare -a TEST_DIRECTORIES=()
declare -a TEST_SCRIPTS=()

# Maximum parallel jobs (default to number of CPU cores)
MAX_PARALLEL_JOBS="${MAX_PARALLEL_JOBS:-$(nproc 2>/dev/null || echo 4)}"

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
            log_err "  Directory not found (skipping): $abs_path"
            return 1
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
                local script_name=$(basename "$script")
                local test_name=$(basename "$(dirname "$(dirname "$script")")")
                SKIPPED_TESTS+=("$test_name/$script_name")
                TEST_MESSAGES["$test_name/$script_name"]="Not executable"
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

# Function to run a single test script (used by parallel)
run_test_script_parallel() {
    local script="$1"
    local test_index="$2"
    local total_tests="$3"
    local script_name=$(basename "$script")
    local script_dir=$(dirname "$script")
    local test_name=$(basename "$(dirname "$script_dir")")
    local test_id="$test_name/$script_name"
    local log_file="$LOGS_DIR/${test_id//\//_}.log"
    local result_file="$TMP_RESULTS_DIR/${test_id//\//_}.result"

    # Record start time
    local start_time=$(date +%s%3N)

    echo "[$test_index/$total_tests] Running: $test_id" >&2

    # Run the script and capture output
    set +e
    (
        # Change to ZEPHYR_BASE directory
        if ! cd "$ZEPHYR_BASE"; then
            echo "ERROR: Failed to change to ZEPHYR_BASE: $ZEPHYR_BASE" >&2
            exit 1
        fi

        # Execute the script and redirect all output to log file
        "$script" > "$log_file" 2>&1
        exit $?
    )
    local exit_code=$?
    set -e

    # Record end time
    local end_time=$(date +%s%3N)
    local duration=$((end_time - start_time))

    # Save result to file for later processing
    echo "test_id=$test_id" > "$result_file"
    echo "exit_code=$exit_code" >> "$result_file"
    echo "duration=$duration" >> "$result_file"
    echo "log_file=$log_file" >> "$result_file"

    # Print result to stderr
    if [[ $exit_code -eq 0 ]]; then
        echo -e "[$test_index/$total_tests] $test_id \e[32mPASS\e[0m (${duration}ms)" >&2
    else
        # Create complete failure message as a single string
        local failure_msg
        failure_msg=$(cat <<EOF
[$test_index/$total_tests] $test_id \e[31mFAIL\e[0m (${duration}ms)
Last 50 lines of log:
$(tail -n 50 "$log_file" 2>/dev/null || echo "Failed to read log file: $log_file")
EOF
)
        echo -e "$failure_msg" >&2
    fi

    return $exit_code
}

# Export function and variables for parallel
export -f run_test_script_parallel
export LOGS_DIR
export TMP_RESULTS_DIR
export ZEPHYR_BASE

# Function to collect results from parallel execution
collect_parallel_results() {
    log_info "Collecting test results..."

    # Process all result files
    for result_file in "$TMP_RESULTS_DIR"/*.result; do
        [[ -f "$result_file" ]] || continue

        # Source the result file to get variables
        local test_id exit_code duration log_file
        source "$result_file"

        # Store results
        TEST_TIMES["$test_id"]=$duration
        TEST_EXIT_CODES["$test_id"]=$exit_code

        if [[ $exit_code -eq 0 ]]; then
            PASSED_TESTS+=("$test_id")
            TEST_MESSAGES["$test_id"]="Test passed successfully"
            echo "PASSED: $test_id (${duration}ms)" >> "$SUMMARY_LOG"
        else
            FAILED_TESTS+=("$test_id")
            TEST_MESSAGES["$test_id"]="Test failed with exit code $exit_code"
            echo "FAILED: $test_id (exit code: $exit_code, ${duration}ms)" >> "$SUMMARY_LOG"
            HAS_FAILURE=true
        fi
    done

    # Clean up temporary results
    rm -rf "$TMP_RESULTS_DIR"
}

# Function to run tests in parallel using GNU parallel
run_tests_parallel() {
    local total_tests=${#TEST_SCRIPTS[@]}

    log_info "Running $total_tests tests in parallel (max $MAX_PARALLEL_JOBS jobs)..."

    # Check if GNU parallel is available
    if ! command -v parallel &> /dev/null; then
        log_warn "GNU parallel not found, falling back to sequential execution"
        return 1
    fi

    # Create array of test indices
    local indices=()
    for i in $(seq 1 $total_tests); do
        indices+=("$i")
    done

    # Run tests in parallel
    set +e
    parallel --halt soon,fail=1 -j "$MAX_PARALLEL_JOBS" --line-buffer \
        run_test_script_parallel {1} {2} $total_tests \
        ::: "${TEST_SCRIPTS[@]}" \
        :::+ "${indices[@]}"
    local parallel_exit=$?
    set -e

    # Collect results
    collect_parallel_results

    return 0
}

# Function to run tests sequentially (fallback)
run_tests_sequential() {
    local total_tests=${#TEST_SCRIPTS[@]}
    local test_index=1

    log_info "Running $total_tests tests sequentially..."

    for script in "${TEST_SCRIPTS[@]}"; do
        local script_name=$(basename "$script")
        local script_dir=$(dirname "$script")
        local test_name=$(basename "$(dirname "$script_dir")")
        local test_id="$test_name/$script_name"
        local log_file="$LOGS_DIR/${test_id//\//_}.log"

        # Record start time
        local start_time=$(date +%s%3N)

        log_info "[$test_index/$total_tests] Running: $test_id"

        # Run the script and capture output
        set +e
        (
            if ! cd "$ZEPHYR_BASE"; then
                echo "ERROR: Failed to change to ZEPHYR_BASE: $ZEPHYR_BASE" >&2
                exit 1
            fi

            "$script" > "$log_file" 2>&1
            exit $?
        )
        local exit_code=$?
        set -e

        # Record end time
        local end_time=$(date +%s%3N)
        local duration=$((end_time - start_time))

        # Store test time
        TEST_TIMES["$test_id"]=$duration
        TEST_EXIT_CODES["$test_id"]=$exit_code

        # Check result
        if [[ $exit_code -eq 0 ]]; then
            log_info "[$test_index/$total_tests] $test_id ${GREEN}PASS${NC} (${duration}ms)"
            PASSED_TESTS+=("$test_id")
            TEST_MESSAGES["$test_id"]="Test passed successfully"
            echo "PASSED: $test_id (${duration}ms)" >> "$SUMMARY_LOG"
        else
            log_error "[$test_index/$total_tests] $test_id ${RED}FAIL${NC} (${duration}ms)"
            log_info "Last 50 lines of log:"
            tail -n 50 "$log_file" 2>/dev/null || echo "Failed to read log file: $log_file"
            FAILED_TESTS+=("$test_id")
            TEST_MESSAGES["$test_id"]="Test failed with exit code $exit_code"
            echo "FAILED: $test_id (exit code: $exit_code, ${duration}ms)" >> "$SUMMARY_LOG"
            HAS_FAILURE=true
        fi

        ((test_index++))
    done
}

# Function to format time duration in milliseconds
format_duration_ms() {
    local total_ms=$1
    local seconds=$((total_ms / 1000))
    local ms=$((total_ms % 1000))
    local minutes=$((seconds / 60))
    local sec=$((seconds % 60))
    local hours=$((minutes / 60))
    local min=$((minutes % 60))

    if [[ $hours -gt 0 ]]; then
        printf "%dh %dm %d.%03ds (%dms)" $hours $min $sec $ms $total_ms
    elif [[ $minutes -gt 0 ]]; then
        printf "%dm %d.%03ds (%dms)" $minutes $sec $ms $total_ms
    elif [[ $seconds -gt 0 ]]; then
        printf "%d.%03ds (%dms)" $sec $ms $total_ms
    else
        printf "%dms" $total_ms
    fi
}

# Function to generate JUnit XML report
generate_junit_xml() {
    log_info "Generating JUnit XML report..."

    local total_elapsed_ms=$1
    local timestamp="$SCRIPT_START_TIMESTAMP"
    local total_tests=$((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]}))
    # Convert milliseconds to seconds (with 3 decimal places)
    local total_time_sec=$(awk "BEGIN {printf \"%.3f\", $total_elapsed_ms/1000}")

    # Start XML document
    cat > "$JUNIT_XML" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="Bluetooth Classic Simulation Tests"
            tests="$total_tests"
            failures="${#FAILED_TESTS[@]}"
            errors="0"
            skipped="${#SKIPPED_TESTS[@]}"
            time="$total_time_sec"
            timestamp="$timestamp">
  <testsuite name="Bluetooth Classic Tests"
             tests="$total_tests"
             failures="${#FAILED_TESTS[@]}"
             errors="0"
             skipped="${#SKIPPED_TESTS[@]}"
             time="$total_time_sec"
             timestamp="$timestamp"
             hostname="$(hostname)">
EOF

    # Add passed tests
    for test_id in "${PASSED_TESTS[@]}"; do
        local time_ms="${TEST_TIMES[$test_id]}"
        local time_sec=$(awk "BEGIN {printf \"%.3f\", $time_ms/1000}")
        local classname=$(echo "$test_id" | cut -d'/' -f1)
        local testname=$(echo "$test_id" | cut -d'/' -f2)

        cat >> "$JUNIT_XML" <<EOF
    <testcase name="$(xml_escape "$testname")" \
classname="$(xml_escape "$classname")" time="$time_sec">
      <system-out>Test passed successfully</system-out>
    </testcase>
EOF
    done

    # Add failed tests
    for test_id in "${FAILED_TESTS[@]}"; do
        local time_ms="${TEST_TIMES[$test_id]}"
        local time_sec=$(awk "BEGIN {printf \"%.3f\", $time_ms/1000}")
        local exit_code="${TEST_EXIT_CODES[$test_id]}"
        local message="${TEST_MESSAGES[$test_id]}"
        local classname=$(echo "$test_id" | cut -d'/' -f1)
        local testname=$(echo "$test_id" | cut -d'/' -f2)
        local log_file="$LOGS_DIR/${test_id//\//_}.log"

        # Read log for failure details
        local log_content=""
        if [[ -f "$log_file" ]]; then
            log_content=$(cat "$log_file" 2>/dev/null || echo "Log file not found")
        fi

        cat >> "$JUNIT_XML" <<EOF
    <testcase name="$(xml_escape "$testname")" \
classname="$(xml_escape "$classname")" time="$time_sec">
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
    local total_elapsed_ms=$1
    local total_tests=$((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]}))

    # Calculate total elapsed time in milliseconds
    local formatted_time=$(format_duration_ms $total_elapsed_ms)

    echo ""
    log_section "Test Summary"
    echo "============================================"
    echo "Total tests run: $total_tests"
    echo "Passed: ${#PASSED_TESTS[@]}"
    echo "Failed: ${#FAILED_TESTS[@]}"
    echo "Skipped: ${#SKIPPED_TESTS[@]}"
    echo "Total time: $formatted_time"
    echo "Max parallel jobs: $MAX_PARALLEL_JOBS"
    echo "============================================"

    if [[ ${#PASSED_TESTS[@]} -gt 0 ]]; then
        echo ""
        echo -e "${GREEN}Passed tests:${NC}"
        for test in "${PASSED_TESTS[@]}"; do
            echo "  ✅ $test (${TEST_TIMES[$test]}ms)"
        done
    fi

    if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
        echo ""
        echo -e "${RED}Failed tests:${NC}"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  ❌ $test (exit code: ${TEST_EXIT_CODES[$test]}, ${TEST_TIMES[$test]}ms)"
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

    # Also write total time to summary log
    echo "" >> "$SUMMARY_LOG"
    echo "============================================" >> "$SUMMARY_LOG"
    echo "Total execution time: $formatted_time" >> "$SUMMARY_LOG"
    echo "Max parallel jobs: $MAX_PARALLEL_JOBS" >> "$SUMMARY_LOG"
    echo "============================================" >> "$SUMMARY_LOG"
}

# Cleanup function
cleanup() {
    log_info "CI BT Classic cleaning up..."

    # Clean up temporary results directory if it exists
    rm -rf "$TMP_RESULTS_DIR" 2>/dev/null || true

    # Calculate total elapsed time in milliseconds (needed for both JUnit XML and summary)
    local script_end_time=$(date +%s%3N)
    local total_elapsed_ms=$((script_end_time - SCRIPT_START_TIME))

    # Generate JUnit XML report
    generate_junit_xml $total_elapsed_ms

    # Print summary
    print_summary $total_elapsed_ms

    # Return appropriate exit code
    if [[ "$HAS_FAILURE" == true ]]; then
        exit 1
    else
        exit 0
    fi
}

# Set cleanup on exit
trap cleanup EXIT

# Main function
main() {
    log_section "Bluetooth Classic Simulation Test Runner (Parallel Mode)"
    log_info "Script directory: $SCRIPT_DIR"
    log_info "Test directories file: $TEST_DIRS_FILE"
    log_info "Logs directory: $LOGS_DIR"
    log_info "Max parallel jobs: $MAX_PARALLEL_JOBS"

    # Validate environment
    if ! validate_zephyr_base; then
        HAS_FAILURE=true
        return 1
    fi

    # Check required commands
    if ! check_required_commands python west; then
        HAS_FAILURE=true
        return 1
    fi

    # Read test directories
    if ! read_test_directories; then
        log_error "Failed to read test directories"
        HAS_FAILURE=true
        return 1
    fi

    # Find test scripts
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

    # Run tests in parallel (with fallback to sequential)
    if ! run_tests_parallel; then
        run_tests_sequential
    fi

    # Summary will be printed in cleanup function
    # Exit code will be set based on HAS_FAILURE
}

# Run main function
main "$@"
