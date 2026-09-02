#!/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Verify that the Service Changed configuration of a client that subscribed
# before bonding survives a reboot of both devices: the server database
# changes after the reboot while the client is disconnected, and the
# indication must still be delivered on reconnection.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_sc_indicate_prj_conf"
simulation_id="${BOARD_TS}_sc_indicate_reboot"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=central_reboot_subscribe_bond -RealEncryption=1 \
  -flash="${simulation_id}_client.log.bin" -flash_erase

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_reboot_bond -RealEncryption=1 \
  -flash="${simulation_id}_server.log.bin" -flash_erase

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6

wait_for_background_jobs

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id}.2 -d=0 \
  -testid=central_reboot_resubscribe -RealEncryption=1 \
  -flash="${simulation_id}_client.log.bin" -flash_rm

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id}.2 -d=1 \
  -testid=peripheral_reboot_indicate -RealEncryption=1 \
  -flash="${simulation_id}_server.log.bin" -flash_rm

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id}.2 \
  -D=2 -sim_length=30e6

wait_for_background_jobs
