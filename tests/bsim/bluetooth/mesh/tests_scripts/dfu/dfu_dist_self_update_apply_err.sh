#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server reports a
# FAILED distribution when its own apply callback rejects the image.
#
# The application can refuse to install the new firmware (returning an error
# from the bt_mesh_dfu_srv_cb.apply callback). Per MshDFUv1.0 Section 6.1.2.3
# the Firmware Update Server leaves Applying Update for Idle "whether the
# firmware image is installed successfully or the installation fails", so the
# phase alone cannot distinguish the two outcomes. The deferred apply result
# therefore has to be propagated to the DFD Server, otherwise a rejected image
# is reported as a successful distribution.
#
# Node layout (see dfu_dist_self_update.sh for background on the two-element
# distributor mandated by Section 2.1.1):
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
# 3. After the confirm step the DFD Server invokes the deferred self-target
#    apply callback, which returns -EIO instead of installing the image. The
#    local Firmware Update Server transitions APPLYING -> IDLE and erases its
#    persisted state.
# 4. Test asserts the distribution ended in the failed state:
#      - DFD Server phase == BT_MESH_DFD_PHASE_FAILED (not COMPLETED).

overlay=overlay_pst_conf
RunTest dfu_self_update_apply_err \
    dfu_dist_dfu_self_update_apply_err \
    -- -argstest targets=1 recover=0
