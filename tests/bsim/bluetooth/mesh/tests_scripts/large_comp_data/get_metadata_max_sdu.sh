#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the large composition data (LCD) server model is able to transmit
# access messages with a total size of 380-bytes (including opcode).
#
# Test procedure:
# 0. Provisioning and setup. Server and client has same comp data.
# 1. Client requests a max SDU metadata status message from server.
# 2. Client fetch its local metadata.
# 3. When server status arrive, assure that the received message length
#    is 378 bytes (380 bytes access payload).
# 4. Remove status field data and compare received metadata with
#    local metadata data.
overlay=overlay_pst_conf
RunTest mesh_lcd_test_max_metadata_access_payload \
	lcd_cli_max_sdu_metadata_request lcd_srv_metadata_status_respond

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTest mesh_lcd_test_max_metadata_access_payload_psa \
	lcd_cli_max_sdu_metadata_request lcd_srv_metadata_status_respond
