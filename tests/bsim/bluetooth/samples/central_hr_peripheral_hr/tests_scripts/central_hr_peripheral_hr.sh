#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

simulation_id="central_hr_peripheral_hr_test"
verbosity_level=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_samples_bluetooth_peripheral_hr_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_samples_central_hr_peripheral_hr_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=central_hr_peripheral_hr

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=12e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
