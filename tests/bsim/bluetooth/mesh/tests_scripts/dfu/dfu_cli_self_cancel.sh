#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a standalone Firmware Update Client cancels an update of a
# Firmware Update Server on its own node after the Apply step.
#
# Node layout:
#   Device 0: Firmware Update Client on element 1 (DIST_ADDR), Firmware Update
#             Server on element 2 (DIST_ADDR + 1), which is the only target.
#
# Test procedure (single run, no reboot):
# 1. The client distributes to the local server, which reaches Verification
#    Succeeded without a transfer.
# 2. The client runs the Apply step, which skips the local target, and then
#    cancels the update.
# 3. Test asserts that the local server is back in BT_MESH_DFU_PHASE_IDLE, so
#    Firmware Update Cancel reached it.

overlay=overlay_pst_conf
RunTest dfu_cli_self_cancel \
    dfu_cli_self_cancel
