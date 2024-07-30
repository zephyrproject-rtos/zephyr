#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that GATT advertisement is stopped when suspending Mesh and disabling
# Bluetooth, and that it is started again when Bluetooth is re-enabled and Mesh is resumed.
#
# Test procedure:
# 0. DUT (Device 0) initializes the Mesh stack, and starts provisioning procedure using
#    bt_mesh_prov_enable(BT_MESH_PROV_GATT).
# 1. Tester (Device 1) observes PB-GATT advs, and will fail the test if the expected
#    amount of advs is not received.
# 2. DUT is provisioned, and Tester observes GATT proxy advs.
# 3. DUT notifies the Tester that it will be suspended, and Tester observes for advs after a
#    brief delay. Receiving an adv while DUT is suspended will cause the test to fail.
# 4. After a delay, the DUT resumes and notifies the Tester, which checks that the
#    advertising resumes.

overlay=overlay_gatt_conf
RunTest mesh_gatt_suspend_disable_resume \
	suspend_dut_gatt_suspend_disable_resume suspend_tester_gatt

overlay="overlay_gatt_conf_overlay_low_lat_conf"
RunTest mesh_gatt_suspend_disable_resume_low_lat \
	suspend_dut_gatt_suspend_disable_resume suspend_tester_gatt

overlay="overlay_gatt_conf_overlay_psa_conf"
RunTest mesh_gatt_suspend_disable_resume_psa \
	suspend_dut_gatt_suspend_disable_resume suspend_tester_gatt
