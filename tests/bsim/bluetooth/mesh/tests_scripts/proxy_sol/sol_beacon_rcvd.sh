#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test procedure:
# 1. Initialize and configure tester and iut instances.
# 3. Tester sends a solicitation PDU.
# 4. Tester scans for beacons and expects to receive at least one private net ID advertisement.
overlay="overlay_pst_conf_overlay_gatt_conf"
RunTest proxy_sol_beacon_rcvd \
	proxy_sol_tester_beacon_rcvd \
	proxy_sol_iut_beacon_send

overlay="overlay_pst_conf_overlay_ss_conf_overlay_gatt_conf_overlay_psa_conf"
RunTest proxy_sol_beacon_rcvd_psa \
	proxy_sol_tester_beacon_rcvd \
	proxy_sol_iut_beacon_send
