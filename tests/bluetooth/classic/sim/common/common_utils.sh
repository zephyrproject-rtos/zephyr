#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Ensure this script is only loaded once
if [[ -n "${COMPILE_UTILS_LOADED:-}" ]]; then
    return 0
fi
COMPILE_UTILS_LOADED=1

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

# Function to find and copy zephyr.exe after build
extra_twister_out() {
    local twister_out_dir="$1"
    local test_name="$2"
    local exe_path
    local scenario
    local output_name

    # Find all zephyr.exe files
    exe_files=$(find "${twister_out_dir}" -name "zephyr.exe" -type f)

    if [[ -z "${exe_files}" ]]; then
        echo "Error: Could not find any zephyr.exe files" >&2
        exit 1
    fi

    # Loop through each found file
    while IFS= read -r exe_path; do
        if [[ -n "${exe_path}" ]]; then
            # Extract platform and scenario from path
            # Path format: ./twister-out/native_sim_native/host/tests/bluetooth/classic/sim/
            # gap_discovery/bluetooth.classic.gap.discovery.peripheral/zephyr/zephyr.exe
            scenario=$(echo "${exe_path}" | \
              sed -n 's|.*/'${test_name}'/\([^/]*\)/zephyr/zephyr.exe|\1|p')

            output_name="bt_classic_${BOARD_SANITIZED}_${scenario}.exe"

            cp "${exe_path}" "${BT_CLASSIC_BIN}/${output_name}"
        fi
    done <<< "${exe_files}"
}

# Function to create temporary directory with cleanup
# Args: prefix
# Returns: temp directory path on stdout
create_temp_dir() {
    local prefix="${1:-bt_test}"
    local temp_dir=$(mktemp -d "/tmp/${prefix}.XXXXXX")
    echo "$temp_dir"
}

# Function to parse test directories from a configuration file
# Args: config_file_path
# Returns: list of test directories (one per line) on stdout
parse_test_dirs() {
    local config_file="$1"

    if [[ ! -f "${config_file}" ]]; then
        echo "Error: Configuration file '${config_file}' not found"
        return 1
    fi

    # Read file, remove comments and empty lines
    # - Remove lines starting with # (comments)
    # - Remove leading/trailing whitespace
    # - Remove empty lines
    grep -v '^\s*#' "${config_file}" | \
        sed 's/^\s*//;s/\s*$//' | \
        grep -v '^$'
}

# Function to validate and resolve test directory paths
# Args: base_dir test_dir
# Returns: absolute path on stdout, or empty if invalid
resolve_test_dir() {
    local base_dir="$1"
    local test_dir="$2"
    local resolved_path

    # Check if path is absolute
    if [[ "${test_dir}" = /* ]]; then
        resolved_path="${test_dir}"
    else
        # Relative path - resolve relative to base_dir
        resolved_path="${base_dir}/${test_dir}"
    fi

    # Verify directory exists
    if [[ -d "${resolved_path}" ]]; then
        echo "${resolved_path}"
        return 0
    else
        echo "Warning: Test directory '${resolved_path}' does not exist"
        return 1
    fi
}

compile_scripts() {
    local config_file="$1"
    local base_dir="$2"
    local test_dirs
    local valid_dirs=()
    local script_dirs=()

    # Parse test directories from config file
    test_dirs=$(parse_test_dirs "${config_file}")
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to resolve config file '${config_file}'" >&2
        exit 1
    fi

    # Resolve and validate each directory
    while IFS= read -r test_dir; do
        if [[ -n "${test_dir}" ]]; then
            local resolved
            resolved=$(resolve_test_dir "${base_dir}" "${test_dir}")
            if [[ $? -ne 0 ]]; then
                echo "Error: Failed to resolve test directory '${test_dir}'" >&2
                exit 1
            fi

            if [[ $? -eq 0 && -n "${resolved}" ]]; then
                valid_dirs+=("${resolved}")
                # Search for compile.sh in the resolved directory
                local compile_sh="${resolved}/compile.sh"
                if [[ -f "${compile_sh}" ]]; then
                    script_dirs+=("${compile_sh}")
                fi
            fi
        fi
    done <<< "${test_dirs}"

    # Return all found compile.sh files
    echo "${script_dirs[@]}"
}

# Function to execute scripts in parallel (if GNU parallel available) or serially
# Args: script_list (space-separated or newline-separated)
#       max_jobs (optional, default: number of CPU cores)
# Returns: 0 if all scripts succeed, 1 if any fail
compile_parallel() {
    local scripts="$1"
    local max_jobs="${MAX_PARALLEL_JOBS:-$(($(nproc) / 2))}"
    local script_array=()
    local failed=0

    # Ensure at least 1 job
    if [[ ${max_jobs} -lt 1 ]]; then
        max_jobs=1
    fi

    # Convert scripts string to array
    while IFS= read -r script; do
        if [[ -n "${script}" ]]; then
            script_array+=("${script}")
        fi
    done <<< "$(echo "${scripts}" | tr ' ' '\n')"

    if [[ ${#script_array[@]} -eq 0 ]]; then
        echo "No scripts to execute"
        exit 1
    fi

    # Check if GNU parallel is available
    if command -v parallel &> /dev/null; then
        echo "Executing ${#script_array[@]} scripts in parallel (max ${max_jobs} jobs) " \
             "using GNU parallel..."

        printf '%s\n' "${script_array[@]}" | \
            parallel --jobs "${max_jobs}" \
                     --halt soon,fail=1 \
                     --line-buffer \
                     --tagstring '[{/}]' \
                     bash {}

        local exit_code=$?

        if [[ ${exit_code} -eq 0 ]]; then
            echo "All scripts completed successfully"
            return 0
        else
            echo "Some scripts failed"
            exit 1
        fi
    else
        echo "GNU parallel not found, executing ${#script_array[@]} scripts serially..."

        for script in "${script_array[@]}"; do
            echo "Executing: ${script}"
            if bash "${script}"; then
                echo "Success: ${script}"
            else
                echo "Failed: ${script}"
                failed=1
            fi
        done

        if [[ ${failed} -eq 0 ]]; then
            echo "All scripts completed successfully"
            return 0
        else
            echo "Some scripts failed"
            exit 1
        fi
    fi
}

run_scripts() {
    local config_file="$1"
    local base_dir="$2"
    local test_dirs
    local valid_dirs=()

    # Parse test directories from config file
    test_dirs=$(parse_test_dirs "${config_file}")
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to resolve config file '${config_file}'" >&2
        exit 1
    fi

    # Resolve and validate each directory
    while IFS= read -r test_dir; do
        if [[ -n "${test_dir}" ]]; then
            local resolved
            resolved=$(resolve_test_dir "${base_dir}" "${test_dir}")
            if [[ $? -ne 0 ]]; then
                echo "Error: Failed to resolve test directory '${test_dir}'" >&2
                exit 1
            fi

            if [[ $? -eq 0 && -n "${resolved}" ]]; then
                valid_dirs+=("${resolved}")
            fi
        fi
    done <<< "${test_dirs}"

    # Search for all *.sh files in tests_scripts subdirectory
    for dir in "${valid_dirs[@]}"; do
        local tests_scripts_dir="${dir}/tests_scripts"
        if [[ -d "${tests_scripts_dir}" ]]; then
            while IFS= read -r script_file; do
                if [[ -f "${script_file}" ]]; then
                    script_files+=("${script_file}")
                fi
            done < <(find "${tests_scripts_dir}" -name "*.sh" -type f)
        fi
    done

    # Return all found compile.sh files
    echo "${script_files[@]}"
}

# Temporary files for parallel execution
TMP_RESULTS_DIR="$BT_CLASSIC_SIM/tmp_results"
mkdir -p "$TMP_RESULTS_DIR"

# Function to run a single test script (used by parallel)
run_script_parallel() {
    local script="$1"
    local test_index=$2
    local total_tests="$3"
    local max_retries="${MAX_RUN_RETRIES:-0}"
    local attempt=0
    local exit_code=1
    local script_name=$(basename "$script" .sh)
    local test_id="${script_name}_${test_index}"
    local log_file="$BT_CLASSIC_SIM/${test_id//\//_}.log"
    local result_file="$TMP_RESULTS_DIR/${test_id//\//_}.result"

    # Record start time
    local start_time=$(date +%s%3N)

    while [[ $attempt -le $max_retries ]]; do

        # Run the script and capture output
        set +e
        (
            # Execute the script and redirect all output to log file
            if [[ $attempt -eq 0 ]]; then
                "$script" > "$log_file" 2>&1
            else
                "$script" >> "$log_file" 2>&1
            fi
            exit $?
        )
        exit_code=$?
        set -e

        if [[ $exit_code -eq 0 ]]; then
            break
        else
            echo "FAILED attempt $attempt for: $script (exit code: $exit_code)" | tee -a "$log_file"
            attempt=$((attempt + 1))

            if [[ $attempt -le $max_retries ]]; then
                # Optional: add a small delay between retries
                sleep 0.1
            fi
        fi
    done

    # Record end time
    local end_time=$(date +%s%3N)
    local duration=$((end_time - start_time))

    # Save result to file for later processing
    echo "script=$script" > "$result_file"
    echo "exit_code=$exit_code" >> "$result_file"
    echo "duration=$duration" >> "$result_file"
    echo "log_file=$log_file" >> "$result_file"
    echo "attempts=$attempt" >> "$result_file"

    return $exit_code
}

# Function to collect results from parallel execution
collect_parallel_results() {
    local results_file=$1
    local total_dur=$2
    local total_tests=0
    local tmp_res_file="tmp_compile_results.xml"
    local failures=0

    # Setup XML cleaning for special characters
    export CLEAN_XML="sed -E -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
                      -e 's/\"/\&quot;/g' -e 's/\x1b\[[0-9;]*[a-zA-Z]//g'"

    echo -n "" > "${tmp_res_file}"

    # Process all result files
    for result_file in $TMP_RESULTS_DIR/*.result; do
        [[ -f "$result_file" ]] || continue

        # Source the result file to get variables
        local script exit_code duration log_file attempts
        source "$result_file"

        total_tests=$((total_tests + 1))

        # Convert duration from milliseconds to seconds
        local duration_sec=$(awk "BEGIN {printf \"%.3f\", $duration/1000}")

        # Start test case XML
        echo "<testcase name=\"${script}\" time=\"${duration_sec}\">" >> "${tmp_res_file}"

        if [[ $exit_code -eq 0 ]]; then
            if [[ ${attempts:-0} -gt 0 ]]; then
                echo "${script} PASSED after ${attempts} retries (${duration}ms)"
            else
                echo "${script} PASSED (${duration}ms)"
            fi
        else
            failures=$((failures + 1))
            echo -e "\e[91m${script} FAILED after $((attempts)) " \
                    "attempts\e[39m (${duration}ms)" >&2
            cat "$log_file"

            # Add failure details to XML
            echo "<failure message=\"failed after $((attempts)) attempts\" " \
                 "type=\"failure\">" >> "${tmp_res_file}"
            if [[ -f "$log_file" ]]; then
                cat "$log_file" | eval $CLEAN_XML >> "${tmp_res_file}"
            fi
            echo "</failure>" >> "${tmp_res_file}"
        fi

        # Close test case XML
        echo "</testcase>" >> "${tmp_res_file}"
    done

    local total_dur_sec=$(awk "BEGIN {printf \"%.3f\", $total_dur/1000}")

    # Finalize XML output
    echo -e "</testsuite>\n</testsuites>\n" >> "${tmp_res_file}"
    echo -e "<testsuites>\n<testsuite errors=\"0\" failures=\"${failures}\" \
name=\"bt classic simulation\" skip=\"0\" tests=\"${total_tests}\" time=\"${total_dur_sec}\">" \
        | cat - "${tmp_res_file}" > "${results_file}"
    rm "${tmp_res_file}"

    # Clean up temporary results
    rm -rf "$TMP_RESULTS_DIR"
}

export -f run_script_parallel
export TMP_RESULTS_DIR
export BT_CLASSIC_SIM
export MAX_RUN_RETRIES

# Function to execute test scripts in parallel (if GNU parallel available) or serially
# Args: script_list (space-separated or newline-separated)
#       max_jobs (optional, default: number of CPU cores)
# Returns: 0 if all scripts succeed, 1 if any fail
run_parallel() {
    local scripts="$1"
    local max_jobs="${MAX_PARALLEL_JOBS:-$(($(nproc) / 2))}"
    local script_array=()
    local failed=0
    local start_time=$(date +%s%3N)

    # Ensure at least 1 job
    if [[ ${max_jobs} -lt 1 ]]; then
        max_jobs=1
    fi

    # Convert scripts string to array
    while IFS= read -r script; do
        if [[ -n "${script}" ]]; then
            script_array+=("${script}")
        fi
    done <<< "$(echo "${scripts}" | tr ' ' '\n')"

    if [[ ${#script_array[@]} -eq 0 ]]; then
        echo "No test scripts to execute"
        exit 1
    fi

    local n_cases=${#script_array[@]}
    local results_file="${RESULTS_FILE:-${BT_CLASSIC_SIM}/${BOARD_SANITIZED}/TestResults.xml}"
    local tmp_res_file="tmp_test_results.xml"

    # Create array of test indices
    local indices=()
    for i in $(seq 1 $n_cases); do
        indices+=("$i")
    done

    # Setup XML cleaning for special characters
    export CLEAN_XML="sed -E -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
                      -e 's/\"/\&quot;/g' -e 's/\x1b\[[0-9;]*[a-zA-Z]//g'"

    mkdir -p "$(dirname "${results_file}")"
    touch "${results_file}"
    echo "Attempting to run ${n_cases} test scripts (logging to $(realpath "${results_file}"))"

    # Check if GNU parallel is available
    if command -v parallel &> /dev/null; then
        echo "Executing ${#script_array[@]} test scripts in parallel (max ${max_jobs} jobs) " \
             "using GNU parallel..."

        set +e
        parallel --halt soon,fail=1 -j "$max_jobs" --line-buffer \
            run_script_parallel {1} {2} $n_cases \
            ::: "${script_array[@]}" \
            :::+ "${indices[@]}" > "${BT_CLASSIC_SIM}/parallel_execution.log" 2>&1
        local exit_code=$?
        set -e

        # Finalize XML output
        local end_time=$(date +%s%3N)
        local total_dur=$((end_time - start_time))

        collect_parallel_results "${results_file}" $total_dur

        if [[ ${exit_code} -eq 0 ]]; then
            return 0
        else
            exit 1
        fi
    else
        echo "GNU parallel not found, executing ${#script_array[@]} test scripts serially..."
        echo -n "" > "${tmp_res_file}"

        local i=0
        for script in "${script_array[@]}"; do

            local script_start=$(date +%s%3N)
            local script_name=$(basename "$script" .sh)
            local test_id="${script_name}_${i}"
            local log_file="$BT_CLASSIC_SIM/${test_id//\//_}.log"

            local max_retries="${MAX_RUN_RETRIES:-0}"
            local attempt=0
            local exit_code=1

            # Retry loop
            while [[ $attempt -le $max_retries ]]; do

                set +e
                if [[ $attempt -eq 0 ]]; then
                    bash "${script}" > "${log_file}" 2>&1
                else
                    bash "${script}" >> "${log_file}" 2>&1
                fi
                exit_code=$?
                set -e

                if [[ ${exit_code} -eq 0 ]]; then
                    break
                else
                    echo "FAILED attempt $attempt for: $script (exit code: $exit_code)" | \
                        tee -a "$log_file"
                    attempt=$((attempt + 1))

                    if [[ $attempt -le $max_retries ]]; then
                        sleep 0.1
                    fi
                fi
            done

            local end_time=$(date +%s%3N)
            local script_dur=$((end_time - script_start))
            local script_dur_sec=$(awk "BEGIN {printf \"%.3f\", $script_dur/1000}")

            # Execute script and capture output
            if [[ ${exit_code} -eq 0 ]]; then
                if [[ $attempt -gt 0 ]]; then
                    echo "${script} PASSED after ${attempt} retries (${script_dur}ms)"
                else
                    echo "${script} PASSED (${script_dur}ms)"
                fi
                echo "<testcase name=\"${script}\" time=\"${script_dur}\">" >> "${tmp_res_file}"
            else
                echo -e "\e[91m${script} FAILED after $((attempt)) " \
                        "attempts\e[39m (${script_dur}ms)" >&2
                cat "${log_file}"
                echo "<testcase name=\"${script}\" time=\"${script_dur_sec}\">" >> "${tmp_res_file}"
                echo "<failure message=\"failed after $((attempt)) attempts\" " \
                     "type=\"failure\">" >> "${tmp_res_file}"
                cat "${log_file}" | eval $CLEAN_XML >> "${tmp_res_file}"
                echo "</failure>" >> "${tmp_res_file}"
                failed=$((failed + 1))
            fi

            echo "</testcase>" >> "${tmp_res_file}"
            i=$((i + 1))
        done

        # Finalize XML output
        local end_time=$(date +%s%3N)
        local total_dur=$((end_time - start_time))
        local total_dur_sec=$(awk "BEGIN {printf \"%.3f\", $total_dur/1000}")

        echo -e "</testsuite>\n</testsuites>\n" >> "${tmp_res_file}"
        echo -e "<testsuites>\n<testsuite errors=\"0\" failures=\"${failed}\"\
 name=\"bt classic simulation\" skip=\"0\" tests=\"${n_cases}\" time=\"${total_dur_sec}\">" \
            | cat - "${tmp_res_file}" > "${results_file}"
        rm "${tmp_res_file}"

        if [[ ${failed} -eq 0 ]]; then
            return 0
        else
            exit 1
        fi
    fi
}
