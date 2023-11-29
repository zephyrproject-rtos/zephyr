#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# Tests with -flash_erase run with clear flash and start new procedure.
# Tests with -flash_rm clean up stored settings after them
# to run tests with -flash_erase correctly.
# Test cases are designed to be run using single target.
overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop -flash_erase blob_srv_stop -flash_erase -- -argstest expected-phase=1

overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop blob_srv_stop -- -argstest expected-phase=2

overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop blob_srv_stop -- -argstest expected-phase=3

overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop -flash_rm blob_srv_stop -flash_rm -- -argstest expected-phase=4

# Test reaching suspended state and continuation after reboot on new procedure.
overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop -flash_erase blob_srv_stop -flash_erase -- -argstest expected-phase=5

overlay=overlay_pst_conf
RunTestFlash blob_recover_phase \
   blob_cli_stop -flash_rm blob_srv_stop -flash_rm -- -argstest expected-phase=4

# The same test but with PSA crypto
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop -flash_erase blob_srv_stop -flash_erase -- -argstest expected-phase=1

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop blob_srv_stop -- -argstest expected-phase=2

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop blob_srv_stop -- -argstest expected-phase=3

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop -flash_rm blob_srv_stop -flash_rm -- -argstest expected-phase=4

# Test reaching suspended state and continuation after reboot on new procedure.
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop -flash_erase blob_srv_stop -flash_erase -- -argstest expected-phase=5

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash blob_recover_phase_psa \
   blob_cli_stop -flash_rm blob_srv_stop -flash_rm -- -argstest expected-phase=4
