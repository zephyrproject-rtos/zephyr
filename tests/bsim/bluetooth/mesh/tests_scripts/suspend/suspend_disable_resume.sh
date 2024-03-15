#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that periodic publication is stopped when suspending Mesh and disabling
# Bluetooth, and that it is started again when Bluetooth is re-enabled and Mesh is resumed.
# The test will fail under two conditions; if no publication is received while
# Mesh is enabled, or if a publication is received while Mesh is suspended.
#
# Test procedure:
# 0. Provisioning and setup.
# 1. Start publication.
# 2. Suspend Mesh and disable Bluetooth, checking that publication stops.
# 3. Enable Bluetooth and resume Mesh a specified time after suspension.
#   Check that publication resumes.

RunTest mesh_suspend_disable_resume \
	suspend_dut_suspend_disable_resume suspend_tester_pub

overlay=overlay_low_lat_conf
RunTest mesh_suspend_disable_resume_low_lat \
	suspend_dut_suspend_disable_resume suspend_tester_pub

overlay=overlay_psa_conf
RunTest mesh_suspend_disable_resume_psa \
	suspend_dut_suspend_disable_resume suspend_tester_pub
