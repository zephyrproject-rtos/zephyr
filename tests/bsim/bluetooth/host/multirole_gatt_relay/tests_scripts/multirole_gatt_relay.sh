#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_host-multirole-gatt-relay"
verbosity_level=2
EXECUTE_TIMEOUT=900

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_multirole_gatt_relay_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_multirole_gatt_relay_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -xo_drift=+100e-6 \
  -testid=multirole

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_multirole_gatt_relay_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 \
  -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=600e6 $@ -argschannel -at=40

wait_for_background_jobs
