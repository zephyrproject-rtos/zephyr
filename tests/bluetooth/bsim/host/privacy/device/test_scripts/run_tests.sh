#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

# Read variable definitions output by _env.sh
source "${bash_source_dir}/_env.sh"

process_ids=""
exit_code=0

function Execute() {
    if [ ! -f $1 ]; then
        echo -e "  \e[91m$(pwd)/$(basename $1) cannot be found (did you forget to\
 compile it?)\e[39m"
        exit 1
    fi
    timeout 30 $@ &
    process_ids="$process_ids $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

cd ${BSIM_OUT_PATH}/bin

TEST_ARGS=(
    "leg_conn_scan_active"
    "leg_conn_nscan_passive"
    "ext_nconn_scan_active"
    "ext_conn_nscan_passive"
)
TEST_ADDR_TYPE=("identity" "rpa")

sim_id_count=0
for args in ${TEST_ARGS[@]}; do
    args=($(echo "${args}" | sed 's/_/ /g'))

    use_ext_adv=0
    if [ "${args[0]}" == "ext" ]; then
        use_ext_adv=1
    fi

    connectable=0
    if [ "${args[1]}" == "conn" ]; then
        connectable=1
    fi

    scannable=0
    if [ "${args[2]}" == "scan" ]; then
        scannable=1
    fi

    use_active_scan=0
    if [ "${args[3]}" == "active" ]; then
        use_active_scan=1
    fi

    for addr_type in "${TEST_ADDR_TYPE[@]}"; do
        Execute "$test_exe" \
            -v=${verbosity_level} -s="${simulation_id}_${sim_id_count}" -d=0 -testid=central \
            -RealEncryption=1 -argstest sim-id=${sim_id_count} connection-test=${connectable} \
            active-scan=${use_active_scan} addr-type="${addr_type}"

        Execute "$test_exe" \
            -v=${verbosity_level} -s="${simulation_id}_${sim_id_count}" -d=1 -testid=peripheral \
            -RealEncryption=1 -argstest sim-id=${sim_id_count} addr-type="${addr_type}" \
            use-ext-adv=${use_ext_adv} scannable=${scannable} connectable=${connectable}

        Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}_${sim_id_count}" \
            -D=2 -sim_length=60e6 $@

        sim_id_count=$(( sim_id_count + 1 ))
    done
done

for process_id in $process_ids; do
    wait $process_id || let "exit_code=$?"
    if [ ${exit_code} -ne 0 ]; then
        exit_code=1
    fi
done

exit $exit_code # the last exit code != 0
