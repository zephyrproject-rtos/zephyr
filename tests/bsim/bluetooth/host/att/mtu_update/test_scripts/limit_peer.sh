#!/usr/bin/env bash
# Copyright (c) 2026 Embeint Holdings Pty Ltd
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_mtu_update_limit_peer"
verbosity_level=2
sim_length_us=$((5 * 1000 * 1000))
central_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_central_conf"
peripheral_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_peripheral_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 \
  -sim_length=${sim_length_us} $@

Execute "${central_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=limit_central_peer
Execute "${peripheral_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=limit_peripheral_peer

wait_for_background_jobs
