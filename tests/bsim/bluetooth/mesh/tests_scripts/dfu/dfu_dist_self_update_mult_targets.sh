#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies the Firmware Distribution Server self-update procedure with a
# mix of Receivers - the co-located self-target plus one remote DFU Target -
# including a reboot in the middle of the local Firmware Update Server's apply
# callback. Persisted state on both devices must be restored on the next boot
# and drive the distribution procedure to BT_MESH_DFD_PHASE_COMPLETED.
#
# Node layout (see dfu_dist_self_update.sh for background on the two-element
# distributor):
#   Device 0: Distributor (DIST_ADDR / DIST_ADDR + 1).
#   Device 1: Remote Firmware Update Target (TARGET_ADDR + 1).
#
# Test procedure (first run, recover=0):
# 1. Distributor is provisioned and configured for self-update.
# 2. Remote target is provisioned and configured as a plain DFU target
#    (test_target_dfu_no_change / target_test_effect(EFFECT_NONE)).
# 3. Distributor uploads a firmware slot and adds two Receivers - its own
#    element 2 (DIST_ADDR + 1) and the remote target (TARGET_ADDR + 1).
# 4. Distribution starts. Transfer and Apply steps run to completion.
# 5. The remote target's apply callback runs to completion, updating its
#    reported FWID and returning after bt_mesh_dfu_srv_applied(). The remote
#    target PASSes and its bsim process exits after target_test_effect().
# 6. On the distributor, the self-target reports APPLYING for the Apply step
#    (deferred apply) and the DFU Client's confirm step excuses it.
# 7. After the confirm step, the DFD Server invokes the deferred self-target
#    apply callback. The test's apply callback emulates a device reboot
#    (updates FWID, gives dfu_ended, returns without applied()).
# 8. Distributor asserts pre-reboot invariants:
#      - DFD Server phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE (state persisted).
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_APPLYING (state persisted).
# 9. Distributor PASSes; bsim exits and flushes persistent state to flash on
#    both devices.
#
# Test procedure (second run, recover=1):
# 10. Both bsim processes start again with the same flash contents (no
#    -flash_erase).
# 11. Remote target: target_fw_ver_curr is set to the new FWID before
#    bt_mesh_device_setup() to emulate having booted new firmware. No apply
#    procedure runs on this side; it merely responds to the Firmware Update
#    Information Get poll from the distributor.
# 12. Distributor: target_fw_ver_curr is set to the new FWID before
#    bt_mesh_device_setup(). bt_mesh_device_setup() loads persistent state and
#    invokes dfd_srv_start(), which transitions the local DFU Server from
#    APPLYING to IDLE and resumes the confirm step.
# 13. The resumed confirm step polls both targets for their new FWID. Both
#    respond with the new FWID; DFU Client marks them APPLY_SUCCESS.
# 14. dfu_confirmed() runs, DFD phase transitions to
#    BT_MESH_DFD_PHASE_COMPLETED, dfu_dist_ended fires.
# 15. Distributor asserts final invariants for both entries in targets[]:
#      - phase == BT_MESH_DFU_PHASE_APPLY_SUCCESS.
#      - status == BT_MESH_DFU_SUCCESS.

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_no_change \
    dfu_dist_dfu_self_update -flash_erase \
    dfu_target_dfu_no_change -flash_erase \
    -- -argstest targets=2 recover=0

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_no_change \
    dfu_dist_dfu_self_update -flash_rm \
    dfu_target_dfu_no_change -flash_rm \
    -- -argstest targets=2 recover=1
