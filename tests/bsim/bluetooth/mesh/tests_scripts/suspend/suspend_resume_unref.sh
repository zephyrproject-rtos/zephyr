#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that initiates a broadcasting of the test model command and immediately suspends device.
# After a short pause, the device is resumed and attempt of broadcasting is repeated.
# This is repeated twice more time than allocated advertiser pool size to be sure in that
# device unreferences allocated advertiser from the pool back if it is suspended.
#
# Test procedure:
# 0. Provisioning and setup.
# 1. Send test model command twice to check that all allocated advertisers are freed.
# 2. Suspend Mesh.
# 3. Resume Mesh.
# 4. Repeat steps 1-3 twice more than allocated advertiser pool size.
RunTest mesh_suspend_resume_unref suspend_dut_suspend_resume_unref

overlay="overlay_low_lat_conf"
RunTest mesh_suspend_resume_unref_low_lat suspend_dut_suspend_resume_unref
