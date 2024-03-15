#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the central_hr/peripheral_hr samples,
# It relies on the bs_tests hooks to register a test timer callback, which after a deadline
# will check how many packets has been received, and if over a threshold
# it considers the test passed

simulation_id="central_hr_peripheral_hr_extended_test"
verbosity_level=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=200

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_samples_bluetooth_peripheral_hr_prj_conf_overlay-extended_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_samples_central_hr_peripheral_hr_prj_conf_overlay-extended_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=central_hr_peripheral_hr

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=120e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
