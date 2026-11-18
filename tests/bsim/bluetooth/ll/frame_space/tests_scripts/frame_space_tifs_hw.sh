#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Frame Space Update test: the central requests the smallest frame space
# supported by the peripheral radio, the PHY is updated to 2M, the peripheral
# requests a larger frame space and the connection interval is updated.
# H/w accelerated tIFS Trx switching.
simulation_id="${BOARD_TS}_frame_space_tifs_hw"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_frame_space_prj_conf_overlay-tifs_hw_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=peripheral

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_frame_space_prj_conf_overlay-tifs_hw_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=central

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
