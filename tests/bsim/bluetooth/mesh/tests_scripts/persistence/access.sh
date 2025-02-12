#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_data_save -flash_erase

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_data_load \
	-- -argstest access-cfg=configured

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_sub_overwrite \
	-- -argstest access-cfg=configured

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_data_load \
	-- -argstest access-cfg=new-subs

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_data_remove \
	-- -argstest access-cfg=new-subs

overlay=overlay_pst_conf
RunTestFlash mesh_pst_access_data_check persistence_access_data_load -flash_rm \
	-- -argstest access-cfg=not-configured

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_data_save -flash_erase

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_data_load \
	-- -argstest access-cfg=configured

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_sub_overwrite \
	-- -argstest access-cfg=configured

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_data_load \
	-- -argstest access-cfg=new-subs

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_data_remove \
	-- -argstest access-cfg=new-subs

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_access_data_check_psa persistence_access_data_load -flash_rm \
	-- -argstest access-cfg=not-configured
