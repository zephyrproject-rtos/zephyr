#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Test purpose:
#
# Verify that the Central can receive 30 notifications from the Peripheral over all available EATT
# channels.
#
# Note: The number of notifications and the size of the characteristic are chosen empirically.
#
# This test is designed to stress HCI driver to guarantee that no data is dropped between Host and
# Controller. Specifically, the HCI IPC driver that tends to drop data when the Host is not able to
# process it fast enough. The test is guaranteed to fail with Controller to Host HCI data flow
# control being disabled.
#
# Test Setup:
#
# - Devices: Central and Peripheral
# - Characteristic Length: Peripheral exposes a long characteristic (greater than 23 bytes).
# - Connection: Central connects to Peripheral.
# - Notification subscription: Central subscribes to notificaitons from the Peripheral.
# - EATT Usage: EATT is enabled, and the maximum possible EATT channels are established to
#   parallelize notifications as much as possible, increasing traffic towards Central.
#
# Test Steps:
#
# 1. The Peripheral sends 30 notifications to the Central over all available EATT channels.
# 2. Upon receiving a notification, the Central holds the received notification callback for 1
#    second before releasing it.
# 3. The test passes if all 30 notifications are successfully sent by the Peripheral and received
#    by the Central.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=150

verbosity_level=2
simulation_id="gatt_notify_enhanced_stress"
server_id="gatt_server_enhanced_notif_stress"
client_id="gatt_client_enhanced_notif_stress"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_stress_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=${client_id} -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_stress_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=${server_id} -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=70e6 $@

wait_for_background_jobs
