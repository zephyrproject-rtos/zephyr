#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies a self-only Firmware Distribution Server update that is started
# without apply and applied later with Firmware Distribution Apply.
#
# Node layout:
#   Device 0: Distributor (DIST_ADDR / DIST_ADDR + 1). Its own element 2 is the
#             only Receiver.
#
# Test procedure (single run, no reboot):
# 1. The distribution starts with apply disabled and reaches Transfer Success,
#    with the local Firmware Update Server in Verification Succeeded.
# 2. The test calls bt_mesh_dfd_srv_apply(). The client's Apply step skips the
#    only target and completes synchronously.
# 3. Test asserts:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED, reported once.
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_IDLE.
#      - Receiver phase == BT_MESH_DFU_PHASE_APPLY_SUCCESS.

overlay=overlay_pst_conf
RunTest dfu_self_update_manual_apply \
    dfu_dist_dfu_self_update_manual_apply \
    -- -argstest targets=1
