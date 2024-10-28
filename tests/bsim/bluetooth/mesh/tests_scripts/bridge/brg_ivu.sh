#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that correct Net Keys are used when bridged subnets in a different IVU phase.
#
# Test Procedure:
# 1. All nodes have IV Update test mode enabled.
# 2. Provisioner configures itself and creates subnets equal to number of non-bridge devices.
# 3. Provisioner provisions and configures Subnet Bridge node to bridge the subnets.
# 4. Provisioner provisions and configures non-bridge devices for each subnet.

RunTest mesh_brg_ivu \
	brg_tester_ivu brg_bridge_simple_iv_test_mode brg_device_simple_iv_test_mode \
	brg_device_simple_iv_test_mode

overlay=overlay_psa_conf
RunTest mesh_brg_ivu_psa \
	brg_tester_ivu brg_bridge_simple_iv_test_mode brg_device_simple_iv_test_mode \
	brg_device_simple_iv_test_mode
