#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest blob_no_rsp_xfer_get \
	blob_cli_fail_on_no_rsp \
	blob_srv_fail_on_xfer_get \
	blob_srv_fail_on_xfer_get -- -argstest msg-fail-type=1
