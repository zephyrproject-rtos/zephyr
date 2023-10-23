#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="bass_server_sync_server_rem"
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

printf "\n\n======== Running BASS Server Sync Server Remove =========\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=bap_scan_delegator_server_sync_server_rem -rs=24 -D=3

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_assistant_server_sync_server_rem -rs=46 -D=3

  Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=bass_broadcaster -rs=69 -D=3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=3 \
  -sim_length=60e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done

exit $EXIT_CODE #the last exit code != 0
