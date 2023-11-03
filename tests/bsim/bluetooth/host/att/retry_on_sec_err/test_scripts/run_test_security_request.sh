#!/bin/env bash
# Copyright 2023 Codecoup
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

simulation_id=$(basename "$0")
source "${bash_source_dir}/_env.sh"
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n==== ATT retry on security error (peripheral security request) ====\n\n"

Execute "$test_exe_d0" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=test_client_security_request \
    -RealEncryption=1

Execute "$test_exe_d1" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=test_server_security_request \
    -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
