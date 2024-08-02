#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test pair: executes Receive Firmware procedure up to certain point using distributor and
# target.
# Second test pair: tests recovery from Verify Fail begin new distribution that will end there,
# reboot devices and continue to Applying.
overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_verify_fail \
  dfu_cli_stop -flash_erase dfu_target_dfu_stop -flash_erase \
  -- -argstest recover=0 expected-phase=5

overlay=overlay_pst_conf
RunTestFlash dfu_dist_recover_verify_fail \
  dfu_cli_stop -flash_rm dfu_target_dfu_stop -flash_rm \
  -- -argstest recover=1 expected-phase=6

# The same test but with PSA crypto
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_verify_fail_psa \
  dfu_cli_stop -flash_erase dfu_target_dfu_stop -flash_erase \
  -- -argstest recover=0 expected-phase=5

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash dfu_dist_recover_verify_fail_psa \
  dfu_cli_stop -flash_rm dfu_target_dfu_stop -flash_rm \
  -- -argstest recover=1 expected-phase=6
