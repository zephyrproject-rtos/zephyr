#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a Firmware Update Client cancels an update at different
# steps, and that Firmware Update Cancel reaches every target.
#
# Node layout:
#   Device 0: Firmware Update Client on element 1 (DIST_ADDR), Firmware Update
#             Server on element 2 (DIST_ADDR + 1). The local server verifies
#             without a transfer, so the Transfer step skips it.
#   Device 1: Remote Firmware Update Server (TARGET_ADDR + 1).
#
# Test procedure, for each cancel-step:
#   0: the client cancels during the Transfer step, after the first chunk;
#   1: the client cancels after the transfer, with both servers verified;
#   2: the client cancels after the Apply step, with the local apply deferred
#      and the remote server applied.
# The client asserts that every target acknowledged the cancel and that the
# local server is back in BT_MESH_DFU_PHASE_IDLE.

overlay=overlay_pst_conf
RunTest dfu_cli_cancel_in_transfer \
    dfu_cli_cancel dfu_target_dfu_cancel \
    -- -argstest cancel-step=0

overlay=overlay_pst_conf
RunTest dfu_cli_cancel_after_verify \
    dfu_cli_cancel dfu_target_dfu_cancel \
    -- -argstest cancel-step=1

overlay=overlay_pst_conf
RunTest dfu_cli_cancel_after_apply \
    dfu_cli_cancel dfu_target_dfu_cancel \
    -- -argstest cancel-step=2
