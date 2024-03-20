#!/bin/env bash
# Copyright 2023 Codecoup
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="retry_on_sec_err"
verbosity_level=2
test_exe_d0="./bs_${BOARD}_$(guess_test_long_name)_client_prj_conf"
test_exe_d1="./bs_${BOARD}_$(guess_test_long_name)_server_prj_conf"

cd ${BSIM_OUT_PATH}/bin

printf "\n\n===== ATT retry on security error (auto security elevation) ======\n\n"

Execute "$test_exe_d0" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=test_client \
    -RealEncryption=1

Execute "$test_exe_d1" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=test_server \
    -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
