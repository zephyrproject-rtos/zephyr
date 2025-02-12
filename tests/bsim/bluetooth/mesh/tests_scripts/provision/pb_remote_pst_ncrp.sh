#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Node Composition Refresh procedure with persistence storage:
# 1. Prepare for Composition Data change.
#    PB-Remote client (1st device):
#     - provision the 2nd device over PB-Adv
#     - provision the 3rd device over PB-Remote
#    PB-Remote server (2nd device):
#     - wait for being provisioned
#     - run PB-Remote bearer
#    PB-Remote server (3rd device):
#     - wait for being provisioned
#     - call bt_mesh_comp_change_prepare() to prepare for Composition Data change
# 2. Verify Node Composition Refresh procedure.
#    PB-Remote client (1st device):
#     - check that Composition Data pages 0 (old comp data) and 128 (new comp data) are different
#     - check that Composition Data pages 1 (old comp data) and 129 (new comp data) are different
#     - run Node Composition Refresh procedure on the 3rd device
#     - verify Composition Data pages 0 (new comp data) and 128 (same as page 0)
#     - verify Composition Data pages 1 (new comp data) and 129 (same as page 1)
#    PB-Remote server (3rd device):
#     - start with a new Composition Data
# 3. Verify that old settings are removed on the 3rd device after Composition Data change.
#    PB-Remote client (1st device):
#     - run Node Composition Refresh procedure again and expect it to fail
#    PB-Remote server (3rd device):
#     - verify that the device is not re-provisioned again.

# Step 1
overlay=overlay_pst_conf
RunTestFlash mesh_prov_pst_pb_remote_ncrp \
	prov_provisioner_pb_remote_client_ncrp_provision -flash_erase \
	prov_device_pb_remote_server_unproved -flash_erase \
	prov_device_pb_remote_server_ncrp_prepare -flash_erase

# Step 2
overlay=overlay_pst_conf
RunTestFlash mesh_prov_pst_pb_remote_ncrp \
	prov_provisioner_pb_remote_client_ncrp \
	prov_device_pb_remote_server_proved \
	prov_device_pb_remote_server_ncrp

# Step 3
overlay=overlay_pst_conf
RunTestFlash mesh_prov_pst_pb_remote_ncrp \
	prov_provisioner_pb_remote_client_ncrp_second_time -flash_rm \
	prov_device_pb_remote_server_proved -flash_rm \
	prov_device_pb_remote_server_ncrp_second_time -flash_rm

# The same test but with PSA crypto
# Step 1
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_prov_pst_pb_remote_ncrp_psa \
	prov_provisioner_pb_remote_client_ncrp_provision -flash_erase \
	prov_device_pb_remote_server_unproved -flash_erase \
	prov_device_pb_remote_server_ncrp_prepare -flash_erase

# Step 2
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_prov_pst_pb_remote_ncrp_psa \
	prov_provisioner_pb_remote_client_ncrp \
	prov_device_pb_remote_server_proved \
	prov_device_pb_remote_server_ncrp

# Step 3
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_prov_pst_pb_remote_ncrp_psa \
	prov_provisioner_pb_remote_client_ncrp_second_time -flash_rm \
	prov_device_pb_remote_server_proved -flash_rm \
	prov_device_pb_remote_server_ncrp_second_time -flash_rm
