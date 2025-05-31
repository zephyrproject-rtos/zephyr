#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Smoketest for CCP BTP commands with the BT tester

simulation_id="tester_ccp"
verbosity_level=2
EXECUTE_TIMEOUT=100

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

UART_DIR=/tmp/bs_${USER}/${simulation_id}/
UART_PER=${UART_DIR}/peripheral
UART_CEN=${UART_DIR}/central

# Central BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_le_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=10 -d=0 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_CEN}.tx -uart0_fifob_txfile=${UART_CEN}.rx

# Central Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=21 -d=10 -RealEncryption=1 -testid=ccp_central \
  -nosim -uart0_fifob_rxfile=${UART_CEN}.rx -uart0_fifob_txfile=${UART_CEN}.tx

# Peripheral BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_le_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=32 -d=1 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_PER}.tx -uart0_fifob_txfile=${UART_PER}.rx

# Peripheral Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=43 -d=11 -RealEncryption=1 -testid=ccp_peripheral \
  -nosim -uart0_fifob_rxfile=${UART_PER}.rx -uart0_fifob_txfile=${UART_PER}.tx

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs # Wait for all programs in background and return != 0 if any fails
