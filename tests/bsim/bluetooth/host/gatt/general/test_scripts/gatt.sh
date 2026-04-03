#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Basic GATT test: A central acting as a GATT client scans for and connects
# to a peripheral acting as a GATT server. The GATT client will then attempt
# to write and read to and from a few GATT characteristics.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="gatt"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
