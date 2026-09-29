#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Verify that the UART over NUS driver delivers none of the output queued or
# written after a peer disconnects to the next connection that subscribes,
# only the output written once it is subscribed.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_uart_bt_reconnect"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_uart_bt_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=uart_bt_reconnect_dut

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_uart_bt_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=uart_bt_reconnect_peer

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
