#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server completes the
# distribution when its own apply is performed asynchronously.
#
# The deferred self-apply callback may reboot the device, call
# bt_mesh_dfu_srv_applied() before returning, or - as here - install the image
# and report completion later. In the last case dfu_confirmed() has already
# returned with the Distribution Phase at Applying Update, so the completion
# has to be driven from bt_mesh_dfu_srv_applied() through
# bt_mesh_dfd_srv_self_applied().
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
# 2. Distribution runs through Transfer and Apply. The self-target reports
#    APPLYING for the Apply step (deferred apply) and the DFU Client's confirm
#    step excuses it.
# 3. The deferred apply callback bumps the reported FWID (image installed) and
#    returns without calling bt_mesh_dfu_srv_applied().
# 4. Test asserts the distribution has not completed yet:
#      - DFD Server phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE.
#      - No Completed phase notification seen.
# 5. Test calls bt_mesh_dfu_srv_applied() as the application would once the
#    install finished, and asserts:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED.
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_IDLE.
#      - Exactly one Completed phase notification, so the synchronous path in
#        dfu_confirmed() does not report it a second time.

overlay=overlay_pst_conf
RunTest dfu_self_update_apply_async \
    dfu_dist_dfu_self_update_apply_async \
    -- -argstest targets=1 recover=0
