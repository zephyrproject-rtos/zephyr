#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name='sc_indicate'
test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_${test_name}_prj_conf"
simulation_id="${test_name}"
verbosity_level=2
EXECUTE_TIMEOUT=30

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6

wait_for_background_jobs
