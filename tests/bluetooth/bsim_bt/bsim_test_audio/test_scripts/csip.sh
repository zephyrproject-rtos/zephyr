#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Basic CSIP test. A set coordinator connects to multiple set members
# lock thems, unlocks them and disconnects.
SIMULATION_ID="csip"
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

# NORMAL TEST
printf "\n\n======== Running normal test ========\n\n"
Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
  -RealEncryption=1 -rs=1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member \
  -RealEncryption=1 -rs=2 -argstest rank 1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member \
  -RealEncryption=1 -rs=3 -argstest rank 2

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member \
  -RealEncryption=1 -rs=4 -argstest rank 3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done

PROCESS_IDS="";

# TEST WITH FORCE RELEASE
printf "\n\n======== Running test with forced release of lock ========\n\n"
Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
  -RealEncryption=1 -rs=1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member \
  -RealEncryption=1 -rs=2 -argstest rank 1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member \
  -RealEncryption=1 -rs=3 -argstest rank 2

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_release \
  -RealEncryption=1 -rs=4 -argstest rank 3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done

# TEST WITH SIRK ENC
printf "\n\n======== Running test with SIRK encrypted ========\n\n"
Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
  -RealEncryption=1 -rs=1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_enc \
  -RealEncryption=1 -rs=2 -argstest rank 1

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_enc \
  -RealEncryption=1 -rs=3 -argstest rank 2

Execute ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_enc \
  -RealEncryption=1 -rs=4 -argstest rank 3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

for PROCESS_ID in $PROCESS_IDS; do
  wait $PROCESS_ID || let "EXIT_CODE=$?"
done
exit $EXIT_CODE #the last exit code != 0
