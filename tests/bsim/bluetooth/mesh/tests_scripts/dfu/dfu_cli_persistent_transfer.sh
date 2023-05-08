#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# The test instance sequence must stay as it is due to addressing scheme
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_persistency \
	dfu_cli_fail_on_persistency \
	dfu_target_fail_on_metadata \
	dfu_target_fail_on_caps_get \
	dfu_target_fail_on_update_get \
	dfu_target_fail_on_verify \
	dfu_target_fail_on_apply \
	dfu_target_fail_on_nothing
