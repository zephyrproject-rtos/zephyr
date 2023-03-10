#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that confirm step fails with all variants of firmware effect
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_mixed_fail dfu_dist_dfu dfu_target_dfu_unprov dfu_target_dfu_no_change \
	dfu_target_dfu_new_comp_rpr dfu_target_dfu_new_comp_no_rpr -- -argstest targets=4 \
	fail-confirm=1
