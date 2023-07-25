#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in sequence.
# First test sets SAR TX/RX configuration.
# Second test restores it from flash and checks if configuration persisted.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest sar_persistence sar_srv_cfg_store
RunTest sar_persistence sar_srv_cfg_restore
