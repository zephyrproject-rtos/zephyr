#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Bumble controller utilities
# Thread-safe: No global variables, all state passed via parameters

# Prevent multiple sourcing
if [[ -n "${BUMBLE_UTILS_LOADED:-}" ]]; then
    return 0
fi
readonly BUMBLE_UTILS_LOADED=1

# Ensure Bumble Python package is installed at the required version.
#
# This is needed by controllers.py (and related test scripts) which rely on Bumble.
#
# Behavior:
# - If Bumble is not installed: install Bumble==0.0.220
# - If Bumble is installed but version differs: uninstall, then install Bumble==0.0.220
#
# Args: [python_exe] (optional, default: python)
# Returns: 0 on success, 1 on failure
ensure_bumble_version() {
    local py="${1:-python}"
    local required_version="0.0.220"

    if ! command -v "$py" >/dev/null 2>&1; then
        echo "Error: '$py' not found in PATH; cannot verify/install Bumble" >&2
        return 1
    fi

    local installed_version=""

    # Query installed version (empty if not installed)
    installed_version=$("$py" -c "
import sys
try:
    import importlib.metadata as m
except Exception:
    m = None

def ver(name):
    if m is not None:
        try:
            return m.version(name)
        except Exception:
            return ''
    try:
        import pkg_resources
        try:
            return pkg_resources.get_distribution(name).version
        except Exception:
            return ''
    except Exception:
        return ''

print(ver('bumble'))
" 2>/dev/null)

    if [[ "$installed_version" == "$required_version" ]]; then
        return 0
    fi

    if [[ -z "$installed_version" ]]; then
        echo "Bumble not installed; installing Bumble==$required_version"
    else
        echo "Bumble version mismatch (installed: $installed_version, " \
             "required: $required_version); reinstalling"
        "$py" -m pip uninstall -y bumble >/dev/null 2>&1 || true
    fi

    # Ensure pip is available
    if ! "$py" -m pip --version >/dev/null 2>&1; then
        echo "Error: pip not available for '$py'; cannot install Bumble" >&2
        return 1
    fi

    # Install the pinned version.
    if ! "$py" -m pip install --no-cache-dir "Bumble==$required_version"; then
        echo "Error: Failed to install Bumble==$required_version" >&2
        return 1
    fi

    return 0
}

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

    # Ensure Bumble is present at the expected version before starting controllers.
    if ! ensure_bumble_version "python"; then
        return 1
    fi

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

    # Wait for each TCP server port to become ready (max 10 s per port).
    local timeout_s=10
    local poll_interval_ms=200
    local timeout_ms=$(( timeout_s * 1000 ))
    for port_spec in "${ports[@]}"; do
        # port_spec format: <port>@<bd_address>  -- extract the numeric port
        local port="${port_spec%%@*}"
        local elapsed_ms=0
        local ready=0
        while (( elapsed_ms < timeout_ms )); do
            # Check if process is still alive before polling
            if ! kill -0 "$controller_pid" 2>/dev/null; then
                echo "Bumble controllers process exited unexpectedly while waiting for port $port" \
                     >&2
                cat "$log_file"
                return 1
            fi
            # Check whether the TCP server port is in LISTEN state.
            # Use ss/netstat first (no real connection needed), consistent
            # with is_port_available() in port_utils.sh.  Fall back to
            # /dev/tcp only when neither tool is available.
            local port_listening=0
            if command -v ss &>/dev/null; then
                ss -tuln 2>/dev/null | grep -q ":${port} " && port_listening=1
            elif command -v netstat &>/dev/null; then
                netstat -tuln 2>/dev/null | grep -q ":${port} " && port_listening=1
            else
                (echo "" > /dev/tcp/127.0.0.1/"${port}") 2>/dev/null && port_listening=1
            fi
            if [[ "${port_listening}" -eq 1 ]]; then
                ready=1
                break
            fi
            sleep 0.2  # must match poll_interval_ms (200 ms = 0.2 s)
            (( elapsed_ms += poll_interval_ms ))
        done
        if [[ "$ready" -eq 0 ]]; then
            echo "Timeout: TCP server on port $port did not become ready within ${timeout_s}s" >&2
            cat "$log_file"
            kill "$controller_pid" 2>/dev/null || true
            return 1
        fi
    done

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
