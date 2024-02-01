#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# The test checks that both advertisers, the legacy and the extended, behave identically.
# In particular:
# - `bt_mesh_send_cb.end` callback with error code `0` is called for the advertisement that the
#   advertiser already pushed to the ble host (called `bt_mesh_send_cb.start`),
# - `bt_mesh_send_cb.start` callback with error `-ENODEV` is called for every advertisement that
#   was pushed to the mesh advertiser using `bt_mesh_adv_send` function,
# - `bt_mesh_adv_create` returns NULL when attempting to create a new advertisement while the stack
#   is suspended.
#
# Tx device procedure:
# 1. Tx device creates `CONFIG_BT_MESH_ADV_BUF_COUNT` advertisements, setting the first byte of the
# PDU to 0xAA and the second byte of the PDU to the advertisement order number
# 2. Tx devices pushes all created advs to the pool by calling `bt_mesh_adv_send` function
# 3. When the `bt_mesh_send_cb.start` callback is called for the first adv, the tx device submits
#    a work item which suspends the advertiser by calling `bt_mesh_adv_disable`.
# 4. Tx device checks that for the first adv the `bt_mesh_send_cb.end` callback is called with the
#    error code `0`.
# 5. Tx device checks that for the consecutive advs the `bt_mesh_send_cb.start` is called with error
#    `-ENODEV`.
# 6. Tx device checks that no more advs can be created using `bt_mesh_adv_create` function.
# 7. Tx device resumes the advertiser and repeats steps 1 and 2.
# 8. Tx device expects that all advs are sent.
#
# Rx device procedure:
# 1. Rx devices listens all the time while tx device sends advs and ensure that no new advs were
#    sent after the advertiser was suspended.

RunTest mesh_adv_disable adv_tx_disable adv_rx_disable

# Low latency overlay uses legacy advertiser
overlay=overlay_low_lat_conf
RunTest mesh_adv_disable adv_tx_disable adv_rx_disable
