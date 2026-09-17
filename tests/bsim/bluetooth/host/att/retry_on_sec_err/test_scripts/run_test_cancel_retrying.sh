#!/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_retry_on_sec_err_cancel_retrying"
verbosity_level=2
test_exe_d0="./bs_${BOARD_TS}_$(guess_test_long_name)_client_prj_conf"
test_exe_d1="./bs_${BOARD_TS}_$(guess_test_long_name)_server_prj_conf"

cd ${BSIM_OUT_PATH}/bin

printf "\n\n===== ATT cancel of a request awaiting a security retry ======\n\n"

Execute "$test_exe_d0" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=test_client_cancel_retrying \
    -RealEncryption=1

Execute "$test_exe_d1" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=test_server_cancel_retrying \
    -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
