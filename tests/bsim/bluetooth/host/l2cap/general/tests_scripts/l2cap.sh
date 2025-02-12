#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# EATT test
simulation_id="l2cap_ecred"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=43

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=42

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
