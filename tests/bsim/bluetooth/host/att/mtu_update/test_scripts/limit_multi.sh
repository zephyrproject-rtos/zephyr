#!/usr/bin/env bash
# Copyright (c) 2026 Embeint Holdings Pty Ltd
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_mtu_update_limit_multi"
verbosity_level=2
sim_length_us=$((10 * 1000 * 1000))
central_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_central_auto_update_conf"
peripheral_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_peripheral_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=3 \
  -sim_length=${sim_length_us} $@

Execute "${central_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=limit_central_multi
Execute "${peripheral_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=limit_peripheral_multi_1
Execute "${peripheral_exe}" -v=${verbosity_level} -s=${simulation_id} -d=2 \
  -testid=limit_peripheral_multi_2

wait_for_background_jobs
