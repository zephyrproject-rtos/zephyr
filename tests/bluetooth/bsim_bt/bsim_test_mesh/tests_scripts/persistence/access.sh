#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_data_save

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_data_load --\
	-argstest access-cfg=configured

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_sub_overwrite --\
	-argstest access-cfg=configured

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_data_load --\
	-argstest access-cfg=new-subs

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_data_remove --\
	-argstest access-cfg=new-subs

conf=prj_pst_conf
RunTest mesh_persistence_access_data_check persistence_access_data_load --\
	-argstest access-cfg=not-configured
