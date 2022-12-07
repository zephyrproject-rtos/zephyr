#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

simulation_id="notify_multiple"
verbosity_level=2
process_ids=""; exit_code=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 120 $@ & process_ids="$process_ids $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_notify_multiple_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

for process_id in $process_ids; do
  wait $process_id || let "exit_code=$?"
done
exit $exit_code #the last exit code != 0
