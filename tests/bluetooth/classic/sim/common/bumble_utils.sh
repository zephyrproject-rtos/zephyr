#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Bumble controller utilities
# Thread-safe: No global variables, all state passed via parameters

# Prevent multiple sourcing
if [[ -n "${BUMBLE_UTILS_LOADED:-}" ]]; then
    return 0
fi
readonly BUMBLE_UTILS_LOADED=1

# Start bumble controllers with variable number of ports
# Args: log_file port1@bd_address1 [port2@bd_address2] [port3@bd_address3] ...
# Returns: PID on stdout, exit code 0 on success
start_bumble_controllers() {
    if [[ $# -lt 2 ]]; then
        echo "start_bumble_controllers requires at least 2 arguments: "\
                  "<log_file> port1@bd_address1 [port2@bd_address2...]" >&2
        return 1
    fi

    local log_file="$1"
    shift  # Remove first argument, remaining are ports

    local ports=("$@")
    local num_ports=${#ports[@]}

    # Check if controllers.py exists
    local controllers_script="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common/controllers.py"
    if [[ ! -f "$controllers_script" ]]; then
        echo "controllers.py not found at $controllers_script" >&2
        return 1
    fi

    # Build transport arguments
    local transport_args=()
    for port in "${ports[@]}"; do
        transport_args+=("tcp-server:_:$port")
    done

    # Start bumble controllers asynchronously
    python "$controllers_script" "${transport_args[@]}" \
        > "$log_file" 2>&1 &

    local controller_pid=$!

    # Wait briefly to check if process started successfully
    sleep 2

    # Check if process is still running
    if ! kill -0 "$controller_pid" 2>/dev/null; then
        echo "Bumble controllers process failed to start or exited immediately" >&2
        cat "$log_file"
        return 1
    fi

    # Return PID on stdout
    echo "$controller_pid"
    return 0
}

# Stop bumble controllers and release port locks
# Args: pid
# Returns: 0 on success, 1 on failure
stop_bumble_controllers() {
    local pid="$1"

    if [[ -z "$pid" ]]; then
        return 0
    fi

    # Check if process exists before attempting to kill
    if ! kill -0 "$pid" 2>/dev/null; then
        return 0
    fi

    # Kill the process with extended timeout for cleanup
    if ! kill_process_graceful "$pid" "bumble controllers" 10; then
        echo "Warning: Failed to gracefully stop bumble controllers (PID: $pid)" >&2
        # Don't exit immediately, allow cleanup to continue
        return 1
    fi

    return 0
}
