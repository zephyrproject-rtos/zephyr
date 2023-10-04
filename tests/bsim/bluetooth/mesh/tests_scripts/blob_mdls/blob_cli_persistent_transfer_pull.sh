#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that BLOB Client continues BLOB Transfer after one target timed out while sending chunks.
conf=prj_mesh1d1_conf
RunTest blob_pst_pull \
	blob_cli_trans_persistency_pull \
	blob_srv_trans_persistency_pull \
	blob_srv_trans_persistency_pull
