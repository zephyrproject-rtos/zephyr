#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test if callback `lost_target` is called on every target failing on Apply step and `ended`
# callback is called when all targets are lost at this step.

# The test instance sequence must stay as it is due to addressing scheme
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_all_tgts_lost_on_apply \
	dfu_cli_all_targets_lost_on_apply \
	dfu_target_fail_on_apply \
	dfu_target_fail_on_apply \
	dfu_target_fail_on_apply \
	-- -argstest targets=3

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTest dfu_all_tgts_lost_on_apply_psa \
	dfu_cli_all_targets_lost_on_apply \
	dfu_target_fail_on_apply \
	dfu_target_fail_on_apply \
	dfu_target_fail_on_apply \
	-- -argstest targets=3
