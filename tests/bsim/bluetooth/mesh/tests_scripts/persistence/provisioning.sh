#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.

# SKIP=(persistence_provisioning_data_save)
overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check persistence_provisioning_data_save -flash_erase

# SKIP=(persistence_provisioning_data_load)
overlay=overlay_pst_conf
RunTestFlash mesh_pst_prov_data_check persistence_provisioning_data_load -flash_rm

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_prov_data_check_psa persistence_provisioning_data_save -flash_erase

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_prov_data_check_psa persistence_provisioning_data_load -flash_rm
