#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests

#set -x #uncomment this line for debugging
set -ue

: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"

BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/host/adv/resume compile
app=tests/bsim/bluetooth/host/adv/resume conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/adv/chain compile
app=tests/bsim/bluetooth/host/adv/periodic compile
app=tests/bsim/bluetooth/host/adv/periodic conf_file=prj_long_data.conf compile
app=tests/bsim/bluetooth/host/adv/encrypted/css_sample_data compile
app=tests/bsim/bluetooth/host/adv/encrypted/ead_sample compile

app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_collision.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_multiple_conn.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_autoconnect.conf compile
app=tests/bsim/bluetooth/host/att/eatt_notif conf_file=prj.conf compile
app=tests/bsim/bluetooth/host/att/mtu_update compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/client compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/server compile

app=tests/bsim/bluetooth/host/gatt/caching compile
app=tests/bsim/bluetooth/host/gatt/general compile
app=tests/bsim/bluetooth/host/gatt/notify compile
app=tests/bsim/bluetooth/host/gatt/notify_multiple compile
app=tests/bsim/bluetooth/host/gatt/settings compile
app=tests/bsim/bluetooth/host/gatt/settings conf_file=prj_2.conf compile

app=tests/bsim/bluetooth/host/l2cap/general compile
app=tests/bsim/bluetooth/host/l2cap/userdata compile
app=tests/bsim/bluetooth/host/l2cap/stress compile

app=tests/bsim/bluetooth/host/misc/disable compile

app=tests/bsim/bluetooth/host/privacy/central compile
app=tests/bsim/bluetooth/host/privacy/peripheral compile
app=tests/bsim/bluetooth/host/privacy/device compile

app=tests/bsim/bluetooth/host/security/bond_overwrite_allowed compile
app=tests/bsim/bluetooth/host/security/bond_overwrite_denied compile

wait_for_background_jobs
