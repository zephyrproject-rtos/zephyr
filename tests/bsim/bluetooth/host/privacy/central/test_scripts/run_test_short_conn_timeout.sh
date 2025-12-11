#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=100
verbosity_level=2

# Test that connection establishment times out when RPA
# timeout is shorter than the connection establishment timeout
simulation_id="test_central_connect_fails_with_short_rpa_timeout"

central_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$central_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 \
    -testid=central_connect_fails_with_short_rpa_timeout -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=1 -sim_length=60e6 $@

wait_for_background_jobs
