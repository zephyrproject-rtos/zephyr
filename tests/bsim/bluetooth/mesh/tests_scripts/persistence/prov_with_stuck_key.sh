#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test checks that mesh provisioning data is saved and loaded correctly
# if a key with ID from mesh PSA key ID range exists in the PSA ITS,
# it is destroyed and the correct key is used instead.
#
# Test must be added in pair and in sequence.
# First test: saves data; second test: verifies it.
#
# Test procedure:
# 1. Fake key is created in the PSA ITS to emulate gotten stuck key.
# 2. Device Under Test (DUT) initializes the Mesh stack,
#    and starts provisioning and configuration itself.
# 3. Test scenario emulates hardware  reset by running the second test
#    with stored data from the first test.
# 4. DUT checks that the stuck key was substituted by the real key.
# 5. DUT starts mesh with loading stored data and checks that if they were restored correctly.

overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check_stuck_key persistence_provisioning_data_save -flash_erase \
	-- -argstest persist-stuck-key=1

overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check_stuck_key persistence_provisioning_data_load -flash_rm \
	-- -argstest persist-stuck-key=1
