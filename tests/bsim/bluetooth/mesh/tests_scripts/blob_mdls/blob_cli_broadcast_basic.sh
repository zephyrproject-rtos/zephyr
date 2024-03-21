#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

conf=prj_mesh1d1_conf
RunTest blob_broadcast_basic blob_cli_broadcast_basic

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest blob_broadcast_basic_psa blob_cli_broadcast_basic
