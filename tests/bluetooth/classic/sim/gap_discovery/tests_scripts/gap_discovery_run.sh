#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Smoketest for GAP Discovery

set -euo pipefail

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

# Source common utilities
source "$COMMON_DIR/test_utils.sh"
source "$COMMON_DIR/bumble_utils.sh"

# Create gap_discovery directory if it doesn't exist
TEST_OUTPUT_DIR="${ZEPHYR_BASE}/bt_classic_sim/gap_discovery"
mkdir -p "$TEST_OUTPUT_DIR"

# Define all log file paths at the beginning
SCRIPT_OUTPUT_LOG="$TEST_OUTPUT_DIR/script_output.log"
PERIPHERAL_RUN_LOG="$TEST_OUTPUT_DIR/peripheral_run.log"
CENTRAL_RUN_LOG="$TEST_OUTPUT_DIR/central_run.log"
PERIPHERAL_BUILD_LOG="$TEST_OUTPUT_DIR/peripheral_build.log"
CENTRAL_BUILD_LOG="$TEST_OUTPUT_DIR/central_build.log"
BUMBLE_CONTROLLERS_LOG="$TEST_OUTPUT_DIR/bumble_controllers.log"

# Clean up previous logs
log_info "Cleaning up previous logs..."
rm -f "$SCRIPT_OUTPUT_LOG"
rm -f "$PERIPHERAL_RUN_LOG"
rm -f "$CENTRAL_RUN_LOG"
rm -f "$PERIPHERAL_BUILD_LOG"
rm -f "$CENTRAL_BUILD_LOG"
rm -f "$BUMBLE_CONTROLLERS_LOG"

# Create temporary build directory
BUILD_TEMP_DIR=$(create_temp_dir "gap_discovery_build")
log_info "Created temporary build directory: $BUILD_TEMP_DIR"

# Redirect all output to log file while also displaying on terminal
exec > >(tee -a "$SCRIPT_OUTPUT_LOG")
exec 2>&1

# Local variables (not global to library)
BUMBLE_CONTROLLER_PID=""
PERIPHERAL_PID=""
HCI_PORT_0=""
HCI_PORT_1=""
BD_ADDRESS_0=""
BD_ADDRESS_1=""
TEST_RESULT=0

# Build directory paths
PERIPHERAL_BUILD_DIR="$BUILD_TEMP_DIR/peripheral"
CENTRAL_BUILD_DIR="$BUILD_TEMP_DIR/central"

# Allocate dynamic ports
allocate_ports() {
    log_info "Allocating dynamic ports..."

    HCI_PORT_0=$(find_available_port 9000)
    if [[ -z "$HCI_PORT_0" ]]; then
        log_error "Failed to allocate HCI_PORT_0"
        return 1
    fi

    HCI_PORT_1=$(find_available_port $((HCI_PORT_0 + 1)))
    if [[ -z "$HCI_PORT_1" ]]; then
        log_error "Failed to allocate HCI_PORT_1"
        # Release the first port if second allocation fails
        release_port_lock "$HCI_PORT_0"
        return 1
    fi

    log_info "Allocated ports: HCI_PORT_0=$HCI_PORT_0, HCI_PORT_1=$HCI_PORT_1"
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

    log_info "Build directory: $PERIPHERAL_BUILD_DIR"
    log_info "Build log: $PERIPHERAL_BUILD_LOG"

    if ! west build -b native_sim \
        "${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery" \
        -d "$PERIPHERAL_BUILD_DIR" \
        -DCONFIG_TEST_GAP_PERIPHERAL=y \
        -DCONFIG_TEST_GAP_CENTRAL_ADDRESS=\"${BD_ADDRESS_1}\" \
        > "$PERIPHERAL_BUILD_LOG" 2>&1; then
        log_error "Build failed for peer device"
        return 1
    fi

    log_info "Build completed successfully"
    return 0
}

# Run peer device asynchronously
run_peer_device() {
    log_info "Running peripheral device asynchronously..."

    local peripheral_exe="$PERIPHERAL_BUILD_DIR/zephyr/zephyr.exe"

    if [[ ! -f "$peripheral_exe" ]]; then
        log_error "Peripheral executable not found: $peripheral_exe"
        return 1
    fi

    log_info "Starting peripheral device asynchronously"
    log_info "Using HCI transport: 127.0.0.1:$HCI_PORT_0"
    log_info "Peripheral log: $PERIPHERAL_RUN_LOG"

    "$peripheral_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" \
        > "$PERIPHERAL_RUN_LOG" 2>&1 &
    PERIPHERAL_PID=$!

    log_info "Peripheral device started with PID: $PERIPHERAL_PID"

    sleep 2

    if ! kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
        log_error "Peripheral device process failed to start"
        PERIPHERAL_PID=""
        return 1
    fi

    log_info "Peripheral device is running successfully"
    return 0
}

# Build local device
build_local_device() {
    log_info "Building local device (central)..."

    if [[ -z "$BD_ADDRESS_0" ]]; then
        log_error "BD_ADDRESS_0 not available for local build"
        return 1
    fi

    log_info "Build directory: $CENTRAL_BUILD_DIR"
    log_info "Build log: $CENTRAL_BUILD_LOG"

    if ! west build -b native_sim \
        "${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery" \
        -d "$CENTRAL_BUILD_DIR" \
        -DCONFIG_TEST_GAP_CENTRAL=y \
        -DCONFIG_TEST_GAP_PERIPHERAL_ADDRESS=\"$BD_ADDRESS_0\" \
        > "$CENTRAL_BUILD_LOG" 2>&1; then
        log_error "Build failed for local device"
        return 1
    fi

    log_info "Build completed successfully"
    return 0
}

# Run local device
run_local_device() {
    log_info "Running local device..."

    local central_exe="$CENTRAL_BUILD_DIR/zephyr/zephyr.exe"

    if [[ ! -f "$central_exe" ]]; then
        log_error "Central executable not found: $central_exe"
        return 1
    fi

    log_info "Executing local device test"
    log_info "Using HCI transport: 127.0.0.1:$HCI_PORT_1"
    log_info "Central log: $CENTRAL_RUN_LOG"

    set +e
    "$central_exe" --bt-dev="127.0.0.1:$HCI_PORT_1" \
        > "$CENTRAL_RUN_LOG" 2>&1
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
    local timeout=${1:-30}

    if [[ -z "$PERIPHERAL_PID" ]]; then
        log_warn "No peripheral device PID to wait for"
        return 0
    fi

    log_info "Waiting for peripheral device to complete (max ${timeout}s)..."

    local wait_count=0
    while kill -0 "$PERIPHERAL_PID" 2>/dev/null && [[ $wait_count -lt $timeout ]]; do
        sleep 1
        ((wait_count++))

        if [[ $((wait_count % 5)) -eq 0 ]]; then
            log_info "Still waiting... ($wait_count/$timeout seconds)"
        fi
    done

    if kill -0 "$PERIPHERAL_PID" 2>/dev/null; then
        log_warn "Peripheral device still running after ${timeout}s timeout"
        return 1
    else
        log_info "Peripheral device completed after ${wait_count}s"
        PERIPHERAL_PID=""
        return 0
    fi
}

# Cleanup function
cleanup() {
    local exit_code=$?

    log_info "Cleaning up..."
    set +e

    # Kill peripheral device
    if [[ -n "$PERIPHERAL_PID" ]]; then
        kill_process_graceful "$PERIPHERAL_PID" "peripheral device" 3
        PERIPHERAL_PID=""
    fi

    # Stop bumble controllers and release port locks
    if [[ -n "$BUMBLE_CONTROLLER_PID" ]]; then
        # Pass both the PID and the ports to stop_bumble_controllers
        if [[ -n "$HCI_PORT_0" ]] && [[ -n "$HCI_PORT_1" ]]; then
            stop_bumble_controllers "$BUMBLE_CONTROLLER_PID" "$HCI_PORT_0" "$HCI_PORT_1"
        else
            # Fallback: just stop the controller without port cleanup
            stop_bumble_controllers "$BUMBLE_CONTROLLER_PID"
        fi
        BUMBLE_CONTROLLER_PID=""
    else
        # If controller wasn't started but ports were allocated, release them
        if [[ -n "$HCI_PORT_0" ]]; then
            log_info "Releasing HCI_PORT_0: $HCI_PORT_0"
            release_port_lock "$HCI_PORT_0"
        fi
        if [[ -n "$HCI_PORT_1" ]]; then
            log_info "Releasing HCI_PORT_1: $HCI_PORT_1"
            release_port_lock "$HCI_PORT_1"
        fi
    fi

    # Clean up temporary build directory
    if [[ -n "$BUILD_TEMP_DIR" ]] && [[ -d "$BUILD_TEMP_DIR" ]]; then
        log_info "Removing temporary build directory: $BUILD_TEMP_DIR"
        rm -rf "$BUILD_TEMP_DIR"
    fi

    set -e

    # Determine final exit code
    if [[ $TEST_RESULT -ne 0 ]]; then
        log_error "Script completed with errors"
        log_info "=========Peripheral build log========="
        cat "$PERIPHERAL_BUILD_LOG" 2>/dev/null || true
        log_info "=========Central build log========="
        cat "$CENTRAL_BUILD_LOG" 2>/dev/null || true
        log_info "=========Sim controller log========="
        cat "$BUMBLE_CONTROLLERS_LOG" 2>/dev/null || true
        log_info "=========Central log========="
        cat "$CENTRAL_RUN_LOG" 2>/dev/null || true
        log_info "=========Peripheral log========="
        cat "$PERIPHERAL_RUN_LOG" 2>/dev/null || true
        exit $TEST_RESULT
    elif [[ $exit_code -ne 0 ]]; then
        log_error "Script completed with errors (exit code: $exit_code)"
        log_info "=========Peripheral build log========="
        cat "$PERIPHERAL_BUILD_LOG" 2>/dev/null || true
        log_info "=========Central build log========="
        cat "$CENTRAL_BUILD_LOG" 2>/dev/null || true
        log_info "=========Sim controller log========="
        cat "$BUMBLE_CONTROLLERS_LOG" 2>/dev/null || true
        log_info "=========Central log========="
        cat "$CENTRAL_RUN_LOG" 2>/dev/null || true
        log_info "=========Peripheral log========="
        cat "$PERIPHERAL_RUN_LOG" 2>/dev/null || true
        exit $exit_code
    else
        # Display ZTEST results on success using common utility function
        display_ztest_results \
            "$CENTRAL_RUN_LOG:Central" \
            "$PERIPHERAL_RUN_LOG:Peripheral"

        log_info "Script completed successfully"
        log_info "Logs saved to:"
        log_info "  Peripheral build: $PERIPHERAL_BUILD_LOG"
        log_info "  Central build: $CENTRAL_BUILD_LOG"
        log_info "  Central: $CENTRAL_RUN_LOG"
        log_info "  Peripheral: $PERIPHERAL_RUN_LOG"
        log_info "  Bumble Controllers: $BUMBLE_CONTROLLERS_LOG"
        exit 0
    fi
}

trap cleanup EXIT

# Main function
main() {
    log_info "Starting GAP Discovery test with Bumble controllers..."

    # Allocate ports
    if ! allocate_ports; then
        TEST_RESULT=1
        return 1
    fi

    # Start bumble controllers with 2 ports
    # Capture PID from stdout
    BUMBLE_CONTROLLER_PID=$(start_bumble_controllers \
        "$BUMBLE_CONTROLLERS_LOG" \
        "$HCI_PORT_0" "$HCI_PORT_1")

    if [[ $? -ne 0 ]] || [[ -z "$BUMBLE_CONTROLLER_PID" ]]; then
        log_error "Failed to start bumble controllers"
        TEST_RESULT=1
        return 1
    fi

    # Extract BD addresses
    if ! extract_bd_addresses "$BUMBLE_CONTROLLERS_LOG" \
        BD_ADDRESS_0 BD_ADDRESS_1; then
        TEST_RESULT=1
        return 1
    fi

    display_controller_info

    # Build and run tests
    if ! build_peer_device || ! build_local_device || \
       ! run_peer_device || ! run_local_device; then
        TEST_RESULT=1
        return 1
    fi

    # Wait for peripheral
    wait_peripheral_device 30 || true

    log_info "PASSED"
    TEST_RESULT=0
    return 0
}

main "$@"
