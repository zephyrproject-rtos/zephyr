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

# Detect Linux distribution
detect_distro() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        echo "$ID"
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        echo "$DISTRIB_ID" | tr '[:upper:]' '[:lower:]'
    else
        echo "unknown"
    fi
}

# Check btvirt if needed
check_btvirt() {
    log_info "Checking for btvirt..."

    if command -v btvirt &> /dev/null; then
        log_info "btvirt is already installed"
        btvirt --version 2>/dev/null || log_info "btvirt found at: $(which btvirt)"
        return 0
    fi

    log_warn "btvirt not found"

    return 1
}

# Check hciconfig if needed
check_hciconfig() {
    if ! command -v hciconfig &> /dev/null; then
        log_warn "hciconfig not found"
        return 1
    fi
    return 0
}

# Check btmon if needed
check_btmon() {
    if ! command -v btmon &> /dev/null; then
        log_warn "btmon not found"
        return 1
    fi
    return 0
}

# Stop all running btvirt processes
stop_btvirt_processes() {
    log_info "Stopping all running btvirt processes..."

    # Find all btvirt processes
    local pids=$(pgrep -f "btvirt" 2>/dev/null || true)

    if [[ -z "$pids" ]]; then
        log_info "No running btvirt processes found"
        return 0
    fi

    log_info "Found btvirt processes: $pids"

    # Try graceful shutdown (SIGTERM)
    echo "$pids" | xargs -r kill -TERM 2>/dev/null || true

    # Wait for processes to stop
    sleep 2

    # Check if any processes are still running
    local remaining_pids=$(pgrep -f "btvirt" 2>/dev/null || true)

    if [[ -n "$remaining_pids" ]]; then
        log_warn "Some btvirt processes still running, force killing..."
        echo "$remaining_pids" | xargs -r kill -KILL 2>/dev/null || true
        sleep 1
    fi

    # Final check
    local final_check=$(pgrep -f "btvirt" 2>/dev/null || true)
    if [[ -n "$final_check" ]]; then
        log_error "Failed to stop all btvirt processes"
        return 1
    fi

    log_info "All btvirt processes stopped successfully"
    return 0
}

# Global variables to store btmon process PIDs
BTMON_HCI0_PID=""
BTMON_HCI1_PID=""

# Start btvirt asynchronously
start_btvirt_async() {
    log_info "Starting btvirt asynchronously..."

    # Check if btvirt command exists
    if ! command -v btvirt &> /dev/null; then
        log_error "btvirt command not found after installation check"
        return 1
    fi

    # Check if btmon command exists
    if ! command -v btmon &> /dev/null; then
        log_error "btmon command not found after installation check"
        return 1
    fi

    # Start btvirt asynchronously and get PID, redirect output to file
    sudo btvirt -d -l2 > ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/btvirt_output.log 2>&1 &
    local btvirt_pid=$!

    sudo btmon -i hci0 -w ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/hci0_log.cfa > \
        ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/hci0_log_output.log 2>&1 &
    BTMON_HCI0_PID=$!

    sudo btmon -i hci1 -w ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/hci1_log.cfa > \
        ${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/hci1_log_output.log 2>&1 &
    BTMON_HCI1_PID=$!

    log_info "Started btvirt with PID: $btvirt_pid"
    log_info "Started btmon for hci0 with PID: $BTMON_HCI0_PID"
    log_info "Started btmon for hci1 with PID: $BTMON_HCI1_PID"

    # Wait briefly to check if process started successfully
    sleep 1

    # Stop Bluetooth service
    sudo systemctl stop bluetooth 2>/dev/null || \
        log_warn "Could not stop bluetooth service (may not be running)"

    # Check if process is still running
    if ! kill -0 "$btvirt_pid" 2>/dev/null; then
        log_error "btvirt process failed to start or exited immediately"
        sudo journalctl -u bluetooth -n 50
        sudo dmesg | tail -50 | grep -i bluetooth
        return 1
    fi

    log_info "btvirt started successfully"

    # Wait for virtual HCI devices to initialize
    log_info "Waiting for virtual HCI devices to initialize..."
    sleep 3

    return 0
}

# Extract Virtual HCI devices and ensure they are DOWN
extract_virtual_hci() {
    log_info "Extracting Virtual HCI devices..."

    # Declare arrays
    declare -g -a virtual_devices=()
    declare -g -a virtual_addresses=()

    # Check hciconfig command
    if ! command -v hciconfig &> /dev/null; then
        log_error "hciconfig command not found after installation check"
        return 1
    fi

    # Extract Virtual HCI device information and store in arrays
    local count=0
    while IFS=' ' read -r device address; do
        if [[ -n "$device" && -n "$address" ]]; then
            virtual_devices+=("$device")
            virtual_addresses+=("$address")
            ((count++))
        fi
    done < <(hciconfig -a 2>/dev/null | awk '
    /^hci[0-9]+:/ {
        device = $1
        gsub(/:/, "", device)
        virtual = 0
    }
    /Bus: Virtual/ {
        virtual = 1
    }
    /BD Address:/ && virtual {
        print device " " $3
        virtual = 0
    }')

    if [[ $count -eq 0 ]]; then
        log_warn "No Virtual HCI devices found"
        return 1
    fi

    if [[ $count -lt 2 ]]; then
        log_warn "At least 2 Virtual HCI devices are required, but only $count found"
        return 1
    fi

    log_info "Found $count Virtual HCI devices"

    # Check and fix status of all devices
    local has_error=false
    local auto_fix=true  # Set to false if you don't want automatic fixing

    for i in "${!virtual_devices[@]}"; do
        local device="${virtual_devices[$i]}"
        local address="${virtual_addresses[$i]}"

        # Get device status
        local status_output
        status_output=$(hciconfig "$device" 2>/dev/null)

        if echo "$status_output" | grep -q "UP RUNNING\|UP"; then
            log_warn "Device $device ($address) is UP - Expected DOWN"

            if [[ "$auto_fix" == true ]]; then
                log_info "Attempting to bring down $device..."
                if sudo hciconfig "$device" down 2>/dev/null; then
                    log_info "Successfully brought down $device"
                else
                    log_error "Failed to bring down $device"
                    has_error=true
                fi
            else
                log_error "Device $device is not DOWN"
                has_error=true
            fi
        elif echo "$status_output" | grep -q "DOWN"; then
            log_info "Device $device ($address): DOWN - OK"
        else
            log_warn "Device $device ($address): Unknown status"
        fi
    done

    if [[ "$has_error" == true ]]; then
        log_error "Some Virtual HCI devices could not be brought to DOWN state"
        return 1
    fi

    log_info "All Virtual HCI devices are in DOWN state"
    return 0
}

# Display results
display_results() {
    echo ""
    echo "Virtual HCI Devices:"
    echo "==================="

    for i in "${!virtual_devices[@]}"; do
        echo "[$i] Device: ${virtual_devices[$i]}, Address: ${virtual_addresses[$i]}"
    done

    echo ""
    echo "Array Information:"
    echo "Total devices: ${#virtual_devices[@]}"
    echo "All devices: ${virtual_devices[*]}"
    echo "All addresses: ${virtual_addresses[*]}"

    if [[ ${#virtual_devices[@]} -gt 0 ]]; then
        echo "First device: ${virtual_devices[0]}"
        echo "First address: ${virtual_addresses[0]}"
    fi
}

# Build peer device
build_peer_device() {
    log_info "Building peer device (peripheral)..."

    local build_dir="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/peripheral"

    log_info "Executing build command for peripheral device"

    # Execute the build command directly
    if ! west build -b native_sim \
        ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery \
        -d "$build_dir" \
        -DCONFIG_TEST_GAP_PERIPHERAL=y \
        -DCONFIG_TEST_GAP_CENTRAL_ADDRESS=\"${virtual_addresses[1]}\"; then
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

# Global variable to store peripheral process PID
PERIPHERAL_PID=""

# Run peer device asynchronously
run_peer_device() {
    log_info "Running peripheral device asynchronously..."

    local peripheral_exe="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/peripheral/zephyr/zephyr.exe"

    # Check if peripheral executable exists
    if [[ ! -f "$peripheral_exe" ]]; then
        log_error "Peripheral executable not found: $peripheral_exe"
        return 1
    fi

    # Check if we have virtual devices
    if [[ ${#virtual_devices[@]} -eq 0 ]]; then
        log_error "No virtual devices available for testing"
        return 1
    fi

    # Get the first virtual device
    local first_device="${virtual_devices[0]}"

    log_info "Starting peripheral device asynchronously"
    log_info "Using virtual device: $first_device (${virtual_addresses[0]})"

    # Execute the test command asynchronously
    sudo "$peripheral_exe" --bt-dev="$first_device" &
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
                kill -KILL "$PERIPHERAL_PID" 2>/dev/null || true
                sleep 1
            fi
        fi

        # Final check
        if kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
            log_error "Failed to stop peripheral device"
        else
            log_info "Peripheral device stopped successfully"
        fi

        PERIPHERAL_PID=""
    fi
}

# Build central device
build_central_device() {
    log_info "Building central device..."

    # Check if we have at least 2 virtual devices
    if [[ ${#virtual_devices[@]} -lt 2 ]]; then
        log_error "Need at least 2 virtual devices for testing. Found: ${#virtual_devices[@]}"
        return 1
    fi

    local peripheral_address="${virtual_addresses[0]}"
    local central_dir="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/central"

    log_info "Executing build command for central device"
    log_info "Peripheral address configured as: $peripheral_address"

    # Execute the build command directly
    if ! west build -b native_sim \
        ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery \
        -d "$central_dir" \
        -DCONFIG_TEST_GAP_CENTRAL=y \
        -DCONFIG_TEST_GAP_PERIPHERAL_ADDRESS=\"$peripheral_address\"; then
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

    # Check if we have at least 2 virtual devices
    if [[ ${#virtual_devices[@]} -lt 2 ]]; then
        log_error "Need at least 2 virtual devices for testing. Found: ${#virtual_devices[@]}"
        return 1
    fi

    local central_exe="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery/central/zephyr/zephyr.exe"

    # Check if central executable exists
    if [[ ! -f "$central_exe" ]]; then
        log_error "Central executable not found: $central_exe"
        return 1
    fi

    # Get the second virtual device for central
    local second_device="${virtual_devices[1]}"

    log_info "Executing central device test"
    log_info "Using central device: $second_device (${virtual_addresses[1]})"

    # Execute the test command and capture exit code
    set +e
    sudo "$central_exe" --bt-dev="$second_device"
    local exit_code=$?
    set -e

    if [[ $exit_code -ne 0 ]]; then
        log_error "Central device execution failed with exit code: $exit_code"
        return 1
    fi

    log_info "Central device execution completed successfully"
    return 0
}

# Kill btmon processes
kill_btmon_processes() {
    # Kill btmon for hci0
    if [[ -n "$BTMON_HCI0_PID" ]]; then
        log_info "Stopping btmon for hci0 (PID: $BTMON_HCI0_PID)..."
        if kill -TERM "$BTMON_HCI0_PID" 2>/dev/null; then
            sleep 1
            if kill -0 "$BTMON_HCI0_PID" 2>/dev/null; then
                kill -KILL "$BTMON_HCI0_PID" 2>/dev/null || true
            fi
            log_info "btmon for hci0 stopped"
        fi
        BTMON_HCI0_PID=""
    fi

    # Kill btmon for hci1
    if [[ -n "$BTMON_HCI1_PID" ]]; then
        log_info "Stopping btmon for hci1 (PID: $BTMON_HCI1_PID)..."
        if kill -TERM "$BTMON_HCI1_PID" 2>/dev/null; then
            sleep 1
            if kill -0 "$BTMON_HCI1_PID" 2>/dev/null; then
                kill -KILL "$BTMON_HCI1_PID" 2>/dev/null || true
            fi
            log_info "btmon for hci1 stopped"
        fi
        BTMON_HCI1_PID=""
    fi
}

# Global variable to track test result
TEST_RESULT=0

# Cleanup function
cleanup() {
    log_info "Cleaning up..."

    # Kill peripheral device if running
    kill_peripheral_device

    # Kill btmon processes
    kill_btmon_processes

    # Optionally stop btvirt on script exit
    stop_btvirt_processes

    # Exit with the test result
    if [[ $TEST_RESULT -eq 0 ]]; then
        log_info "Script completed successfully"
    else
        log_error "Script completed with errors"
    fi

    exit $TEST_RESULT
}

# Set cleanup on exit
trap cleanup EXIT

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

# Main function
main() {
    log_info "Starting Virtual HCI extraction with btvirt management..."

    # 0. Check and install required tools
    log_info "Checking required tools..."

    if ! check_btvirt; then
        log_error "Failed to ensure btvirt is installed"
        TEST_RESULT=1
        return 1
    fi

    if ! check_hciconfig; then
        log_error "Failed to ensure hciconfig is installed"
        TEST_RESULT=1
        return 1
    fi

    if ! check_btmon; then
        log_error "Failed to ensure btmon is installed"
        TEST_RESULT=1
        return 1
    fi

    # 1. Stop all btvirt processes
    if ! stop_btvirt_processes; then
        log_error "Failed to stop existing btvirt processes"
        TEST_RESULT=1
        return 1
    fi

    # 2. Start btvirt asynchronously
    if ! start_btvirt_async; then
        log_error "Failed to start btvirt"
        TEST_RESULT=1
        return 1
    fi

    # 3. Extract Virtual HCI devices
    if ! extract_virtual_hci; then
        log_error "Failed to extract Virtual HCI devices"
        TEST_RESULT=1
        return 1
    fi

    # 4. Display results
    display_results

    # 5. Build peer device
    if ! build_peer_device; then
        log_error "Failed to build peer device"
        TEST_RESULT=1
        return 1
    fi

    # 6. Build central device
    if ! build_central_device; then
        log_error "Failed to build central device"
        TEST_RESULT=1
        return 1
    fi

    # 7. Run peer device
    if ! run_peer_device; then
        log_error "Failed to run peer device"
        TEST_RESULT=1
        return 1
    fi

    # 8. Run central device
    if ! run_central_device; then
        log_error "Failed to run central device"
        TEST_RESULT=1
        return 1
    fi

    # 9. Wait for peripheral device to complete
    if ! wait_peripheral_device 30; then
        log_warn "Peripheral device did not complete within timeout"
    fi

    log_info "PASSED"
    TEST_RESULT=0
    return 0
}

# Run main function
main "$@"
