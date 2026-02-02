#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Prevent multiple sourcing
if [[ -n "${THREAD_UTILS_LOADED:-}" ]]; then
    return 0
fi
readonly THREAD_UTILS_LOADED=1

# Function to get the PID-specific array name
get_pids_array_name() {
    echo "ASYNC_PIDS_$$"
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

    # Check if process exists
    if ! kill -0 "$pid" 2>/dev/null; then
        return 0
    fi

    # Try graceful termination first
    if kill -TERM "$pid" 2>/dev/null; then
        local count=0
        while kill -0 "$pid" 2>/dev/null && [[ $count -lt $timeout ]]; do
            sleep 1
            ((count++))
        done

        # If still running after timeout, force kill
        if kill -0 "$pid" 2>/dev/null; then
            echo "Warning: $process_name (PID: $pid) did not respond to SIGTERM, " \
                 "sending SIGKILL" >&2
            if kill -KILL "$pid" 2>/dev/null; then
                sleep 1
            fi
        fi
    fi

    # Final check - if process still exists, try to kill process group
    if kill -0 "$pid" 2>/dev/null; then
        echo "Warning: Attempting to kill process group for $process_name (PID: $pid)" >&2
        # Kill the entire process group
        kill -KILL -"$pid" 2>/dev/null || true
        sleep 1
    fi

    # Verify process is gone
    if kill -0 "$pid" 2>/dev/null; then
        echo "Error: $process_name (PID: $pid) still running after kill attempts" >&2
        return 1
    fi

    return 0
}

# Function to execute command asynchronously
# Args: command_line
# Returns: PID of the background process
execute_async() {
    local command_line="$*"
    local array_name=$(get_pids_array_name)

    if [[ -z "$command_line" ]]; then
        echo "Error: No command provided to execute_async" >&2
        return 1
    fi

    eval "$command_line" &
    local pid=$!

    if ! kill -0 "$pid" 2>/dev/null; then
        echo "Error: Failed to start async process" >&2
        return 1
    fi

    # Save PID to process-specific array
    eval "${array_name}+=('$pid')"

    echo "$pid"
    return 0
}

# Function to wait for all async processes to complete
# Args: none
# Returns: exit code of first failed process, or 0 if all succeeded
wait_for_async_jobs() {
    local exit_code=0
    local array_name=$(get_pids_array_name)
    local pids_copy

    # Get copy of PIDs array
    eval "pids_copy=(\"\${${array_name}[@]}\")"

    # Wait for all processes
    for pid in "${pids_copy[@]}"; do
        wait $pid || let "exit_code=$?"
    done

    # Clear the array after waiting
    eval "${array_name}=()"

    [ $exit_code -eq 0 ] || exit $exit_code
}

# Function to kill all async processes
# Args: timeout (optional, default: 5)
# Returns: 0 on success, 1 on failure
kill_all_async() {
    local timeout="${1:-5}"
    local failed=0
    local array_name=$(get_pids_array_name)
    local pids_copy

    # Get copy of PIDs array
    eval "pids_copy=(\"\${${array_name}[@]}\")"

    # Kill all processes
    for pid in "${pids_copy[@]}"; do
        if ! kill_process_graceful "$pid" "async_process" "$timeout"; then
            failed=1
        fi
    done

    # Clear the array
    eval "${array_name}=()"

    return $failed
}
