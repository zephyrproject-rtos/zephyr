#!/usr/bin/env bash

# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="resume2"
dut_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_dut_prj_conf"
connecter_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_connecter_prj_conf"
connectable_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_connectable_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -RealEncryption=1

Execute "$connecter_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -RealEncryption=1

Execute "$connecter_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=2 -RealEncryption=1

Execute "$connecter_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=3 -RealEncryption=1

Execute "$connectable_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=4 -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=5 -sim_length=60e6 $@

wait_for_background_jobs
