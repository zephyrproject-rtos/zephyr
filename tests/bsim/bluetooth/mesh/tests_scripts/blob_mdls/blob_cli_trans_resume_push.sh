#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that BLOB Client can resume a suspended BLOB Transfer in Push mode
conf=prj_mesh1d1_conf
RunTest blob_resume_push blob_cli_trans_resume blob_srv_trans_resume
