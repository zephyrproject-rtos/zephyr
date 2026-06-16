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
# Args: command [args...]
# Returns: PID of the background process
execute_async() {
    local array_name
    array_name=$(get_pids_array_name)

    if [[ $# -eq 0 ]]; then
        echo "Error: No command provided to execute_async" >&2
        return 1
    fi

    # Print the command line before execution.
    echo "execute_async: $*" >&2

    # Execute the command without eval to preserve argument boundaries.
    # This avoids unexpected word splitting when args contain spaces/special chars.
    "$@" &
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
# Behavior:
#   1) Wait for ANY job to exit.
#   2) If it exits non-zero, terminate the remaining running jobs.
#   3) If it exits zero, remove it from the wait set and keep waiting.
#   4) Return 0 only if all jobs exit 0; otherwise return the first non-zero.
# Args: none
# Returns: exit code of first failed process, or 0 if all succeeded
wait_for_async_jobs() {
    local exit_code=0
    local array_name
    array_name=$(get_pids_array_name)

    local -a remaining
    eval "remaining=(\"\${${array_name}[@]}\")"

    # No jobs -> success
    if [[ ${#remaining[@]} -eq 0 ]]; then
        return 0
    fi

    while [[ ${#remaining[@]} -gt 0 ]]; do
        local exited_pid=""
        local rc=0

        # Bash 5+: wait for any to finish and capture which PID exited.
        if wait -n -p exited_pid "${remaining[@]}"; then
            rc=0
        else
            rc=$?
        fi

        # If we couldn't determine which PID, fall back to scanning.
        if [[ -z "$exited_pid" ]]; then
            for pid in "${remaining[@]}"; do
                if ! kill -0 "$pid" 2>/dev/null; then
                    exited_pid="$pid"
                    wait "$pid" 2>/dev/null
                    rc=$?
                    break
                fi
            done
        fi

        # Remove exited_pid from remaining
        if [[ -n "$exited_pid" ]]; then
            local -a new_remaining=()
            local pid
            for pid in "${remaining[@]}"; do
                if [[ "$pid" != "$exited_pid" ]]; then
                    new_remaining+=("$pid")
                fi
            done
            remaining=("${new_remaining[@]}")
        fi

        # On first failure: kill the rest and record the error.
        if [[ $rc -ne 0 && $exit_code -eq 0 ]]; then
            exit_code=$rc
            local pid
            for pid in "${remaining[@]}"; do
                kill_process_graceful "$pid" "async_process" 5 || true
            done

            # Drain the remaining children to avoid zombies.
            for pid in "${remaining[@]}"; do
                wait "$pid" 2>/dev/null || true
            done
            break
        fi
    done

    # Clear the array after waiting
    eval "${array_name}=()"

    return $exit_code
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
