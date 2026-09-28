#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The GATT server queues a notification for batching and disconnects before
# the batch is flushed.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_notify_multiple_disconnect"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client_disconnect -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server_disconnect -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
