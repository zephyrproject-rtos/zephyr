#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies a Firmware Distribution Server self-update that uses a
# Distribution Multicast Address.
#
# The Distributor's own Target element is subscribed to the group like any other
# Target, so group-addressed Firmware Update and BLOB Transfer messages reach it
# over loopback although the Firmware Update Client skips it. Its deferred apply
# must still run only after the Confirm step, so the distribution completes with
# every Receiver confirmed.
#
# Node layout:
#   Device 0: Distributor (DIST_ADDR / DIST_ADDR + 1).
#   Device 1: Remote Firmware Update Target (TARGET_ADDR + 1).
#
# Test procedure (single run, no reboot):
# 1. Both devices subscribe their BLOB Transfer Server and Firmware Update Server
#    to group 0xc000.
# 2. Distributor adds its own element 2 and the remote target as Receivers and
#    starts the distribution to the group.
# 3. Test asserts:
#      - DFD Server phase == BT_MESH_DFD_PHASE_COMPLETED, reported once.
#      - Both Receivers have status Success and phase Apply Success.

overlay=overlay_pst_conf
RunTest dfu_self_update_group \
    dfu_dist_dfu_self_update_group \
    dfu_target_dfu_no_change \
    -- -argstest targets=2 group=49152
