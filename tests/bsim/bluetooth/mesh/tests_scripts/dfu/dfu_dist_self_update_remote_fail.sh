#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server still
# completes its own update when a remote Target fails the Confirm step.
#
# This is the mixed-outcome counterpart to dfu_dist_self_update_mult_targets.sh,
# where both Receivers succeed. Here the remote target is started with
# fail-confirm=1, so it applies the image but deliberately keeps reporting the
# old Firmware ID.
#
# Per MshDFUv1.0 Section 7.1.2.9, the Confirm Update On Target Nodes procedure
# "completes successfully" if at least one receiver in the Active Update
# Receivers state has a Retrieved Update Phase field value equal to Apply
# Success. The distribution therefore has to reach COMPLETED on the strength of
# the self-target alone, while the per-receiver entries still record the
# individual outcomes - the aggregate Distribution Phase is not a per-node
# result.
#
# Node layout (see dfu_dist_self_update.sh for background on the two-element
# distributor mandated by Section 2.1.1):
#   Device 0: Distributor (DIST_ADDR / DIST_ADDR + 1).
#   Device 1: Remote Firmware Update Target (TARGET_ADDR + 1), fail-confirm=1.
#
# Note that fail-confirm=1 is passed to every device, but only affects the
# remote target: on the distributor the self-target returns from
# target_dfu_apply() through the self_update_reboot_emulation path, which bumps
# the reported FWID and returns before the dfu_fail_confirm handling.
#
# Test procedure (first run, recover=0):
# 1. Identical to dfu_dist_self_update_mult_targets.sh: both nodes are
#    provisioned and configured, the distributor uploads a slot and adds its own
#    element 2 (DIST_ADDR + 1) plus the remote target (TARGET_ADDR + 1) as
#    Receivers, and the distribution runs to the deferred self-target apply.
# 2. The remote target applies and, because of fail-confirm=1, leaves
#    target_fw_ver_curr at the old value.
# 3. The distributor's deferred apply callback emulates a reboot (bumps its own
#    FWID, does not call bt_mesh_dfu_srv_applied()) and the run PASSes with the
#    DFD Server in APPLYING_UPDATE and the local DFU Server in APPLYING.
#
# Test procedure (second run, recover=1):
# 4. Both processes restart with the same flash contents. The distributor loads
#    persistent state, dfd_srv_model_start() moves the local DFU Server to IDLE
#    and resumes the Confirm step.
# 5. The self-target answers Firmware Update Information Get with the new FWID
#    and is marked APPLY_SUCCESS. The remote target answers with the old FWID,
#    so handle_info_status() reaches the confirm-procedure termination path and
#    marks it APPLY_FAIL.
# 6. confirmed() finds one successful Receiver, so the distribution completes.
#    Test asserts:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED.
#      - Local DFU Server phase == BT_MESH_DFU_PHASE_IDLE.
#      - targets[0] (self)   == BT_MESH_DFU_PHASE_APPLY_SUCCESS / SUCCESS.
#      - targets[1] (remote) == BT_MESH_DFU_PHASE_APPLY_FAIL.

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_remote_fail \
    dfu_dist_dfu_self_update_remote_fail -flash_erase \
    dfu_target_dfu_no_change -flash_erase \
    -- -argstest targets=2 recover=0 fail-confirm=1

overlay=overlay_pst_conf
RunTestFlash dfu_self_update_remote_fail \
    dfu_dist_dfu_self_update_remote_fail -flash_rm \
    dfu_target_dfu_no_change -flash_rm \
    -- -argstest targets=2 recover=1 fail-confirm=1
