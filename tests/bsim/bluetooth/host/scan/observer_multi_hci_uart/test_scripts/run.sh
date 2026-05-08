#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Multi-controller observer test using two HCI UART controller instances.
#
# Topology:
#   [observer_multi app] --(uart1/fifob)--> [hci_uart ctrl 0] <--(phy)--> [hci_uart ctrl 1]
#   [observer_multi app] --(uart0/fifob)--> [hci_uart ctrl 1]
#
# The observer_multi sample initialises two HCI controllers (one per UART),
# starts passive scanning on each, logs advertising reports from both, and
# exits cleanly.  The two hci_uart processes run the open-source Zephyr LL
# SW-split Bluetooth controller and are connected to the 2G4 radio PHY.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="observer_multi_hci_uart"
verbosity_level=2
EXECUTE_TIMEOUT=60

cd ${BSIM_OUT_PATH}/bin

UART_DIR=/tmp/bs_${USER}/${simulation_id}/
UART_HCI0=${UART_DIR}/hci0
UART_HCI1=${UART_DIR}/hci1

mkdir -p ${UART_DIR}

# Observer multi app (no PHY connection):
#   uart1 (HCI0) <--> controller 0
#   uart0 (HCI1) <--> controller 1
Execute ./bs_${BOARD_TS}_samples_bluetooth_observer_multi_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=10 -nosim \
  -uart1_fifob_rxfile=${UART_HCI0}.rx -uart1_fifob_txfile=${UART_HCI0}.tx \
  -uart0_fifob_rxfile=${UART_HCI1}.rx -uart0_fifob_txfile=${UART_HCI1}.tx

# Controller 0 (HCI0): connected to PHY as device 0, uart1 <--> observer app
Execute ./bs_${BOARD_TS}_samples_bluetooth_hci_uart_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -uart1_fifob_rxfile=${UART_HCI0}.tx -uart1_fifob_txfile=${UART_HCI0}.rx

# Controller 1 (HCI1): connected to PHY as device 1, uart1 <--> observer app
# (the observer app reaches this controller via its own uart0)
Execute ./bs_${BOARD_TS}_samples_bluetooth_hci_uart_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=0 \
  -uart1_fifob_rxfile=${UART_HCI1}.tx -uart1_fifob_txfile=${UART_HCI1}.rx

# PHY simulation – 2 radio devices (the two controllers), 15 s of simulated time
Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=15e6 $@

wait_for_background_jobs
