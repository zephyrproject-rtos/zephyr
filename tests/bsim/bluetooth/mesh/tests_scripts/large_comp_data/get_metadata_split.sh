#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the the LCD server model is able to split the
# metadata when the total size exceeds the maximum access message size.
#
# Test procedure:
# 0. Provisioning and setup. Server and client has same comp data.
# 1. Client requests the first X bytes from server's metadata.
# 2. Client fetches its local metadata.
# 3. When the server status arrive, remove status field data and compare
#    received metadata with corresponding bytes in local data.
# 4. Client requests the next X bytes from server's metadata.
# 5. When the server status arrive, remove status field data and compare
#    received metadata with corresponding bytes in local data.
# 6. Client merges the two samples and checks that the collected data is
#    correctly merged, continuous, and matches its local metadata.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest mesh_lcd_test_split_metadata \
	lcd_cli_split_metadata_request lcd_srv_metadata_status_respond

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTest mesh_lcd_test_split_metadata_psa \
	lcd_cli_split_metadata_request lcd_srv_metadata_status_respond
