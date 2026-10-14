#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# Execute Write test: the client queues prepared writes and then cancels the
# queue, and queues a write that the server fails. It checks that the server
# answers a cancel with an Execute Write Response and that it stops at the
# first failing handle.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_exec_write"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=exec_write_client

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=exec_write_server

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
