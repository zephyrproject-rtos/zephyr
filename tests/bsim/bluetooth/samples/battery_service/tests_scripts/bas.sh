#!/usr/bin/env bash
# Copyright 2024 Demant A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Battery service test: a central connects to a peripheral and expects a
# indication/notification of BAS chars from peripheral
simulation_id="battery_service_test"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_samples_battery_service_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=peripheral -rs=23

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_samples_battery_service_prj_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=central -rs=6

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_samples_battery_service_prj_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=2 \
  -testid=central -rs=6

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=10e6 $@

wait_for_background_jobs
