#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_first.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-legacy_adv.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-legacy_adv_acl_first.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_group.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_group_acl_first.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-peripheral_cis.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_first_ft_per_skip_2_se.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_first_ft_per_skip_4_se.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_first_ft_cen_skip_2_se.conf compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_first_ft_cen_skip_4_se.conf compile

wait_for_background_jobs
