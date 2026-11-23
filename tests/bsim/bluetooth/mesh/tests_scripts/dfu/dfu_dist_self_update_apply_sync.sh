#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server reports the
# Completed phase exactly once when its own apply finishes inside the callback.
#
# This is the synchronous counterpart to dfu_dist_self_update_apply_async.sh.
# The apply callback calls bt_mesh_dfu_srv_applied() and returns, so the
# Firmware Update Server notifies the Distribution Server through
# bt_mesh_dfd_srv_self_applied() while dfu_confirmed() is still on the stack.
# dfu_confirmed() must then not report the Completed phase a second time.
#
# Node layout (see dfu_dist_self_update.sh for background on the two-element
# distributor mandated by MshDFUv1.0 Section 2.1.1):
#   Device 0: Distributor. Element 1 (DIST_ADDR) hosts the Firmware
#             Distribution Server; element 2 (DIST_ADDR + 1) hosts the Firmware
#             Update Server that is the sole Receiver of the distribution.
#
# Test procedure (single run, no reboot):
# 1. Distributor is provisioned and configured for self-update, uploads a
#    firmware slot and adds its own element 2 (DIST_ADDR + 1) as the sole
#    Receiver.
# 2. Distribution runs through Transfer and Apply. No emulation flag is set, so
#    the deferred apply callback takes the ordinary path: it calls
#    bt_mesh_dfu_srv_applied() and returns 0.
# 3. Test asserts:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED.
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_IDLE.
#      - Exactly one Completed phase notification.

overlay=overlay_pst_conf
RunTest dfu_self_update_apply_sync \
    dfu_dist_dfu_self_update_apply_sync \
    -- -argstest targets=1 recover=0
