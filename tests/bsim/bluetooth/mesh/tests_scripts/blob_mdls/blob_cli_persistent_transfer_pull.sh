#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that BLOB Client continues BLOB Transfer after one target timed out while sending chunks.
RunTest blob_pst_pull \
	blob_cli_trans_persistency_pull \
	blob_srv_trans_persistency_pull \
	blob_srv_trans_persistency_pull

overlay=overlay_psa_conf
RunTest blob_pst_pull_psa \
	blob_cli_trans_persistency_pull \
	blob_srv_trans_persistency_pull \
	blob_srv_trans_persistency_pull
