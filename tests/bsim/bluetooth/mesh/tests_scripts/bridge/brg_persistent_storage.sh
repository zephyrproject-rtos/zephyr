#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that subnet bridge states are restored correctly after reboot
#
# Test procedure:
# 1. Tester configures itself and creates subnets equal to number of non-bridge nodes.
# 2. Tester provisions and configures Subnet Bridge node to bridge the subnets.
# 3. Devices reboot
# 4. Tester retrieves and verifies configuration of the Subnet Bridge node

overlay=overlay_pst_conf
RunTestFlash mesh_brg_persistence \
	brg_tester_persistence -flash_erase brg_bridge_simple -flash_erase

overlay=overlay_pst_conf
RunTestFlash mesh_brg_persistence \
	brg_tester_persistence brg_bridge_simple

# The same test but with PSA crypto
overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash mesh_brg_persistence_psa \
	brg_tester_persistence -flash_erase brg_bridge_simple -flash_erase

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash mesh_brg_persistence_psa \
	brg_tester_persistence brg_bridge_simple
