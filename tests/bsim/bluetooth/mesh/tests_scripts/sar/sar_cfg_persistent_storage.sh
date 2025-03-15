#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Note:
# Tests must be added in sequence.
# First test sets SAR TX/RX configuration.
# Second test restores it from flash and checks if configuration persisted.
overlay=overlay_pst_conf
RunTestFlash sar_persistence sar_srv_cfg_store -flash_erase
RunTestFlash sar_persistence sar_srv_cfg_restore -flash_rm

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTestFlash sar_persistence_psa sar_srv_cfg_store -flash_erase
RunTestFlash sar_persistence_psa sar_srv_cfg_restore -flash_rm
