#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The GATT server sends two values in one Multiple Handle Value Notification
# PDU, first one octet larger than the ATT_MTU and then exactly as large.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_notify_multiple_mtu"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client_mtu -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server_mtu -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
