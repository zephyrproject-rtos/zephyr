#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server does not
# report a successful distribution when the deferred self-target apply never
# installed the new firmware image.
#
# This is the counterpart to dfu_dist_self_update.sh. That test emulates a
# reboot AFTER the image was installed (the apply callback bumps the reported
# FWID); this test emulates power loss BEFORE the image was installed, so the
# node comes back up running the OLD firmware while Update Phase = Applying
# Update is still on flash.
#
# Why the distribution must fail rather than complete, per MshDFUv1.0:
#   - Applying Update (0x6) is server-sourced and means "the Apply New Firmware
#     procedure is being executed" only (Section 4.1.2 Table 4.5, Section 4.2.1
#     Table 4.8). It is not a success indication.
#   - Apply Success (0x8) is client-sourced (Table 4.8) and is derived by the
#     Confirm Update On Target Nodes procedure (Section 7.1.2.9), whose
#     Table 7.3 condition for a node that stays provisioned is that the
#     distributed Firmware ID matches the Target's Current Firmware ID.
#   - A node that did not apply must still report the old Firmware ID, because
#     the Firmware Information List state "changes after the node applies a new
#     firmware update successfully" (Section 4.1.1).
# The FWID comparison in the Confirm step is therefore the mechanism that is
# required to catch this case, and this test pins that behavior.
#
# Node layout (see dfu_dist_self_update.sh for background on the two-element
# distributor mandated by Section 2.1.1):
#   Device 0: Distributor. Element 1 (DIST_ADDR) hosts the Firmware
#             Distribution Server; element 2 (DIST_ADDR + 1) hosts the Firmware
#             Update Server that is the sole Receiver of the distribution.
#
# Test procedure (first run, recover=0):
# 1. Distributor is provisioned and configured for self-update.
# 2. Distributor uploads a firmware slot and adds its own element 2
#    (DIST_ADDR + 1) as the sole Receiver.
# 3. Distribution starts. Transfer and Apply steps run to completion. The
#    self-target reports APPLYING for the Apply step (deferred apply) and the
#    DFU Client's confirm step excuses it.
# 4. After the confirm step, the DFD Server invokes the deferred self-target
#    apply callback. With self_update_apply_fail set, that callback emulates
#    power loss before the image swap: it leaves target_fw_ver_curr at the OLD
#    value, does NOT call bt_mesh_dfu_srv_applied(), gives dfu_ended and
#    returns.
# 5. Test asserts the same pre-reboot invariants as dfu_dist_self_update.sh -
#    DFD Server in APPLYING_UPDATE and local DFU Server in APPLYING, both
#    persisted. The two runs are indistinguishable on flash at this point;
#    only the running image differs.
# 6. Test PASSes and the bsim process exits, flushing persistent state.
#
# Test procedure (second run, recover=1):
# 7. The bsim process starts again with the same flash contents (no
#    -flash_erase), and target_fw_ver_curr is deliberately left at its old
#    value (0xDEADBEEF) to emulate still running the pre-update image.
# 8. bt_mesh_device_setup() loads persistent state and invokes dfd_srv_model_start(),
#    which transitions the local DFU Server from APPLYING to IDLE and resumes
#    the confirm step.
# 9. The resumed confirm step polls the self-target with Firmware Update
#    Information Get. The self-target answers with the OLD FWID, so no image in
#    its Images List matches the distributed Firmware ID.
# 10. handle_info_status() reaches the confirm-procedure termination path,
#    marks the Receiver BT_MESH_DFU_PHASE_APPLY_FAIL and fails it. confirmed()
#    then finds no successful Receiver and fails the distribution.
# 11. Test asserts the distribution ended in the failed state:
#      - DFD Server phase == BT_MESH_DFD_PHASE_FAILED.
#      - Self-target's targets[] entry phase == BT_MESH_DFU_PHASE_APPLY_FAIL.

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_apply_fail \
    dfu_dist_dfu_self_update_apply_fail -flash_erase \
    -- -argstest targets=1 recover=0

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_apply_fail \
    dfu_dist_dfu_self_update_apply_fail -flash_rm \
    -- -argstest targets=1 recover=1
