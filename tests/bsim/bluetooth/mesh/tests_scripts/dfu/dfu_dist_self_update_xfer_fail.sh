#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a self-updating Firmware Distribution Server does not apply
# its own image when the distribution fails before the Apply step.
#
# Node layout:
#   Device 0: Distributor (DIST_ADDR / DIST_ADDR + 1). The distribution also
#             lists a remote receiver (TARGET_ADDR + 1) that does not exist, so
#             the Transfer step is not skipped.
#
# Test procedure (single run, no reboot):
# 1. The self-target accepts Firmware Update Start and reaches Verification
#    Succeeded without a transfer.
# 2. The absent remote receiver is dropped, and the BLOB read stream fails to
#    open when the transfer starts, so the distribution fails.
# 3. Test asserts:
#      - The apply callback is never called.
#      - DFD Server phase == BT_MESH_DFD_PHASE_FAILED.
#      - Local DFU Server phase stays BT_MESH_DFU_PHASE_VERIFY_OK.

overlay=overlay_pst_conf
RunTest dfu_self_update_xfer_fail \
    dfu_dist_dfu_self_update_xfer_fail \
    -- -argstest targets=2
