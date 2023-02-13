#!/usr/bin/env bash
#
# Copyright (c) 2022 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="ias"
VERBOSITY_LEVEL=2
PROCESS_IDS=""; EXIT_CODE=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 20 $@ & PROCESS_IDS="$PROCESS_IDS $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running IAS main (API) test =========\n\n"

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=ias -rs=23

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=ias_client -rs=6

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done

exit $EXIT_CODE #the last exit code != 0
