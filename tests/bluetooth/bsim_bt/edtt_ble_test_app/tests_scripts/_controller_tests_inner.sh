#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Common part of the test scripts for some of the EDTT tests
# in which 2 controller only builds of the stack are run against each other
VERBOSITY_LEVEL=2
PROCESS_IDS=""; EXIT_CODE=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 300 $@ & PROCESS_IDS="$PROCESS_IDS $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${EDTT_PATH:?EDTT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

cd ${EDTT_PATH}

Execute ./src/edttool.py -s=${SIMULATION_ID} -d=0 --transport bsim \
  -T $TEST_MODULE -C $TEST_FILE -v=${VERBOSITY_LEVEL}

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_device_EDTT_bridge -s=${SIMULATION_ID} -d=0 -AutoTerminate \
  -RxWait=2.5e3 -D=2 -dev0=1 -dev1=2 -v=${VERBOSITY_LEVEL}

Execute \
  ./bs_${BOARD}_tests_bluetooth_bsim_bt_edtt_ble_test_app_hci_test_app_prj_conf\
  -s=${SIMULATION_ID} -d=1 -v=${VERBOSITY_LEVEL} -RealEncryption=1

Execute \
  ./bs_${BOARD}_tests_bluetooth_bsim_bt_edtt_ble_test_app_hci_test_app_prj_conf\
  -s=${SIMULATION_ID} -d=2 -v=${VERBOSITY_LEVEL} -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=3 -sim_length=3600e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done
exit $EXIT_CODE #the last exit code != 0
