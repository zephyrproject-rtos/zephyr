#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test DFU Slot API. This test tests slot reservation APIs.
overlay=overlay_pst_conf
RunTest dfu_slot_reservation dfu_dist_dfu_slot_reservation

overlay="overlay_pst_conf_overlay_psa_conf"
RunTest dfu_slot_reservation_psa dfu_dist_dfu_slot_reservation
