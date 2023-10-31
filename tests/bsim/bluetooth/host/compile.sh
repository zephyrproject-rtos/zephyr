#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

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
app=tests/bsim/bluetooth/host/adv/extended compile
app=tests/bsim/bluetooth/host/adv/periodic compile
app=tests/bsim/bluetooth/host/adv/periodic conf_file=prj_long_data.conf compile
app=tests/bsim/bluetooth/host/adv/encrypted/css_sample_data compile
app=tests/bsim/bluetooth/host/adv/encrypted/ead_sample compile

app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_lowres.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_collision.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_multiple_conn.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_autoconnect.conf compile
app=tests/bsim/bluetooth/host/att/eatt_notif conf_file=prj.conf compile
app=tests/bsim/bluetooth/host/att/mtu_update compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/client compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/server compile
app=tests/bsim/bluetooth/host/att/retry_on_sec_err/client compile
app=tests/bsim/bluetooth/host/att/retry_on_sec_err/server compile
app=tests/bsim/bluetooth/host/att/sequential/dut compile
app=tests/bsim/bluetooth/host/att/sequential/tester compile
app=tests/bsim/bluetooth/host/att/pipeline/dut compile
app=tests/bsim/bluetooth/host/att/pipeline/tester compile
app=tests/bsim/bluetooth/host/att/long_read compile
app=tests/bsim/bluetooth/host/att/open_close compile

app=tests/bsim/bluetooth/host/gatt/authorization compile
app=tests/bsim/bluetooth/host/gatt/caching compile
app=tests/bsim/bluetooth/host/gatt/general compile
app=tests/bsim/bluetooth/host/gatt/notify compile
app=tests/bsim/bluetooth/host/gatt/notify_multiple compile
app=tests/bsim/bluetooth/host/gatt/settings compile
app=tests/bsim/bluetooth/host/gatt/settings conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/gatt/ccc_store compile
app=tests/bsim/bluetooth/host/gatt/ccc_store conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/gatt/sc_indicate compile

app=tests/bsim/bluetooth/host/iso/cis compile

app=tests/bsim/bluetooth/host/l2cap/general compile
app=tests/bsim/bluetooth/host/l2cap/userdata compile
app=tests/bsim/bluetooth/host/l2cap/stress compile
app=tests/bsim/bluetooth/host/l2cap/split/dut compile
app=tests/bsim/bluetooth/host/l2cap/split/tester compile
app=tests/bsim/bluetooth/host/l2cap/credits compile
app=tests/bsim/bluetooth/host/l2cap/credits conf_file=prj_ecred.conf compile
app=tests/bsim/bluetooth/host/l2cap/credits_seg_recv compile
app=tests/bsim/bluetooth/host/l2cap/credits_seg_recv conf_file=prj_ecred.conf compile
app=tests/bsim/bluetooth/host/l2cap/frags compile
app=tests/bsim/bluetooth/host/l2cap/send_on_connect compile
app=tests/bsim/bluetooth/host/l2cap/send_on_connect conf_file=prj_ecred.conf compile

app=tests/bsim/bluetooth/host/misc/disable compile
app=tests/bsim/bluetooth/host/misc/disconnect/dut compile
app=tests/bsim/bluetooth/host/misc/disconnect/tester compile
app=tests/bsim/bluetooth/host/misc/conn_stress/central compile
app=tests/bsim/bluetooth/host/misc/conn_stress/peripheral compile
app=tests/bsim/bluetooth/host/misc/unregister_conn_cb compile

app=tests/bsim/bluetooth/host/privacy/central compile
app=tests/bsim/bluetooth/host/privacy/peripheral compile
app=tests/bsim/bluetooth/host/privacy/peripheral conf_file=prj_rpa_sharing.conf compile
app=tests/bsim/bluetooth/host/privacy/device compile
app=tests/bsim/bluetooth/host/privacy/legacy compile

app=tests/bsim/bluetooth/host/security/bond_overwrite_allowed compile
app=tests/bsim/bluetooth/host/security/bond_overwrite_denied compile
app=tests/bsim/bluetooth/host/security/bond_per_connection compile
app=tests/bsim/bluetooth/host/security/ccc_update compile
app=tests/bsim/bluetooth/host/security/ccc_update conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/security/id_addr_update/central compile
app=tests/bsim/bluetooth/host/security/id_addr_update/peripheral compile
app=tests/bsim/bluetooth/host/security/security_changed_callback compile

app=tests/bsim/bluetooth/host/id/settings compile

wait_for_background_jobs
