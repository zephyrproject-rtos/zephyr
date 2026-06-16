#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Ensure this script is only loaded once
if [[ -n "${COMMON_UTILS_LOADED:-}" ]]; then
    return 0
fi
COMMON_UTILS_LOADED=1

BOARD=${BOARD:-native_sim}

# Function to sanitize board name by replacing slashes with underscores
sanitize_board_name() {
    echo "${1//\//_}"
}

# Sanitize board name
BOARD_SANITIZED=$(sanitize_board_name "$BOARD")

COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

# Source utilities
source "$COMMON_DIR/parameter_utils.sh"
source "$COMMON_DIR/port_utils.sh"
source "$COMMON_DIR/thread_utils.sh"
source "$COMMON_DIR/bumble_utils.sh"

# Function to find zephyr.exe after build and return its full path
extra_twister_out() {
    local twister_out_dir="$1"
    local test_name="$2"

    # Find all zephyr.exe files
    local exe_files
    exe_files=$(find "${twister_out_dir}" -name "zephyr.exe" -type f)

    if [[ -z "${exe_files}" ]]; then
        echo "Error: Could not find any zephyr.exe files" >&2
        exit 1
    fi

    # Prefer zephyr.exe paths that include test_name as a full directory component.
    # This prevents e.g. "bluetooth.classic.sim.l2cap.basic.mode_optional" matching
    # "bluetooth.classic.sim.l2cap.basic".
    local matched_exe_files
    matched_exe_files=$(
        printf '%s\n' "${exe_files}" |
            grep -F -- "/${test_name}/" || true
    )

    if [[ -z "${matched_exe_files}" ]]; then
        echo "Error: Found zephyr.exe files, but none of their paths contain test_name=" \
             "'${test_name}'" >&2
        echo "Searched under: ${twister_out_dir}" >&2
        echo "Candidates:" >&2
        echo "${exe_files}" >&2
        exit 1
    fi

    # If multiple candidates match, select only the newest one and warn
    local n_matches
    n_matches=$(printf '%s\n' "${matched_exe_files}" | grep -c '^' || true)

    local selected_exe
    selected_exe=$(printf '%s\n' "${matched_exe_files}" | head -n 1)

    if [[ "${n_matches}" -gt 1 ]]; then
        # Try to select newest by mtime; if ls sorting fails, keep the first line
        local newest_by_mtime
        newest_by_mtime=$(
            printf '%s\n' "${matched_exe_files}" |
                xargs -d '\n' ls -1t 2>/dev/null |
                head -n 1 || true
        )
        if [[ -n "${newest_by_mtime}" ]]; then
            selected_exe="${newest_by_mtime}"
        fi

        echo "Warning: Found ${n_matches} zephyr.exe files matching " \
             "test_name='${test_name}'." >&2
        echo "Warning: Using newest: ${selected_exe}" >&2
    fi

    # Log and return full path to selected zephyr.exe
    echo "Info: Using zephyr.exe: ${selected_exe}" >&2
    printf '%s\n' "${selected_exe}"
}
