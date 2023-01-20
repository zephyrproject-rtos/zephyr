#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in pairs and in sequence.
# First test: saves data; second test: verifies it.

overlay=overlay_pst_conf
RunTest mesh_persistence_cfg_check persistence_cfg_save -- -argstest stack-cfg=0

overlay=overlay_pst_conf
RunTest mesh_persistence_cfg_check persistence_cfg_load -- -argstest stack-cfg=0

overlay=overlay_pst_conf
RunTest mesh_persistence_cfg_check persistence_cfg_save -- -argstest stack-cfg=1

overlay=overlay_pst_conf
RunTest mesh_persistence_cfg_check persistence_cfg_load -- -argstest stack-cfg=1
