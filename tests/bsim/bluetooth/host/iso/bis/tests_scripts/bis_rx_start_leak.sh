#!/usr/bin/env bash
# Copyright (c) 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# Reproducer for an ISO RX buffer leak when a BIG sync ends while a BIS has a Start fragment
# in reassembly. Fails (receiver reports "ISO RX buffer leaked") until the leak is fixed.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_iso_bis_rx_start_leak"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_bis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=broadcaster

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_bis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=receiver_rx_start_leak

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=30e6 $@

wait_for_background_jobs
