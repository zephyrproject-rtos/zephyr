#!/usr/bin/env bash
# Copyright 2024-2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Smoketest for GAP ISO BTP commands with the BT tester

simulation_id="tester_gap_iso"
verbosity_level=2
EXECUTE_TIMEOUT=100

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

UART_DIR=/tmp/bs_${USER}/${simulation_id}/
UART_ISO_BROADCASTER=${UART_DIR}/broadcaster
UART_ISO_SYNC_RECEIVER=${UART_DIR}/sync_receiver

# broadcaster BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=10 -d=0 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_ISO_BROADCASTER}.tx -uart0_fifob_txfile=${UART_ISO_BROADCASTER}.rx

# broadcaster Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=21 -d=10 -RealEncryption=1 -testid=iso_broadcaster \
  -nosim -uart0_fifob_rxfile=${UART_ISO_BROADCASTER}.rx \
  -uart0_fifob_txfile=${UART_ISO_BROADCASTER}.tx

# Sync receiver BT Tester
Execute ./bs_${BOARD_TS}_tests_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=32 -d=1 -RealEncryption=1 \
  -uart0_fifob_rxfile=${UART_ISO_SYNC_RECEIVER}.tx -uart0_fifob_txfile=${UART_ISO_SYNC_RECEIVER}.rx

# Sync receiver Upper Tester
Execute ./bs_nrf52_bsim_native_tests_bsim_bluetooth_tester_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -rs=43 -d=11 -RealEncryption=1 \
  -testid=iso_sync_receiver -nosim -uart0_fifob_rxfile=${UART_ISO_SYNC_RECEIVER}.rx \
  -uart0_fifob_txfile=${UART_ISO_SYNC_RECEIVER}.tx

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs # Wait for all programs in background and return != 0 if any fails
