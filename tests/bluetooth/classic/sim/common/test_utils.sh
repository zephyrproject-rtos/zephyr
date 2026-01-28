#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Common utilities for Bluetooth Classic simulation tests
# Thread-safe: Enhanced with proper locking mechanisms

# Prevent multiple sourcing
if [[ -n "${TEST_UTILS_LOADED:-}" ]]; then
    return 0
fi
readonly TEST_UTILS_LOADED=1

# Declare global variables used for port management
declare -a ALLOCATED_PORTS=()

# Color definitions (read-only, safe for multi-threading)
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

# Port allocation settings
readonly PORT_LOCK_DIR="${TMPDIR:-/tmp}/bt_port_locks"
readonly PORT_MARKER_DIR="${TMPDIR:-/tmp}/bt_port_markers"

# Initialize port allocation directories
mkdir -p "$PORT_LOCK_DIR" "$PORT_MARKER_DIR" 2>/dev/null || true

# Logging functions (stateless)
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

# Function to escape XML special characters (stateless)
xml_escape() {
    local text="$1"
    text="${text//&/&amp;}"
    text="${text//</&lt;}"
    text="${text//>/&gt;}"
    text="${text//\"/&quot;}"
    text="${text//\'/&apos;}"
    echo "$text"
}

# Thread-safe function to find available port
# Uses directory-based locking for atomicity
# Returns: port number on stdout, exit code 0 on success
find_available_port() {
    local start_port=${1:-9000}
    local max_attempts=200
    local port_range=10000

    for ((attempt=0; attempt<max_attempts; attempt++)); do
        # Use random port to reduce collision probability
        local port=$((start_port + RANDOM % port_range))
        local lock_file="$PORT_LOCK_DIR/port_${port}.lock"
        local marker_file="$PORT_MARKER_DIR/port_${port}.pid"

        # Try to create lock directory atomically
        if mkdir "$lock_file" 2>/dev/null; then
            # We got the lock, now check if port is actually available
            if ! netstat -tuln 2>/dev/null | grep -qE "[:.]$port[[:space:]]" && \
               ! ss -tuln 2>/dev/null | grep -qE "[:.]$port[[:space:]]"; then

                # Port is available, create marker with our PID
                echo "$BASHPID" > "$marker_file" 2>/dev/null

                # Add port to ALLOCATED_PORTS array for cleanup
                ALLOCATED_PORTS+=("$port")

                # Keep the lock - it will be released by release_port_lock or cleanup
                echo "$port"
                return 0
            else
                # Port is in use, release lock
                rmdir "$lock_file" 2>/dev/null || true
            fi
        fi

        # Add small random delay to reduce contention
        if [[ $attempt -gt 10 ]]; then
            local delay=$((RANDOM % 100 + 1))
            sleep 0.0$delay 2>/dev/null || sleep 0.01
        fi
    done

    log_error "Could not find available port after $max_attempts attempts"
    return 1
}

# Function to release port lock explicitly
# Args: port_number
# Returns: 0 on success
release_port_lock() {
    local port="$1"

    if [[ -z "$port" ]]; then
        return 0
    fi

    local lock_file="$PORT_LOCK_DIR/port_${port}.lock"
    local marker_file="$PORT_MARKER_DIR/port_${port}.pid"

    # Remove marker if it's ours
    if [[ -f "$marker_file" ]]; then
        local pid=$(cat "$marker_file" 2>/dev/null)
        if [[ "$pid" == "$$" ]]; then
            rm -f "$marker_file" 2>/dev/null || true
        fi
    fi

    # Remove lock directory
    if [[ -d "$lock_file" ]]; then
        rmdir "$lock_file" 2>/dev/null || true
    fi

    return 0
}

# Function to cleanup stale port locks
# Args: max_age_minutes (default: 60)
# Returns: 0 on success
cleanup_stale_port_locks() {
    local max_age_minutes=${1:-60}

    # Cleanup stale lock directories
    if [[ -d "$PORT_LOCK_DIR" ]]; then
        find "$PORT_LOCK_DIR" -type d -name "port_*.lock" \
            -mmin +$max_age_minutes -exec rmdir {} \; 2>/dev/null || true
    fi

    # Cleanup markers for dead processes
    if [[ -d "$PORT_MARKER_DIR" ]]; then
        for marker_file in "$PORT_MARKER_DIR"/port_*.pid; do
            [[ -f "$marker_file" ]] || continue

            local pid=$(cat "$marker_file" 2>/dev/null)
            if [[ -n "$pid" ]] && ! kill -0 "$pid" 2>/dev/null; then
                # Process is dead, remove marker
                rm -f "$marker_file" 2>/dev/null || true

                # Also try to remove corresponding lock
                local port="${marker_file##*/port_}"
                port="${port%.pid}"
                local lock_file="$PORT_LOCK_DIR/port_${port}.lock"
                rmdir "$lock_file" 2>/dev/null || true
            fi
        done
    fi

    return 0
}

# Function to kill process gracefully (stateless)
# Args: pid, process_name, timeout
# Returns: 0 on success, 1 on failure
kill_process_graceful() {
    local pid="$1"
    local process_name="${2:-process}"
    local timeout="${3:-5}"

    if [[ -z "$pid" ]]; then
        return 0
    fi

    log_info "Stopping $process_name (PID: $pid)..."

    # Check if process is still running
    if ! kill -0 "$pid" 2>/dev/null; then
        log_info "$process_name already stopped"
        return 0
    fi

    # Try graceful shutdown first
    if kill -TERM "$pid" 2>/dev/null; then
        log_info "Sent SIGTERM to $process_name"

        # Wait for graceful shutdown
        local count=0
        while kill -0 "$pid" 2>/dev/null && [[ $count -lt $timeout ]]; do
            sleep 1
            ((count++))
        done

        # Force kill if still running
        if kill -0 "$pid" 2>/dev/null; then
            log_warn "$process_name still running, force killing..."
            if kill -KILL "$pid" 2>/dev/null; then
                sleep 1
            fi
        fi
    fi

    # Final check
    if kill -0 "$pid" 2>/dev/null; then
        log_warn "$process_name still running after kill attempts"
        return 1
    else
        log_info "$process_name stopped successfully"
        return 0
    fi
}

# Function to validate ZEPHYR_BASE (stateless)
# Returns: 0 on success, 1 on failure
validate_zephyr_base() {
    if [[ -z "${ZEPHYR_BASE:-}" ]]; then
        log_error "ZEPHYR_BASE environment variable is not set"
        log_error "Please run: source zephyr/zephyr-env.sh"
        return 1
    fi

    if [[ ! -d "$ZEPHYR_BASE" ]]; then
        log_error "ZEPHYR_BASE directory does not exist: $ZEPHYR_BASE"
        return 1
    fi

    log_info "ZEPHYR_BASE: $ZEPHYR_BASE"
    return 0
}

# Function to check required commands (stateless)
# Args: command1 [command2 ...]
# Returns: 0 if all commands exist, 1 otherwise
check_required_commands() {
    local commands=("$@")
    local missing=()

    for cmd in "${commands[@]}"; do
        if ! command -v "$cmd" &> /dev/null; then
            missing+=("$cmd")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required commands: ${missing[*]}"
        return 1
    fi

    return 0
}

# Function to create temporary file with cleanup
# Args: prefix
# Returns: temp file path on stdout
create_temp_file() {
    local prefix="${1:-bt_test}"
    local temp_file=$(mktemp "/tmp/${prefix}.XXXXXX")
    echo "$temp_file"
}

# Function to create temporary directory with cleanup
# Args: prefix
# Returns: temp directory path on stdout
create_temp_dir() {
    local prefix="${1:-bt_test}"
    local temp_dir=$(mktemp -d "/tmp/${prefix}.XXXXXX")
    echo "$temp_dir"
}

# Function to parse and display ZTEST results from a single log file
# Args: log_file, device_name
# Returns: 0 on success, 1 if log not found or no summary
parse_ztest_log() {
    local log_file="$1"
    local device_name="${2:-Device}"

    if [[ ! -f "$log_file" ]]; then
        echo -e "${RED}Log file not found: $log_file${NC}"
        return 1
    fi

    if ! grep -q "TESTSUITE SUMMARY START" "$log_file"; then
        echo -e "${YELLOW}No TESTSUITE SUMMARY found in $device_name log${NC}"
        return 1
    fi

    # Extract and display the complete TESTSUITE SUMMARY section with colors
    sed -n '/TESTSUITE SUMMARY START/,/TESTSUITE SUMMARY END/p' "$log_file" | \
    while IFS= read -r line; do
        # Colorize output based on content
        if [[ "$line" =~ "SUITE PASS" ]]; then
            echo -e "${GREEN}${line}${NC}"
        elif [[ "$line" =~ "SUITE FAIL" ]]; then
            echo -e "${RED}${line}${NC}"
        elif [[ "$line" =~ "- PASS -" ]]; then
            echo -e "${GREEN}${line}${NC}"
        elif [[ "$line" =~ "- FAIL -" ]]; then
            echo -e "${RED}${line}${NC}"
        elif [[ "$line" =~ "- SKIP -" ]]; then
            echo -e "${YELLOW}${line}${NC}"
        elif [[ "$line" =~ "TESTSUITE SUMMARY" ]]; then
            echo -e "${BLUE}${line}${NC}"
        else
            echo "$line"
        fi
    done

    return 0
}

# Function to extract ZTEST statistics from a log file
# Args: log_file
# Outputs: "pass fail skip" on stdout
# Returns: 0 on success, 1 on failure
extract_ztest_stats() {
    local log_file="$1"
    local pass=0
    local fail=0
    local skip=0

    if [[ ! -f "$log_file" ]]; then
        echo "0 0 0"
        return 1
    fi

    if grep -q "TESTSUITE SUMMARY START" "$log_file"; then
        pass=$(sed -n '/TESTSUITE SUMMARY START/,/TESTSUITE SUMMARY END/p' "$log_file" | \
              grep -oP 'pass = \K\d+' | head -1 || echo "0")
        fail=$(sed -n '/TESTSUITE SUMMARY START/,/TESTSUITE SUMMARY END/p' "$log_file" | \
              grep -oP 'fail = \K\d+' | head -1 || echo "0")
        skip=$(sed -n '/TESTSUITE SUMMARY START/,/TESTSUITE SUMMARY END/p' "$log_file" | \
              grep -oP 'skip = \K\d+' | head -1 || echo "0")
    fi

    echo "$pass $fail $skip"
    return 0
}

# Function to display ZTEST results from multiple log files
# Args: log_file1:device_name1 [log_file2:device_name2 ...]
# Example: display_ztest_results "$CENTRAL_LOG:Central" "$PERIPHERAL_LOG:Peripheral"
# Returns: 0 if all tests passed, 1 if any failures
display_ztest_results() {
    local log_entries=("$@")
    local total_pass=0
    local total_fail=0
    local total_skip=0
    local device_stats=()

    log_section "ZTEST Results Summary"
    echo ""

    # Parse each log file
    for entry in "${log_entries[@]}"; do
        local log_file="${entry%%:*}"
        local device_name="${entry#*:}"

        # Default device name if not provided
        if [[ "$log_file" == "$device_name" ]]; then
            device_name="Device"
        fi

        echo "==================================================================="
        echo "                  $device_name Test Results"
        echo "==================================================================="

        parse_ztest_log "$log_file" "$device_name"

        echo ""

        # Extract statistics
        local stats
        stats=$(extract_ztest_stats "$log_file")
        read -r pass fail skip <<< "$stats"

        # Accumulate totals
        total_pass=$((total_pass + pass))
        total_fail=$((total_fail + fail))
        total_skip=$((total_skip + skip))

        # Store device stats for summary
        device_stats+=("$device_name:$pass:$fail:$skip")
    done

    # Display overall statistics
    echo "==================================================================="
    echo "                   Overall Test Statistics"
    echo "==================================================================="

    for stat_entry in "${device_stats[@]}"; do
        IFS=':' read -r dev_name dev_pass dev_fail dev_skip <<< "$stat_entry"
        printf "  %-15s PASS: %-3s FAIL: %-3s SKIP: %-3s\n" \
               "$dev_name:" "$dev_pass" "$dev_fail" "$dev_skip"
    done

    echo "==================================================================="

    local stats="PASS: $total_pass  FAIL: $total_fail  SKIP: $total_skip"
    if [[ $total_fail -eq 0 ]]; then
        echo -e "  ${GREEN}TOTAL:          $stats${NC}"
        echo -e "  ${GREEN}✓ All tests passed successfully!${NC}"
    else
        echo -e "  ${RED}TOTAL:          $stats${NC}"
        echo -e "  ${RED}✗ Some tests failed!${NC}"
    fi

    echo "==================================================================="
    echo ""

    # Return failure if any tests failed
    if [[ $total_fail -gt 0 ]]; then
        return 1
    fi

    return 0
}

# Cleanup function to be called on script exit
cleanup_port_resources() {
    # Clear all allocated ports
    for port in "${ALLOCATED_PORTS[@]}"; do
        release_port_lock "$port"
    done

    # Cleanup any locks/markers created by this process
    if [[ -d "$PORT_MARKER_DIR" ]]; then
        for marker_file in "$PORT_MARKER_DIR"/port_*.pid; do
            [[ -f "$marker_file" ]] || continue

            local pid=$(cat "$marker_file" 2>/dev/null)
            if [[ "$pid" == "$$" ]]; then
                rm -f "$marker_file" 2>/dev/null || true

                # Also remove corresponding lock
                local port="${marker_file##*/port_}"
                port="${port%.pid}"
                local lock_file="$PORT_LOCK_DIR/port_${port}.lock"
                rmdir "$lock_file" 2>/dev/null || true
            fi
        done
    fi
}

# Register cleanup on script exit
trap cleanup_port_resources EXIT
