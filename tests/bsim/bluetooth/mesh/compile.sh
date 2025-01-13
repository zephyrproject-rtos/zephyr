#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/mesh compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_pst.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_gatt.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_gatt_separate.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_low_lat.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_psa.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_workq_sys.conf compile
app=tests/bsim/bluetooth/mesh conf_overlay=overlay_multi_adv_sets.conf compile
app=tests/bsim/bluetooth/mesh \
  conf_overlay="overlay_pst.conf;overlay_ss.conf;overlay_psa.conf" compile
app=tests/bsim/bluetooth/mesh conf_overlay="overlay_gatt.conf;overlay_psa.conf" compile
app=tests/bsim/bluetooth/mesh conf_overlay="overlay_gatt.conf;overlay_workq_sys.conf" compile
app=tests/bsim/bluetooth/mesh conf_overlay="overlay_low_lat.conf;overlay_psa.conf" compile
app=tests/bsim/bluetooth/mesh conf_overlay="overlay_gatt.conf;overlay_low_lat.conf" compile
app=tests/bsim/bluetooth/mesh conf_overlay="overlay_pst.conf;overlay_gatt.conf" compile
app=tests/bsim/bluetooth/mesh \
  conf_overlay="overlay_gatt.conf;overlay_multi_adv_sets.conf" compile
app=tests/bsim/bluetooth/mesh \
  conf_overlay="overlay_pst.conf;overlay_ss.conf;overlay_gatt.conf;overlay_psa.conf" compile
app=tests/bsim/bluetooth/mesh \
  conf_overlay="overlay_pst.conf;overlay_gatt.conf;overlay_workq_sys.conf" compile
wait_for_background_jobs
