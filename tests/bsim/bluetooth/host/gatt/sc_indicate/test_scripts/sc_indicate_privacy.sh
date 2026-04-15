#!/bin/env bash
# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This test verifies that the Service Changed indication is delivered when the central
# uses privacy (RPA) and subscribes to SC CCC before bonding. Before bonding the SC is
# stored under the central's RPA. On bonding, the SC is stored under the identity address
# through the bt_gatt_identity_resolved callback. This test will fail if the SC indication
# is not delivered after reconnection.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_sc_indicate_overlay-privacy_conf"
simulation_id='sc_indicate_privacy'
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -RealEncryption=1

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6

wait_for_background_jobs
