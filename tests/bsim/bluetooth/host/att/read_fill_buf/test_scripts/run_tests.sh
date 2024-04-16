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
    echo -E "$@"
    timeout 30 $@ &
    process_ids="$process_ids $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

cd ${BSIM_OUT_PATH}/bin

Execute "$test_exe_d0" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=the_test \
    -RealEncryption=1

Execute "$test_exe_d1" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=the_test \
    -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=60e6 $@

for process_id in $process_ids; do
    wait $process_id || let "exit_code=$?"
done

if [ ${exit_code} -ne 0 ]; then
    exit_code=1
fi

exit $exit_code # the last exit code != 0
