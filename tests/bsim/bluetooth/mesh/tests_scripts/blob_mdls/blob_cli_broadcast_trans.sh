#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest blob_broadcast_trans blob_cli_broadcast_trans

overlay=overlay_psa_conf
RunTest blob_broadcast_trans_psa blob_cli_broadcast_trans
