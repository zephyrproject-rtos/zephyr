#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test checks that mesh provisioning data is saved and loaded correctly.
#
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.
#
# Test procedure:
# 1. Device Under Test (DUT) initializes the Mesh stack,
#    and starts provisioning and configuration itself.
# 2. Test scenario emulates hardware  reset by running the second test
#    with stored data from the first test.
# 3. DUT starts mesh with loading stored data and checks that if they were restored correctly.

overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check persistence_provisioning_data_save -flash_erase \
	-- -argstest persist-stuck-key=0

overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check persistence_provisioning_data_load -flash_rm \
	-- -argstest persist-stuck-key=0
