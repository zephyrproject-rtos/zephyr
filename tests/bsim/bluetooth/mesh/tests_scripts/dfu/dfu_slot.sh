#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test DFU Slot API. This tests are meant to be executed in sequence to work properly.
#  - First test creates slots and saves them in storage.
#  - Second test is rebooted device that restores saved slots from storage and checks their state.
#  - Third test is rebooted device that deletes all previously added slots
#    and verifies they do not exist.
#  - Fourth test is rebooted device that verifies if removing all slots also removed them
#    from storage.
overlay=overlay_pst_conf
RunTestFlash dfu_slot dfu_dist_dfu_slot_create -flash_erase

RunTestFlash dfu_slot dfu_dist_dfu_slot_create_recover

RunTestFlash dfu_slot dfu_dist_dfu_slot_delete_all

RunTestFlash dfu_slot dfu_dist_dfu_slot_check_delete_all -flash_rm

overlay="overlay_pst_conf_overlay_psa_conf"
RunTestFlash dfu_slot_psa dfu_dist_dfu_slot_create -flash_erase

RunTestFlash dfu_slot_psa dfu_dist_dfu_slot_create_recover

RunTestFlash dfu_slot_psa dfu_dist_dfu_slot_delete_all

RunTestFlash dfu_slot_psa dfu_dist_dfu_slot_check_delete_all -flash_rm
