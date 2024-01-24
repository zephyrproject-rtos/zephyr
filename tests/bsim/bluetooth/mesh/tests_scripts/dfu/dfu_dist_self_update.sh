#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

overlay=overlay_pst_conf
RunTest dfu_self_update dfu_dist_dfu_self_update -- -argstest targets=1

overlay="overlay_pst_conf_overlay_psa_conf"
RunTest dfu_self_update_psa dfu_dist_dfu_self_update -- -argstest targets=1
