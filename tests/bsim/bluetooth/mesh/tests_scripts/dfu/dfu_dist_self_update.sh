#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that the Firmware Distribution Server can perform a self-update
# and survive a reboot in the middle of the local Firmware Update Server's
# apply callback. Persisted state must be restored on the next boot and drive
# the distribution procedure to BT_MESH_DFD_PHASE_COMPLETED.
#
# Per MshDFUv1.0 Section 2.1.1, the Firmware Update Server (Target role) must
# reside on a different element than the Firmware Distribution Server
# (Distributor role) when the two are co-located. The Distributor node
# therefore has two elements:
#   - Element 1 (DIST_ADDR):     Config, SAR Config, Firmware Distribution
#                                Server and the internal Firmware Update
#                                Client + BLOB Transfer Client that drive
#                                the transfer.
#   - Element 2 (DIST_ADDR + 1): Firmware Update Server + BLOB Transfer Server
#                                (the self-target that receives the image).
#
# Test procedure (first run, recover=0):
# 1. Distributor is provisioned with the self-update composition and configured
#    (app key bound to the Client models on element 1 and to the Server models
#    on element 2).
# 2. Distributor uploads a firmware slot.
# 3. Distributor adds its own element 2 address (DIST_ADDR + 1) as the sole
#    Receiver of the distribution.
# 4. Distribution is started. Transfer and Apply steps run to completion. The
#    self-target reports APPLYING for the Apply step (deferred apply) and the
#    DFU Client's confirm step excuses it as expected.
# 5. Once the confirm step completes, the DFD Server invokes the deferred
#    self-target apply callback. The test's apply callback emulates a device
#    reboot by:
#      - updating the reported FWID (target_fw_ver_curr = target_fw_ver_new),
#      - releasing the dfu_ended semaphore so the test main can proceed,
#      - returning without calling bt_mesh_dfu_srv_applied().
# 6. Test asserts pre-reboot invariants:
#      - DFD Server phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE (state persisted).
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_APPLYING (state persisted).
# 7. Test PASSes and the bsim process exits, flushing persistent state to flash.
#
# Test procedure (second run, recover=1):
# 8. The bsim process starts again with the same flash contents (no
#    -flash_erase). No fresh provisioning, no receiver adds, no distribution
#    start.
# 9. target_fw_ver_curr is set to the new FWID before bt_mesh_device_setup() to
#    emulate the effect of running the new firmware image.
# 10. bt_mesh_device_setup() loads persistent state (DFD Server target list and
#    distribution parameters, DFU Client xfer state, DFU Server update state)
#    and invokes dfd_srv_start(), which:
#      - transitions the local DFU Server from APPLYING to IDLE
#        (bt_mesh_dfu_srv_applied()),
#      - resumes the confirm step (bt_mesh_dfu_cli_confirm()).
# 11. The resumed confirm step polls the self-target's Firmware Update
#    Information Get, which now responds with the new FWID.
# 12. DFU Client confirms the target, dfu_confirmed() runs, DFD phase
#    transitions to BT_MESH_DFD_PHASE_COMPLETED, and dfu_dist_ended fires.
# 13. Test asserts final invariants:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED.
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_IDLE.
#      - Self-target's targets[] entry phase == BT_MESH_DFU_PHASE_APPLY_SUCCESS
#        with status BT_MESH_DFU_SUCCESS.

overlay=overlay_pst_conf
RunTestFlash dfu_self_update \
    dfu_dist_dfu_self_update -flash_erase \
    -- -argstest targets=1 recover=0

overlay=overlay_pst_conf
RunTestFlash dfu_self_update \
    dfu_dist_dfu_self_update -flash_rm \
    -- -argstest targets=1 recover=1
