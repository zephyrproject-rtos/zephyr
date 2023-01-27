#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim_bt tests

#set -x #uncomment this line for debugging
set -ue

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_bt_out}"
BOARD="${BOARD:-nrf52_bsim}"
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/compile.source

app=tests/bluetooth/bsim_bt/bsim_test_adv_resume compile &
app=tests/bluetooth/bsim_bt/bsim_test_adv_resume conf_file=prj_2.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_bond_overwrite_allowed compile &
app=tests/bluetooth/bsim_bt/bsim_test_bond_overwrite_denied compile &
app=tests/bluetooth/bsim_bt/bsim_test_notify compile &
app=tests/bluetooth/bsim_bt/bsim_test_notify_multiple compile &
app=tests/bluetooth/bsim_bt/bsim_test_eatt_notif conf_file=prj.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_gatt_caching compile &
app=tests/bluetooth/bsim_bt/bsim_test_eatt conf_file=prj_collision.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_eatt conf_file=prj_multiple_conn.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_eatt conf_file=prj_autoconnect.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_app conf_file=prj_split.conf \
  compile &
app=tests/bluetooth/bsim_bt/bsim_test_app conf_file=prj_split_privacy.conf \
  compile &
app=tests/bluetooth/bsim_bt/bsim_test_app conf_file=prj_split_low_lat.conf \
  compile &
app=tests/bluetooth/bsim_bt/bsim_test_mtu_update compile &
app=tests/bluetooth/bsim_bt/bsim_test_multiple compile &
app=tests/bluetooth/bsim_bt/bsim_test_advx compile &
app=tests/bluetooth/bsim_bt/bsim_test_adv_chain compile &
app=tests/bluetooth/bsim_bt/bsim_test_gatt compile &
app=tests/bluetooth/bsim_bt/bsim_test_gatt_write compile &
app=tests/bluetooth/bsim_bt/bsim_test_l2cap compile &
app=tests/bluetooth/bsim_bt/bsim_test_l2cap_userdata compile &
app=tests/bluetooth/bsim_bt/bsim_test_l2cap_stress compile &
app=tests/bluetooth/bsim_bt/bsim_test_iso compile &
app=tests/bluetooth/bsim_bt/bsim_test_iso conf_file=prj_vs_dp.conf \
  compile &
app=tests/bluetooth/bsim_bt/bsim_test_audio compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app \
  conf_file=prj_dut_llcp.conf compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app \
  conf_file=prj_tst_llcp.conf compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app \
  conf_file=prj_dut.conf compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app \
  conf_file=prj_tst.conf compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/gatt_test_app \
  conf_file=prj.conf compile &
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/gatt_test_app \
  conf_file=prj_llcp.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_mesh compile &
app=tests/bluetooth/bsim_bt/bsim_test_mesh conf_overlay=overlay_low_lat.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_mesh conf_overlay=overlay_pst.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_mesh conf_overlay=overlay_gatt.conf compile &
app=tests/bluetooth/bsim_bt/bsim_test_disable compile &
app=tests/bluetooth/bsim_bt/bsim_test_per_adv compile &

wait
