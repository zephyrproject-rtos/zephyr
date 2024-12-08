#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test scenario:
# Initialize the mesh stack, but do not provision the device.
# Start the provisioning procedure using bt_mesh_prov_enable(BT_MESH_PROV_GATT).
# With a separate scanner device, observe PB-GATT beacons being sent
# at approximately 100ms intervals.
# Provision the device using bt_mesh_provision().
# With a separate scanner device, observe GATT Proxy beacons being sent
# at approximately 1s intervals.
# Allocate an advertiser buffer using bt_mesh_adv_buf_create().
# With the separate scanner, observe that the GATT Proxy beacons are not interrupted.
# Set the allocated buffer's xmit parameters to BT_MESH_TRANSMIT(5, 20).
# Submit the advertiser buffer using bt_mesh_adv_send().
# With the separate scanner, observe that the submitted buffer is transmitted
# at least 6 times. Then, observe GATT Proxy beacons resuming.
overlay=overlay_gatt_conf
RunTest mesh_adv_proxy_mixin adv_tx_proxy_mixin adv_rx_proxy_mixin

overlay=overlay_gatt_conf_overlay_workq_sys_conf
RunTest mesh_adv_proxy_mixin_workq adv_tx_proxy_mixin adv_rx_proxy_mixin

overlay="overlay_gatt_conf_overlay_psa_conf"
RunTest mesh_adv_proxy_mixin_psa adv_tx_proxy_mixin adv_rx_proxy_mixin
