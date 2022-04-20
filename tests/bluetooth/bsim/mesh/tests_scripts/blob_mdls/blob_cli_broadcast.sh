#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

conf=prj_mesh1d1_conf
RunTest blob_broadcast \
	blob_cli_broadcast_basic

conf=prj_mesh1d1_conf
RunTest blob_broadcast \
	blob_cli_broadcast_trans

conf=prj_mesh1d1_conf
RunTest blob_broadcast \
	blob_cli_broadcast_unicast_seq

conf=prj_mesh1d1_conf
RunTest blob_broadcast \
	blob_cli_broadcast_unicast
