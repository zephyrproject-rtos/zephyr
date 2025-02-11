#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

## Test persistence of Proxy Solicitation replay protection list

# Test procedure:
# 1. Initialize and configure tester and IUT instances.
# 2. Tester scans for beacons for a few seconds to verify that none are
#    currently advertised from the IUT.
# 3. Tester sends a modified solicitation PDU with a fixed sequence number.
# 4. Tester scans for beacons and expects to receive at least one private net ID advertisement.
# 5. IUT stops all ongoing advertisements (timeout).
# 6. Tester re-sends the same fixed solicitation PDU, and scans to verify that
#    no beacons are received.
# 7. The IUT is rebooted.
# 8. Tester re-sends the same fixed solicitation PDU, and scans to verify that no
#    beacons are received.
# 9. Tester sends a new solicitation PDU, with a new and unique sequence number,
#    and scans to verify that a beacon is received.
overlay="overlay_pst_conf_overlay_gatt_conf"
RunTest mesh_srpl_replay_attack \
	proxy_sol_tester_immediate_replay_attack \
	proxy_sol_iut_immediate_replay_attack \
	-flash=../results/mesh_srpl_replay_attack/flash.bin -flash_erase

overlay="overlay_pst_conf_overlay_gatt_conf"
RunTest mesh_srpl_replay_attack \
	proxy_sol_tester_power_replay_attack \
	proxy_sol_iut_power_replay_attack \
	-flash=../results/mesh_srpl_replay_attack/flash.bin -flash_rm

overlay="overlay_pst_conf_overlay_gatt_conf_overlay_psa_conf"
RunTest mesh_srpl_replay_attack_psa \
	proxy_sol_tester_immediate_replay_attack \
	proxy_sol_iut_immediate_replay_attack \
	-flash=../results/mesh_srpl_replay_attack_psa/flash.bin -flash_erase

overlay="overlay_pst_conf_overlay_gatt_conf_overlay_psa_conf"
RunTest mesh_srpl_replay_attack_psa \
	proxy_sol_tester_power_replay_attack \
	proxy_sol_iut_power_replay_attack \
	-flash=../results/mesh_srpl_replay_attack_psa/flash.bin -flash_rm
