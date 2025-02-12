#!/usr/bin/env bash
# Copyright 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic Connected ISO test: multiple peripheral CIS establishment
simulation_id="connected_iso_peripheral_cis"
verbosity_level=2
EXECUTE_TIMEOUT=100

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-peripheral_cis_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-peripheral_cis_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
