#!/usr/bin/env bash
# Copyright 2026 Koppel Electronic
# SPDX-License-Identifier: Apache-2.0

simulation_id="peripheral_gap_svc_test"
verbosity_level=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_samples_bluetooth_peripheral_gap_svc_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_samples_peripheral_gap_svc_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=central

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
