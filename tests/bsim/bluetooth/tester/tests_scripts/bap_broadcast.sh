#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Smoketest for BAP BTP commands with the BT tester

simulation_id="tester_bap_broadcast"
verbosity_level=2
EXECUTE_TIMEOUT=100

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

UART_DIR=/tmp/bs_${USER}/${simulation_id}/
UART_SNK=${UART_DIR}/sink
UART_BCST=${UART_DIR}/broadcaster

# Broadcaster BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_le_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=10 -d=0 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_BCST}.tx -uart0_fifob_txfile=${UART_BCST}.rx

# Broadcaste Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=21 -d=10 -RealEncryption=1 \
  -testid=bap_broadcast_source \
  -nosim -uart0_fifob_rxfile=${UART_BCST}.rx -uart0_fifob_txfile=${UART_BCST}.tx

# Sink BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_le_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=32 -d=1 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_SNK}.tx -uart0_fifob_txfile=${UART_SNK}.rx

# Sink Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=43 -d=11 -RealEncryption=1 \
  -testid=bap_broadcast_sink \
  -nosim -uart0_fifob_rxfile=${UART_SNK}.rx -uart0_fifob_txfile=${UART_SNK}.tx

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs # Wait for all programs in background and return != 0 if any fails
