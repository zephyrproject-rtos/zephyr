#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Usage:
# one script instance per device, e.g. to run gdb on the client:
# `_notify-debug.sh client debug`
# `_notify-debug.sh server`
# `_notify-debug.sh`
#
# GDB can be run on the two devices at the same time without issues, just append
# `debug` when running the script.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="notify_multiple"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

if [[ $2 == "debug" ]]; then
  GDB_P="gdb --args "
fi

if [[ $1 == "client" ]]; then
$GDB_P ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client

elif [[ $1 == "server" ]]; then
$GDB_P ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server

else
./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

fi
