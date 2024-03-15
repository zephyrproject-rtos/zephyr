#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="central_hr_peripheral_hr_extended_test"
test_long_name="$(guess_test_long_name)"
verbosity_level=2

EXECUTE_TIMEOUT=10

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_samples_bluetooth_peripheral_hr_prj_conf_overlay-extended_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD_TS}_${test_long_name}_prj_conf_overlay-extended_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=central_hr_peripheral_hr

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
