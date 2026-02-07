#!/usr/bin/env bash
#
# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="tests_bsim_bluetooth_host_ext_adv_info_privacy"
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"
simulation_id=${test_name}
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=adv_info

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=10e6 $@

wait_for_background_jobs
