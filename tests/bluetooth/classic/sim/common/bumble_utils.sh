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

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_utils.sh"

# Start bumble controllers with variable number of ports
# Args: log_file port1 [port2] [port3] ...
# Returns: PID on stdout, exit code 0 on success
start_bumble_controllers() {
    if [[ $# -lt 2 ]]; then
        log_error "start_bumble_controllers requires at least 2 arguments: "\
                  "<log_file> <port1> [port2...]"
        return 1
    fi

    local log_file="$1"
    shift  # Remove first argument, remaining are ports

    local ports=("$@")
    local num_ports=${#ports[@]}

    log_info "Starting bumble controllers with $num_ports port(s)..."

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

    # Build transport arguments
    local transport_args=()
    for port in "${ports[@]}"; do
        transport_args+=("tcp-server:_:$port")
        log_info "  Port: $port"
    done

    # Start bumble controllers asynchronously
    python "$controllers_script" "${transport_args[@]}" \
        > "$log_file" 2>&1 &

    local controller_pid=$!

    log_info "Started bumble controllers with PID: $controller_pid"

    # Wait briefly to check if process started successfully
    sleep 2

    # Check if process is still running
    if ! kill -0 "$controller_pid" 2>/dev/null; then
        log_error "Bumble controllers process failed to start or exited immediately"
        cat "$log_file"
        return 1
    fi

    log_info "Bumble controllers started successfully"

    # Wait for controllers to initialize
    log_info "Waiting for bumble controllers to initialize..."
    sleep 3

    # Return PID on stdout
    echo "$controller_pid"
    return 0
}

# Extract BD addresses from bumble controller log
# Args: log_file var_name_0 var_name_1 [var_name_2] ...
# Returns: 0 on success, 1 on failure
# Side effect: Sets variables in caller's scope via nameref
extract_bd_addresses() {
    if [[ $# -lt 3 ]]; then
        log_error "extract_bd_addresses requires at least 3 arguments: " \
                  "<log_file> <var_name_0> <var_name_1> [var_name_2...]"
        return 1
    fi

    local log_file="$1"
    shift  # Remove first argument

    log_info "Extracting BD addresses from bumble controller log..."

    if [[ ! -f "$log_file" ]]; then
        log_error "Bumble controller log file not found: $log_file"
        return 1
    fi

    local index=0
    local extraction_failed=false

    # Extract BD address for each controller
    for var_name in "$@"; do
        local bd_addr=""

        # Try sed method first
        bd_addr=$(grep "HCI${index} BD Address" "$log_file" | sed -r 's/\x1B\[[0-9;]*[mK]//g' | \
                  grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)

        # If sed failed, try perl method
        if [[ -z "$bd_addr" ]]; then
            log_info "Attempting alternative extraction method with perl for HCI${index}..."
            bd_addr=$(grep "HCI${index} BD Address" "$log_file" | perl -pe 's/\e\[[0-9;]*m//g' | \
                      grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)
        fi

        if [[ -z "$bd_addr" ]]; then
            log_error "Failed to extract BD address for HCI${index}"
            extraction_failed=true
        else
            # Use nameref to set the variable in caller's scope
            local -n bd_addr_ref=$var_name
            bd_addr_ref="$bd_addr"
            log_info "HCI${index} BD Address: $bd_addr"
        fi

        ((index++))
    done

    if [[ "$extraction_failed" == true ]]; then
        log_error "Failed to extract one or more BD addresses from log file"
        log_info "Log file content:"
        cat "$log_file"
        return 1
    fi

    return 0
}

# Extract BD addresses into an array
# Args: log_file array_name num_controllers
# Returns: 0 on success, 1 on failure
# Side effect: Populates array in caller's scope via nameref
extract_bd_addresses_array() {
    if [[ $# -ne 3 ]]; then
        log_error "extract_bd_addresses_array requires 3 arguments: " \
                  "<log_file> <array_name> <num_controllers>"
        return 1
    fi

    local log_file="$1"
    local -n array_ref=$2
    local num_controllers=$3

    log_info "Extracting $num_controllers BD addresses from bumble controller log..."

    if [[ ! -f "$log_file" ]]; then
        log_error "Bumble controller log file not found: $log_file"
        return 1
    fi

    array_ref=()
    local extraction_failed=false

    for ((i=0; i<num_controllers; i++)); do
        local bd_addr=""

        # Try sed method first
        bd_addr=$(grep "HCI${i} BD Address" "$log_file" | sed -r 's/\x1B\[[0-9;]*[mK]//g' | \
                  grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)

        # If sed failed, try perl method
        if [[ -z "$bd_addr" ]]; then
            log_info "Attempting alternative extraction method with perl for HCI${i}..."
            bd_addr=$(grep "HCI${i} BD Address" "$log_file" | perl -pe 's/\e\[[0-9;]*m//g' | \
                      grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | head -1)
        fi

        if [[ -z "$bd_addr" ]]; then
            log_error "Failed to extract BD address for HCI${i}"
            extraction_failed=true
        else
            array_ref+=("$bd_addr")
            log_info "HCI${i} BD Address: $bd_addr"
        fi
    done

    if [[ "$extraction_failed" == true ]]; then
        log_error "Failed to extract one or more BD addresses from log file"
        log_info "Log file content:"
        cat "$log_file"
        return 1
    fi

    return 0
}

# Stop bumble controllers and release port locks
# Args: pid port1 [port2] [port3] ...
# Returns: 0 on success, 1 on failure
stop_bumble_controllers() {
    local pid="$1"
    shift  # Remove first argument, remaining are ports
    local ports=("$@")

    if [[ -z "$pid" ]]; then
        log_warn "No bumble controller PID provided"

        # Still try to release port locks if provided
        if [[ ${#ports[@]} -gt 0 ]]; then
            log_info "Releasing port locks..."
            for port in "${ports[@]}"; do
                release_port_lock "$port"
            done
        fi

        return 0
    fi

    # Kill the process
    local result=0
    if ! kill_process_graceful "$pid" "bumble controllers" 3; then
        result=1
    fi

    # Release all port locks
    if [[ ${#ports[@]} -gt 0 ]]; then
        log_info "Releasing ${#ports[@]} port lock(s)..."
        for port in "${ports[@]}"; do
            log_info "  Releasing port: $port"
            release_port_lock "$port"
        done
    else
        log_warn "No ports provided to release locks"
    fi

    return $result
}

# Wait for bumble controllers log to contain expected pattern
# Args: log_file pattern timeout
# Returns: 0 if pattern found, 1 on timeout
wait_for_log_pattern() {
    local log_file="$1"
    local pattern="$2"
    local timeout="${3:-10}"

    log_info "Waiting for pattern '$pattern' in log file (timeout: ${timeout}s)..."

    local count=0
    while [[ $count -lt $timeout ]]; do
        if [[ -f "$log_file" ]] && grep -q "$pattern" "$log_file" 2>/dev/null; then
            log_info "Pattern found after ${count}s"
            return 0
        fi
        sleep 1
        ((count++))
    done

    log_error "Pattern '$pattern' not found after ${timeout}s timeout"
    return 1
}
