#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_notify_enhanced_unenhanced"
test_exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./${test_exe} \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client_unenhanced -RealEncryption=1

Execute ./${test_exe} \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server_enhanced -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
