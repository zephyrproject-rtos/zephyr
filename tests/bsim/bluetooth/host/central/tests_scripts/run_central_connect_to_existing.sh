#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="central_connect_timeout_to_existing"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_central_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_connect_to_existing

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_central_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_dummy

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
