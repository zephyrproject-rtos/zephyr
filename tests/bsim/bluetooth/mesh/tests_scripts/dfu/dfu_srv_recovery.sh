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
overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_phase \
  dfu_cli_stop -flash_erase dfu_target_dfu_stop -flash_erase \
  -- -argstest recover=0 expected-phase=2

overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_phase \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=3

overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_phase \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=4

overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_phase \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=6

# Use phase `BT_MESH_DFU_PHASE_APPLY_SUCCESS` as marker to bring whole procedure to an end
overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_phase \
  dfu_cli_stop -flash_rm dfu_target_dfu_stop -flash_rm \
  -- -argstest recover=1 expected-phase=8

# The same test but with PSA crypto
overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_phase_psa \
  dfu_cli_stop -flash_erase dfu_target_dfu_stop -flash_erase \
  -- -argstest recover=0 expected-phase=2

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_phase_psa \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=3

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_phase_psa \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=4

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_phase_psa \
  dfu_cli_stop dfu_target_dfu_stop \
  -- -argstest recover=1 expected-phase=6

# Use phase `BT_MESH_DFU_PHASE_APPLY_SUCCESS` as marker to bring whole procedure to an end
overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_phase_psa \
  dfu_cli_stop -flash_rm dfu_target_dfu_stop -flash_rm \
  -- -argstest recover=1 expected-phase=8
