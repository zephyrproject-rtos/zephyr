#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# HID Service test: a HID Host (GATT client) connects to the HID Service
# server, discovers the service, checks the Report Map, the HID Information and
# the Report Reference descriptors, subscribes to the Input Report, and
# exercises GET_REPORT, SET_REPORT, the Protocol Mode and the HID Control
# Point, including a reconnection and unregistering the service.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_hids"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_hids_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=hids_server -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_hids_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=hids_client -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
