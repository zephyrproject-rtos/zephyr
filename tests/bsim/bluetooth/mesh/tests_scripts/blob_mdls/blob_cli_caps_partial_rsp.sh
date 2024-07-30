#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# The test instance sequence must stay as it is due to addressing scheme
RunTest blob_caps_partial_rsp \
	blob_cli_caps_partial_rsp blob_srv_caps_standard blob_srv_caps_no_rsp

overlay=overlay_psa_conf
RunTest blob_caps_partial_rsp_psa \
	blob_cli_caps_partial_rsp blob_srv_caps_standard blob_srv_caps_no_rsp
