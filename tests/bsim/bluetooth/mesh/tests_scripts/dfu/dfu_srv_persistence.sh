#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test pair: executes Receive Firmware procedure up to certain point using distributor and
# target.
# Second test pair: tests are executed with `recover` enabled. This means target will recover
# settings from persistent storage, which will allow to verify if stored DFU server's phase and
# image index were loaded correctly.
# Test cases are designed to be run using single target. `dfu_cli_stop` test case in recovery part
# plays dummy role, and is there to keep order of settings files being loaded.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
   recover=0 expected-phase=2

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop  -- -argstest \
  recover=1 expected-phase=3

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
  recover=1 expected-phase=4

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
  recover=1 expected-phase=6

# Use phase `BT_MESH_DFU_PHASE_APPLY_SUCCESS` as marker to bring whole procedure to an end
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
  recover=1 expected-phase=8

# To test recovery from Verify Fail begin new distribution that will end there,
# reboot devices and continue to Applying.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
   recover=0 expected-phase=5

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_dist_recover_phase dfu_cli_stop dfu_target_dfu_stop -- -argstest \
   recover=1 expected-phase=6
