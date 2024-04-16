#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test DFU Slot API. This test tests that the APIs are idempotent.
conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest dfu_slot_idempotency dfu_dist_dfu_slot_idempotency

conf=prj_mesh1d1_conf
overlay="overlay_pst_conf_overlay_psa_conf"
RunTest dfu_slot_idempotency_psa dfu_dist_dfu_slot_idempotency
