#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Smoketest for GAP Discovery

set -euo pipefail

# Create gap_discovery directory if it doesn't exist
mkdir -p ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery

# Redirect all output to log file while also displaying on terminal
exec > >(tee -a ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/script_output.log)
exec 2>&1

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

# Global variables
BUMBLE_CONTROLLER_PID=""
PERIPHERAL_PID=""
HCI_PORT_0="9000"
HCI_PORT_1="9001"
BD_ADDRESS_0=""
BD_ADDRESS_1=""

# Check and kill existing controllers.py processes
check_and_kill_existing_controllers() {
    log_info "Checking for existing controllers.py processes..."

    local controllers_script="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common/controllers.py"

    # Find all processes running controllers.py
    local pids=$(pgrep -f "$controllers_script" 2>/dev/null || true)

    if [[ -z "$pids" ]]; then
        log_info "No existing controllers.py processes found"
        return 0
    fi

    log_warn "Found existing controllers.py processes: $pids"

    # Kill each process
    for pid in $pids; do
        log_info "Attempting to kill process $pid..."

        # Try graceful shutdown first
        if kill -TERM "$pid" 2>/dev/null; then
            log_info "Sent SIGTERM to process $pid"

            # Wait for graceful shutdown
            local count=0
            while kill -0 "$pid" 2>/dev/null && [[ $count -lt 5 ]]; do
                sleep 1
                ((count++))
            done

            # Force kill if still running
            if kill -0 "$pid" 2>/dev/null; then
                log_warn "Process $pid still running, force killing..."
                kill -KILL "$pid" 2>/dev/null || true
                sleep 1
            fi
        fi

        # Check if process was killed
        if kill -0 "$pid" 2>/dev/null; then
            log_error "Failed to kill process $pid"
            return 1
        else
            log_info "Successfully killed process $pid"
        fi
    done

    log_info "All existing controllers.py processes have been terminated"

    # Wait a bit to ensure ports are released
    sleep 2

    return 0
}

# Start bumble controllers asynchronously
start_bumble_controllers() {
    log_info "Starting bumble controllers asynchronously..."

    # Check if python is available
    if ! command -v python &> /dev/null; then
        log_error "python command not found"
        return 1
    fi

    # Check if controllers.py exists
    local controllers_script="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common/controllers.py"
    if [[ ! -f "$controllers_script" ]]; then
        log_error "controllers.py not found at $controllers_script"
        return 1
    fi

    log_info "Starting bumble controllers on ports $HCI_PORT_0 and $HCI_PORT_1..."

    # Start bumble controllers asynchronously and redirect output to file
    python "$controllers_script" \
        "tcp-server:_:$HCI_PORT_0" \
        "tcp-server:_:$HCI_PORT_1" \
        > ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/bumble_controllers.log 2>&1 &

    BUMBLE_CONTROLLER_PID=$!

    log_info "Started bumble controllers with PID: $BUMBLE_CONTROLLER_PID"

    # Wait briefly to check if process started successfully
    sleep 2

    # Check if process is still running
    if ! kill -0 "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
        log_error "Bumble controllers process failed to start or exited immediately"
        cat ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/bumble_controllers.log
        return 1
    fi

    log_info "Bumble controllers started successfully"

    # Wait for controllers to initialize
    log_info "Waiting for bumble controllers to initialize..."
    sleep 3

    # Extract BD addresses from log file
    extract_bd_addresses

    return 0
}

# Extract BD addresses from bumble controller log
extract_bd_addresses() {
    log_info "Extracting BD addresses from bumble controller log..."

    local log_file="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/bumble_controllers.log"

    if [[ ! -f "$log_file" ]]; then
        log_error "Bumble controller log file not found"
        return 1
    fi

    # Remove ANSI color codes using sed and extract BD addresses
    # Use a more robust pattern to match BD addresses (XX:XX:XX:XX:XX:XX format)
    BD_ADDRESS_0=$(grep "HCI0 BD Address" "$log_file" | sed -r 's/\x1B\[[0-9;]*[mK]//g' | \
                   grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)
    BD_ADDRESS_1=$(grep "HCI1 BD Address" "$log_file" | sed -r 's/\x1B\[[0-9;]*[mK]//g' | \
                   grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)

    if [[ -z "$BD_ADDRESS_0" ]] || [[ -z "$BD_ADDRESS_1" ]]; then
        log_error "Failed to extract BD addresses from log file"
        log_info "Log file content:"
        cat "$log_file"
        log_info "Attempting alternative extraction method..."

        # Alternative method: use perl for better ANSI code removal
        BD_ADDRESS_0=$(grep "HCI0 BD Address" "$log_file" | perl -pe 's/\e\[[0-9;]*m//g' | \
                       grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)
        BD_ADDRESS_1=$(grep "HCI1 BD Address" "$log_file" | perl -pe 's/\e\[[0-9;]*m//g' | \
                       grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)

        if [[ -z "$BD_ADDRESS_0" ]] || [[ -z "$BD_ADDRESS_1" ]]; then
            log_error "Alternative extraction method also failed"
            return 1
        fi
    fi

    log_info "HCI0 BD Address: $BD_ADDRESS_0 (port $HCI_PORT_0)"
    log_info "HCI1 BD Address: $BD_ADDRESS_1 (port $HCI_PORT_1)"

    return 0
}

# Kill bumble controllers process
kill_bumble_controllers() {
    if [[ -n "$BUMBLE_CONTROLLER_PID" ]]; then
        log_info "Stopping bumble controllers (PID: $BUMBLE_CONTROLLER_PID)..."

        # Check if process is still running
        if ! kill -0 "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
            log_info "Bumble controllers process already stopped"
            BUMBLE_CONTROLLER_PID=""
            return 0
        fi

        # Try graceful shutdown first
        if kill -TERM "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
            log_info "Sent SIGTERM to bumble controllers"

            # Wait for graceful shutdown
            local count=0
            while kill -0 "$BUMBLE_CONTROLLER_PID" 2>/dev/null && [[ $count -lt 5 ]]; do
                sleep 1
                ((count++))
            done

            # Force kill if still running
            if kill -0 "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
                log_warn "Bumble controllers still running, force killing..."
                if kill -KILL "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
                    sleep 1
                fi
            fi
        fi

        # Final check
        if kill -0 "$BUMBLE_CONTROLLER_PID" 2>/dev/null; then
            log_warn "Bumble controllers process still running after kill attempts"
            # Don't return error, just warn
        else
            log_info "Bumble controllers stopped successfully"
        fi

        BUMBLE_CONTROLLER_PID=""
    fi

    return 0
}

# Display controller information
display_controller_info() {
    echo ""
    echo "Bumble Controller Information:"
    echo "=============================="
    echo "HCI0: tcp-server:127.0.0.1:$HCI_PORT_0"
    echo "      BD Address: $BD_ADDRESS_0"
    echo ""
    echo "HCI1: tcp-server:127.0.0.1:$HCI_PORT_1"
    echo "      BD Address: $BD_ADDRESS_1"
    echo ""
}

# Build peer device
build_peer_device() {
    log_info "Building peer device (peripheral)..."

    if [[ -z "$BD_ADDRESS_1" ]]; then
        log_error "BD_ADDRESS_1 not available for peripheral build"
        return 1
    fi

    local build_dir="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/peripheral"

    log_info "Executing build command for peripheral device"

    # Execute the build command directly
    if ! west build -b native_sim \
        ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery \
        -d "$build_dir" \
        -DCONFIG_TEST_GAP_PERIPHERAL=y \
        -DCONFIG_TEST_GAP_CENTRAL_ADDRESS=\"${BD_ADDRESS_1}\"; then
        log_error "Build failed for peer device"
        return 1
    fi

    log_info "Build completed successfully"

    # Check if zephyr.exe was generated
    local zephyr_exe="$build_dir/zephyr/zephyr.exe"

    log_info "Checking for generated executable: $zephyr_exe"

    if [[ ! -f "$zephyr_exe" ]]; then
        log_error "zephyr.exe not found at $zephyr_exe"
        return 1
    fi

    log_info "zephyr.exe found successfully at $zephyr_exe"

    # Optional: Check if the file is executable
    if [[ ! -x "$zephyr_exe" ]]; then
        log_warn "zephyr.exe exists but is not executable"
        chmod +x "$zephyr_exe"
        log_info "Made zephyr.exe executable"
    fi

    log_info "Peer device build verification completed"
    return 0
}

# Run peer device asynchronously
run_peer_device() {
    log_info "Running peripheral device asynchronously..."

    local peripheral_exe="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/peripheral/zephyr/zephyr.exe"

    # Check if peripheral executable exists
    if [[ ! -f "$peripheral_exe" ]]; then
        log_error "Peripheral executable not found: $peripheral_exe"
        return 1
    fi

    log_info "Starting peripheral device asynchronously"
    log_info "Using HCI transport: 127.0.0.1:$HCI_PORT_0"
    log_info "BD Address: $BD_ADDRESS_0"

    # Execute the test command asynchronously
    "$peripheral_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" &
    PERIPHERAL_PID=$!

    log_info "Peripheral device started with PID: $PERIPHERAL_PID"

    # Wait a moment to check if process started successfully
    sleep 2

    # Check if process is still running
    if ! kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
        log_error "Peripheral device process failed to start or exited immediately"
        PERIPHERAL_PID=""
        return 1
    fi

    log_info "Peripheral device is running successfully"
    return 0
}

# Kill peripheral device process
kill_peripheral_device() {
    if [[ -n "$PERIPHERAL_PID" ]]; then
        log_info "Stopping peripheral device (PID: $PERIPHERAL_PID)..."

        # Check if process is still running
        if ! kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
            log_info "Peripheral device process already stopped"
            PERIPHERAL_PID=""
            return 0
        fi

        # Try graceful shutdown first
        if kill -TERM "$PERIPHERAL_PID" 2>/dev/null; then
            log_info "Sent SIGTERM to peripheral device"

            # Wait for graceful shutdown
            local count=0
            while kill -0 "$PERIPHERAL_PID" 2>/dev/null && [[ $count -lt 10 ]]; do
                sleep 1
                ((count++))
            done

            # Force kill if still running
            if kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
                log_warn "Peripheral device still running, force killing..."
                if kill -KILL "$PERIPHERAL_PID" 2>/dev/null; then
                    sleep 1
                fi
            fi
        fi

        # Final check
        if kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
            log_warn "Peripheral device process still running after kill attempts"
            # Don't return error, just warn
        else
            log_info "Peripheral device stopped successfully"
        fi

        PERIPHERAL_PID=""
    fi

    return 0
}

# Build central device
build_central_device() {
    log_info "Building central device..."

    if [[ -z "$BD_ADDRESS_0" ]]; then
        log_error "BD_ADDRESS_0 not available for central build"
        return 1
    fi

    local central_dir="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/central"

    log_info "Executing build command for central device"
    log_info "Peripheral address configured as: $BD_ADDRESS_0"

    # Execute the build command directly
    if ! west build -b native_sim \
        ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery \
        -d "$central_dir" \
        -DCONFIG_TEST_GAP_CENTRAL=y \
        -DCONFIG_TEST_GAP_PERIPHERAL_ADDRESS=\"$BD_ADDRESS_0\"; then
        log_error "Build failed for central device"
        return 1
    fi

    log_info "Build completed successfully"

    # Check if zephyr.exe was generated
    local zephyr_exe="$central_dir/zephyr/zephyr.exe"

    log_info "Checking for generated executable: $zephyr_exe"

    if [[ ! -f "$zephyr_exe" ]]; then
        log_error "zephyr.exe not found at $zephyr_exe"
        return 1
    fi

    log_info "zephyr.exe found successfully at $zephyr_exe"

    # Optional: Check if the file is executable
    if [[ ! -x "$zephyr_exe" ]]; then
        log_warn "zephyr.exe exists but is not executable"
        chmod +x "$zephyr_exe"
        log_info "Made zephyr.exe executable"
    fi

    log_info "Central device build verification completed"
    return 0
}

# Run central device
run_central_device() {
    log_info "Running central device..."

    local central_exe="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/central/zephyr/zephyr.exe"

    # Check if central executable exists
    if [[ ! -f "$central_exe" ]]; then
        log_error "Central executable not found: $central_exe"
        return 1
    fi

    log_info "Executing central device test"
    log_info "Using HCI transport: 127.0.0.1:$HCI_PORT_1"
    log_info "BD Address: $BD_ADDRESS_1"

    # Execute the test command and capture exit code
    set +e
    "$central_exe" --bt-dev="127.0.0.1:$HCI_PORT_1"
    local exit_code=$?
    set -e

    if [[ $exit_code -ne 0 ]]; then
        log_error "Central device execution failed with exit code: $exit_code"
        return 1
    fi

    log_info "Central device execution completed successfully"
    return 0
}

# Wait for peripheral device to complete
wait_peripheral_device() {
    local timeout=${1:-30}  # Default timeout is 30 seconds

    if [[ -z "$PERIPHERAL_PID" ]]; then
        log_warn "No peripheral device PID to wait for"
        return 0
    fi

    log_info "Waiting for peripheral device to complete (PID: $PERIPHERAL_PID, max ${timeout}s)..."

    local wait_count=0

    while kill -0 "$PERIPHERAL_PID" 2>/dev/null && [[ $wait_count -lt $timeout ]]; do
        sleep 1
        ((wait_count++))

        # Log progress every 5 seconds
        if [[ $((wait_count % 5)) -eq 0 ]]; then
            log_info "Still waiting... ($wait_count/$timeout seconds)"
        fi
    done

    if kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
        log_warn "Peripheral device still running after ${timeout}s timeout"
        return 1
    else
        log_info "Peripheral device completed after ${wait_count}s"
        PERIPHERAL_PID=""  # Clear the PID since process has finished
        return 0
    fi
}

# Global variable to track test result
TEST_RESULT=0

# Cleanup function
cleanup() {
    local exit_code=$?

    log_info "Cleaning up..."

    # Temporarily disable exit on error for cleanup
    set +e

    # Kill peripheral device if running
    kill_peripheral_device

    # Kill bumble controllers
    kill_bumble_controllers

    # Re-enable exit on error
    set -e

    # Determine final exit code
    if [[ $TEST_RESULT -ne 0 ]]; then
        log_error "Script completed with errors"
        log_info "=========Sim controller log========="
        cat ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/bumble_controllers.log
        exit $TEST_RESULT
    elif [[ $exit_code -ne 0 ]]; then
        log_error "Script completed with errors (exit code: $exit_code)"
        log_info "=========Sim controller log========="
        cat ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/bumble_controllers.log
        exit $exit_code
    else
        log_info "Script completed successfully"
        exit 0
    fi
}

# Set cleanup on exit
trap cleanup EXIT

# Main function
main() {
    log_info "Starting GAP Discovery test with Bumble controllers..."

    # 0. Check and kill existing controllers.py processes
    if ! check_and_kill_existing_controllers; then
        log_error "Failed to clean up existing controllers.py processes"
        TEST_RESULT=1
        return 1
    fi

    # 1. Start bumble controllers
    if ! start_bumble_controllers; then
        log_error "Failed to start bumble controllers"
        TEST_RESULT=1
        return 1
    fi

    # 2. Display controller information
    display_controller_info

    # 3. Build peer device
    if ! build_peer_device; then
        log_error "Failed to build peer device"
        TEST_RESULT=1
        return 1
    fi

    # 4. Build central device
    if ! build_central_device; then
        log_error "Failed to build central device"
        TEST_RESULT=1
        return 1
    fi

    # 5. Run peer device
    if ! run_peer_device; then
        log_error "Failed to run peer device"
        TEST_RESULT=1
        return 1
    fi

    # 6. Run central device
    if ! run_central_device; then
        log_error "Failed to run central device"
        TEST_RESULT=1
        return 1
    fi

    # 7. Wait for peripheral device to complete
    if ! wait_peripheral_device 30; then
        log_warn "Peripheral device did not complete within timeout"
    fi

    log_info "PASSED"
    TEST_RESULT=0
    return 0
}

# Run main function
main "$@"
