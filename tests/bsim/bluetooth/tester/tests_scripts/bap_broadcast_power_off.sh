#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# The BT tester is powered off while its BAP broadcast source is streaming

simulation_id="${BOARD_TS}_tester_bap_broadcast_power_off"
verbosity_level=2
EXECUTE_TIMEOUT=300

cd ${BSIM_OUT_PATH}/bin

UART_DIR=/tmp/bs_${USER}/${simulation_id}/
UART_BCST=${UART_DIR}/broadcaster

# Broadcaster BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_le_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=10 -d=0 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_BCST}.tx -uart0_fifob_txfile=${UART_BCST}.rx

# Broadcaster Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=21 -d=10 -RealEncryption=1 \
  -testid=bap_broadcast_source_power_off \
  -nosim -uart0_fifob_rxfile=${UART_BCST}.rx -uart0_fifob_txfile=${UART_BCST}.tx

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=1 -sim_length=20e6 $@

wait_for_background_jobs # Wait for all programs in background and return != 0 if any fails
