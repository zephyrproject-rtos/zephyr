#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the LCD server model is able to split the
# composition data when the total size exceeds the maximum access message size.
#
# Test procedure:
# 0. Provisioning and setup. Server and client has same comp data.
# 1. Client requests a sample exceeding the maximum available payload from the
#    server's composition data.
# 2. Client fetch its local comp data.
# 3. When server status arrive, remove status field data and compare received
#    comp data with corresponding bytes in local comp data.
# 4. Client requests the next sample from server's composition data.
# 5. When server status arrive, remove status field data and compare received
#    comp data with corresponding bytes in local comp data.
# 6. Client merges the two samples and checks that the collected data is
#    correctly merged, continuous, and matches its local comp data.
overlay=overlay_pst_conf
RunTest mesh_lcd_test_comp1_data_split \
	lcd_srv_comp_data_status_respond lcd_cli_split_comp_data_request -- -argstest page=1

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTest mesh_lcd_test_comp1_data_split \
	lcd_srv_comp_data_status_respond lcd_cli_split_comp_data_request -- -argstest page=1
