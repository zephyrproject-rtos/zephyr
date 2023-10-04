#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# Tests with recover set to 0 clear previous settings and start new procedure.
# Tests with recover set to 1 load stored settings and continue procedure to next phase.
# Test cases are designed to be run using single target.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=0 expected-phase=1

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=1 expected-phase=2

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=1 expected-phase=3

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=1 expected-phase=4

# Test reaching suspended state and continuation after reboot on new procedure.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=0 expected-phase=5

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest blob_recover_phase blob_cli_stop blob_srv_stop -- -argstest \
   recover=1 expected-phase=4
