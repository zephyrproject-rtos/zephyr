#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the large composition data (LCD) server model is able to transmit
# access messages with a total size of 380-bytes (including opcode).
#
# Test procedure:
# 0. Provisioning and setup. Server and client has same comp data.
# 1. Client requests a max SDU from server's composition data.
# 2. Client fetch its local comp data.
# 3. When server status arrive, remove status field data and compare received
#    comp data with local comp data and assure that the received message length
#    is 378 bytes (380 bytes access payload).
overlay=overlay_pst_conf
RunTest mesh_lcd_test_max_access_payload \
	lcd_cli_max_sdu_comp_data_request lcd_srv_comp_data_status_respond

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTest mesh_lcd_test_max_access_payload_psa \
	lcd_cli_max_sdu_comp_data_request lcd_srv_comp_data_status_respond
