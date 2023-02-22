#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests

#set -x #uncomment this line for debugging
set -ue

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"
BOARD="${BOARD:-nrf52_bsim}"
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bluetooth/bsim/compile.source

app=tests/bluetooth/bsim/host/adv/resume compile &
app=tests/bluetooth/bsim/host/adv/resume conf_file=prj_2.conf compile &
app=tests/bluetooth/bsim/host/adv/chain compile &
app=tests/bluetooth/bsim/host/adv/periodic compile &
app=tests/bluetooth/bsim/host/adv/periodic conf_file=prj_long_data.conf compile &

app=tests/bluetooth/bsim/host/att/eatt conf_file=prj_collision.conf compile &
app=tests/bluetooth/bsim/host/att/eatt conf_file=prj_multiple_conn.conf compile &
app=tests/bluetooth/bsim/host/att/eatt conf_file=prj_autoconnect.conf compile &
app=tests/bluetooth/bsim/host/att/eatt_notif conf_file=prj.conf compile &
app=tests/bluetooth/bsim/host/att/mtu_update compile &

app=tests/bluetooth/bsim/host/gatt/caching compile &
app=tests/bluetooth/bsim/host/gatt/general compile &
app=tests/bluetooth/bsim/host/gatt/notify compile &
app=tests/bluetooth/bsim/host/gatt/notify_multiple compile &
app=tests/bluetooth/bsim/host/gatt/settings compile &
app=tests/bluetooth/bsim/host/gatt/settings conf_file=prj_2.conf compile &
app=tests/bluetooth/bsim/host/gatt/write compile &

app=tests/bluetooth/bsim/host/l2cap/general compile &
app=tests/bluetooth/bsim/host/l2cap/userdata compile &
app=tests/bluetooth/bsim/host/l2cap/stress compile &

app=tests/bluetooth/bsim/host/misc/disable compile &
app=tests/bluetooth/bsim/host/misc/multiple_id compile &

app=tests/bluetooth/bsim/host/privacy/central compile &
app=tests/bluetooth/bsim/host/privacy/peripheral compile &
app=tests/bluetooth/bsim/host/privacy/device compile &

app=tests/bluetooth/bsim/host/security/bond_overwrite_allowed compile &
app=tests/bluetooth/bsim/host/security/bond_overwrite_denied compile &

wait
