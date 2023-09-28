#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.

overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check persistence_cfg_save -flash_erase \
    -- -argstest stack-cfg=0

overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=0

overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check persistence_cfg_save -flash_erase \
    -- -argstest stack-cfg=1

overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=1

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check_1d1 persistence_cfg_save -flash_erase \
     -- -argstest stack-cfg=0

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check_1d1 persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=0

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check_1d1 persistence_cfg_save -flash_erase \
    -- -argstest stack-cfg=1

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTestFlash mesh_pst_cfg_check_1d1 persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=1

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_cfg_check_psa persistence_cfg_save -flash_erase \
    -- -argstest stack-cfg=0

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_cfg_check_psa persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=0

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_cfg_check_psa persistence_cfg_save -flash_erase \
    -- -argstest stack-cfg=1

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash mesh_pst_cfg_check_psa persistence_cfg_load -flash_rm \
    -- -argstest stack-cfg=1
