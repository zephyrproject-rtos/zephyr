#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Ensure this script is only loaded once
if [[ -n "${PORT_UTILS_LOADED:-}" ]]; then
    return 0
fi
PORT_UTILS_LOADED=1

# TCP port allocation settings
TCP_PORT_RANGE_START="${TCP_PORT_RANGE_START:-9000}"
TCP_PORT_RANGE_END="${TCP_PORT_RANGE_END:-19999}"  # Increased range for parallel execution
TCP_PORT_LOCK_DIR="${BT_CLASSIC_BIN}/.port_locks"
TCP_PORT_REGISTRY="${BT_CLASSIC_BIN}/.port_registry"

# Create lock directory if it doesn't exist
mkdir -p "${TCP_PORT_LOCK_DIR}"

# Clean up stale locks (older than 60 seconds)
cleanup_stale_locks() {
    local lock_file="${TCP_PORT_REGISTRY}.lock"

    if [[ ! -d "${lock_file}" ]]; then
        return 0
    fi

    # Check if lock is stale (older than 60 seconds)
    if command -v stat &>/dev/null; then
        local current_time
        local lock_time
        local lock_age

        current_time=$(date +%s 2>/dev/null) || return 0
        lock_time=$(stat -c %Y "${lock_file}" 2>/dev/null || \
            stat -f %m "${lock_file}" 2>/dev/null) || return 0
        lock_age=$((current_time - lock_time))

        if [[ ${lock_age} -gt 60 ]]; then
            echo "WARNING: Removing stale lock (${lock_age}s old): ${lock_file}" >&2
            rmdir "${lock_file}" 2>/dev/null || rm -rf "${lock_file}" 2>/dev/null || true
        fi
    fi
}

# Initialize port registry file with lock
initialize_port_registry() {
    # If registry already exists, no need to lock
    if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
        return 0
    fi

    local lock_file="${TCP_PORT_REGISTRY}.lock"

    # Clean up any stale locks first
    cleanup_stale_locks

    # Acquire lock with exponential backoff
    local timeout=30
    local elapsed=0
    local backoff=1

    while ! mkdir "${lock_file}" 2>/dev/null; do
        # Check again if file was created by another process
        if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
            return 0
        fi

        sleep 0.0${backoff}
        elapsed=$((elapsed + backoff))
        backoff=$((backoff < 9 ? backoff + 1 : 9))  # Max 0.9s backoff

        if [[ ${elapsed} -ge ${timeout} ]]; then
            echo "ERROR: Failed to acquire lock for port registry initialization " \
                 "after ${timeout}s" >&2
            echo "ERROR: Lock file: ${lock_file}" >&2
            ls -la "${TCP_PORT_LOCK_DIR}" >&2 2>/dev/null || true
            return 1
        fi

        # Cleanup stale locks every 5 seconds
        if [[ $((elapsed % 5)) -eq 0 ]]; then
            cleanup_stale_locks
        fi
    done

    # Double-check: another process might have created it while we were waiting
    if [[ ! -f "${TCP_PORT_REGISTRY}" ]]; then
        touch "${TCP_PORT_REGISTRY}"
    fi

    # Release lock
    rmdir "${lock_file}"
    return 0
}

# Check if a port is available
is_port_available() {
    local port=$1

    # First check registry (faster, no system call)
    if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
        grep -q "^${port}$" "${TCP_PORT_REGISTRY}" && return 1
    fi

    # Then check if port is in use by the system
    if command -v ss &>/dev/null; then
        ss -tuln 2>/dev/null | grep -q ":${port} " && return 1
    elif command -v netstat &>/dev/null; then
        netstat -tuln 2>/dev/null | grep -q ":${port} " && return 1
    else
        # Fallback: try to bind to the port
        timeout 0.5 bash -c "echo >/dev/tcp/127.0.0.1/${port}" 2>/dev/null && return 1
    fi

    return 0
}

# Allocate a TCP port with thread-safe locking
# Usage: allocate_tcp_port [preferred_port]
# Returns: allocated port number or exits with code 1 on failure
allocate_tcp_port() {
    local preferred_port=${1:-}
    local allocated_port=""

    # Initialize registry if needed
    initialize_port_registry || return 1

    # Use flock for more reliable locking if available
    if command -v flock &>/dev/null; then
        local lock_file="${TCP_PORT_REGISTRY}.flock"
        touch "${lock_file}"

        # Acquire exclusive lock with 60 second timeout
        exec 200>"${lock_file}"
        if ! flock -x -w 60 200; then
            echo "ERROR: Failed to acquire lock for port allocation (timeout after 60s)" >&2
            echo "ERROR: Lock file: ${lock_file}" >&2
            echo "ERROR: Allocated ports: $(list_allocated_ports 2>/dev/null | wc -l)" >&2
            exec 200>&-
            return 1
        fi

        # Critical section - port allocation
        {
            # Try preferred port first
            if [[ -n "${preferred_port}" ]]; then
                if [[ ${preferred_port} -ge ${TCP_PORT_RANGE_START} ]] && \
                   [[ ${preferred_port} -le ${TCP_PORT_RANGE_END} ]]; then
                    if is_port_available "${preferred_port}"; then
                        allocated_port="${preferred_port}"
                        echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                    fi
                fi
            fi

            # Find next available port with random start to reduce contention
            if [[ -z "${allocated_port}" ]]; then
                local start_port=$((TCP_PORT_RANGE_START + \
                    RANDOM % (TCP_PORT_RANGE_END - TCP_PORT_RANGE_START)))
                local port

                # Try from random start to end
                for port in $(seq ${start_port} ${TCP_PORT_RANGE_END}); do
                    if is_port_available "${port}"; then
                        allocated_port="${port}"
                        echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                        break
                    fi
                done

                # If not found, try from start to random start
                if [[ -z "${allocated_port}" ]]; then
                    for port in $(seq ${TCP_PORT_RANGE_START} $((start_port - 1))); do
                        if is_port_available "${port}"; then
                            allocated_port="${port}"
                            echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                            break
                        fi
                    done
                fi
            fi
        }

        # Release lock (automatically released when fd closes)
        exec 200>&-
    else
        # Fallback to mkdir-based locking
        cleanup_stale_locks

        local lock_file="${TCP_PORT_REGISTRY}.lock"
        local timeout=60
        local elapsed=0
        local backoff=1
        local max_backoff=10

        while ! mkdir "${lock_file}" 2>/dev/null; do
            sleep 0.0${backoff}
            elapsed=$((elapsed + 1))

            # Exponential backoff
            if [[ $((elapsed % 10)) -eq 0 ]] && [[ ${backoff} -lt ${max_backoff} ]]; then
                backoff=$((backoff + 1))
            fi

            # Periodic stale lock cleanup
            if [[ $((elapsed % 50)) -eq 0 ]]; then
                cleanup_stale_locks
            fi

            if [[ ${elapsed} -ge $((timeout * 10)) ]]; then
                echo "ERROR: Failed to acquire lock for port allocation " \
                     "(timeout after ${timeout}s)" >&2
                echo "ERROR: Lock file: ${lock_file}" >&2
                echo "ERROR: Allocated ports: $(list_allocated_ports 2>/dev/null | wc -l)" >&2
                return 1
            fi
        done

        # Critical section - port allocation
        {
            # Try preferred port first
            if [[ -n "${preferred_port}" ]]; then
                if [[ ${preferred_port} -ge ${TCP_PORT_RANGE_START} ]] && \
                   [[ ${preferred_port} -le ${TCP_PORT_RANGE_END} ]]; then
                    if is_port_available "${preferred_port}"; then
                        allocated_port="${preferred_port}"
                        echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                    fi
                fi
            fi

            # Find next available port with random start
            if [[ -z "${allocated_port}" ]]; then
                local start_port=$((TCP_PORT_RANGE_START + \
                    RANDOM % (TCP_PORT_RANGE_END - TCP_PORT_RANGE_START)))
                local port

                # Try from random start to end
                for port in $(seq ${start_port} ${TCP_PORT_RANGE_END}); do
                    if is_port_available "${port}"; then
                        allocated_port="${port}"
                        echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                        break
                    fi
                done

                # If not found, try from start to random start
                if [[ -z "${allocated_port}" ]]; then
                    for port in $(seq ${TCP_PORT_RANGE_START} $((start_port - 1))); do
                        if is_port_available "${port}"; then
                            allocated_port="${port}"
                            echo "${allocated_port}" >> "${TCP_PORT_REGISTRY}"
                            break
                        fi
                    done
                fi
            fi
        }

        # Release lock
        rmdir "${lock_file}"
    fi

    # Return result or exit on failure
    if [[ -n "${allocated_port}" ]]; then
        echo "${allocated_port}"
        return 0
    else
        echo "ERROR: No available ports in range ${TCP_PORT_RANGE_START}-${TCP_PORT_RANGE_END}" >&2
        echo "ERROR: Total ports in range: $((TCP_PORT_RANGE_END - TCP_PORT_RANGE_START + 1))" >&2
        echo "ERROR: Allocated ports: $(list_allocated_ports 2>/dev/null | wc -l)" >&2
        return 1
    fi
}

# Release a TCP port
# Usage: release_tcp_port <port>
release_tcp_port() {
    local port=$1

    if [[ -z "${port}" ]]; then
        echo "ERROR: Port number required" >&2
        return 1
    fi

    # Validate port is in valid range
    if [[ ${port} -lt ${TCP_PORT_RANGE_START} ]] || \
       [[ ${port} -gt ${TCP_PORT_RANGE_END} ]]; then
        echo "ERROR: Port ${port} out of range ${TCP_PORT_RANGE_START}-${TCP_PORT_RANGE_END}" >&2
        return 1
    fi

    # Use flock if available
    if command -v flock &>/dev/null; then
        local lock_file="${TCP_PORT_REGISTRY}.flock"
        touch "${lock_file}"

        exec 200>"${lock_file}"
        if ! flock -x -w 30 200; then
            echo "ERROR: Failed to acquire lock for port release (timeout)" >&2
            exec 200>&-
            return 1
        fi

        # Critical section - remove port from registry
        {
            if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
                local temp_file="${TCP_PORT_REGISTRY}.tmp.${BASHPID:-$$}"
                grep -v "^${port}$" "${TCP_PORT_REGISTRY}" > "${temp_file}" 2>/dev/null || true
                mv "${temp_file}" "${TCP_PORT_REGISTRY}"
            fi
        }

        exec 200>&-
    else
        # Fallback to mkdir-based locking
        local lock_file="${TCP_PORT_REGISTRY}.lock"

        cleanup_stale_locks

        # Acquire lock with timeout
        local timeout=30
        local elapsed=0
        while ! mkdir "${lock_file}" 2>/dev/null; do
            sleep 0.1
            elapsed=$((elapsed + 1))
            if [[ ${elapsed} -ge $((timeout * 10)) ]]; then
                echo "ERROR: Failed to acquire lock for port release (timeout)" >&2
                return 1
            fi

            # Periodic cleanup
            if [[ $((elapsed % 50)) -eq 0 ]]; then
                cleanup_stale_locks
            fi
        done

        # Critical section - remove port from registry
        {
            if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
                local temp_file="${TCP_PORT_REGISTRY}.tmp.${BASHPID:-$$}"
                grep -v "^${port}$" "${TCP_PORT_REGISTRY}" > "${temp_file}" 2>/dev/null || true
                mv "${temp_file}" "${TCP_PORT_REGISTRY}"
            fi
        }

        # Release lock
        rmdir "${lock_file}"
    fi

    return 0
}

# Clean up all allocated ports (useful for cleanup on exit)
# Usage: cleanup_all_ports
cleanup_all_ports() {
    # Use flock if available
    if command -v flock &>/dev/null; then
        local lock_file="${TCP_PORT_REGISTRY}.flock"
        touch "${lock_file}"

        exec 200>"${lock_file}"
        if ! flock -x -w 30 200; then
            echo "WARNING: Failed to acquire lock for port cleanup (timeout)" >&2
            exec 200>&-
            return 1
        fi

        # Critical section - clear registry
        {
            if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
                > "${TCP_PORT_REGISTRY}"
            fi
        }

        exec 200>&-
    else
        # Fallback to mkdir-based locking
        local lock_file="${TCP_PORT_REGISTRY}.lock"

        cleanup_stale_locks

        # Acquire lock with timeout
        local timeout=30
        local elapsed=0
        while ! mkdir "${lock_file}" 2>/dev/null; do
            sleep 0.1
            elapsed=$((elapsed + 1))
            if [[ ${elapsed} -ge $((timeout * 10)) ]]; then
                echo "WARNING: Failed to acquire lock for port cleanup (timeout)" >&2
                return 1
            fi
        done

        # Critical section - clear registry
        {
            if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
                > "${TCP_PORT_REGISTRY}"
            fi
        }

        # Release lock
        rmdir "${lock_file}"
    fi

    return 0
}

# Get list of currently allocated ports
# Usage: list_allocated_ports
list_allocated_ports() {
    if [[ -f "${TCP_PORT_REGISTRY}" ]]; then
        cat "${TCP_PORT_REGISTRY}"
    fi
}
