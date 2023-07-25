#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# The test instance sequence must stay as it is due to addressing scheme
conf=prj_mesh1d1_conf
RunTest blob_fail \
	blob_cli_fail_on_persistency \
	blob_srv_fail_on_block_start\
	blob_srv_fail_on_block_get \
	blob_srv_fail_on_xfer_get \
	blob_srv_fail_on_nothing
