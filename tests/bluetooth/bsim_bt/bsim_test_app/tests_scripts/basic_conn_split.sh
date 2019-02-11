#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Basic connection test: a central connects to a peripheral and expects a
# notification, using the split controller (ULL LLL)
SIMULATION_ID="basic_conn_split"
VERBOSITY_LEVEL=2
PROCESS_IDS=""; EXIT_CODE=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 5 $@ & PROCESS_IDS="$PROCESS_IDS $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_app_prj_split_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -RealEncryption=0 \
  -testid=peripheral -rs=23

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_app_prj_split_conf\
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -RealEncryption=0 \
  -testid=central -rs=6

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=20e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done
exit $EXIT_CODE #the last exit code != 0
