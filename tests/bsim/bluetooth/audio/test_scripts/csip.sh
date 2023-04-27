#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Basic CSIP test. A set coordinator connects to multiple set members
# lock thems, unlocks them and disconnects.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=20

cd ${BSIM_OUT_PATH}/bin

## NORMAL TEST
#printf "\n\n======== Running normal test ========\n\n"
#
#SIMULATION_ID="csip"
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
#  -RealEncryption=1 -rs=1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member \
#  -RealEncryption=1 -rs=2 -argstest rank 1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member \
#  -RealEncryption=1 -rs=3 -argstest rank 2
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member \
#  -RealEncryption=1 -rs=4 -argstest rank 3
#
## Simulation time should be larger than the WAIT_TIME in common.h
#Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
#  -D=4 -sim_length=60e6 $@
#
#wait_for_background_jobs
#
## TEST WITH FORCE RELEASE
#
#SIMULATION_ID="csip_forced_release"
#
#printf "\n\n======== Running test with forced release of lock ========\n\n"
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
#  -RealEncryption=1 -rs=1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member \
#  -RealEncryption=1 -rs=2 -argstest rank 1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member \
#  -RealEncryption=1 -rs=3 -argstest rank 2
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_release \
#  -RealEncryption=1 -rs=4 -argstest rank 3
#
## Simulation time should be larger than the WAIT_TIME in common.h
#Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
#  -D=4 -sim_length=60e6 $@
#
#wait_for_background_jobs
#
## TEST WITH SIRK ENC
#
#SIMULATION_ID="csip_sirk_encrypted"
#
#printf "\n\n======== Running test with SIRK encrypted ========\n\n"
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
#  -RealEncryption=1 -rs=1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_enc \
#  -RealEncryption=1 -rs=2 -argstest rank 1
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_enc \
#  -RealEncryption=1 -rs=3 -argstest rank 2
#
#Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
#  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_enc \
#  -RealEncryption=1 -rs=4 -argstest rank 3
#
## Simulation time should be larger than the WAIT_TIME in common.h
#Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
#  -D=4 -sim_length=60e6 $@
#
#wait_for_background_jobs

# TEST INVALID BEHAVIOUR

SIMULATION_ID="csip_invalid_tests"

##printf "\n\n======== Running test non lockable  ========\n\n"
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator_non_lockable \
##  -RealEncryption=1 -rs=1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_non_lockable \
##  -RealEncryption=1 -rs=2 -argstest rank 1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_non_lockable \
##  -RealEncryption=1 -rs=3 -argstest rank 2
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_non_lockable \
##  -RealEncryption=1 -rs=4 -argstest rank 3
##
### Simulation time should be larger than the WAIT_TIME in common.h
##Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
##  -D=4 -sim_length=60e6 $@
##
##wait_for_background_jobs

##printf "\n\n======== Running test non ranked  ========\n\n"
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator_unranked \
##  -RealEncryption=1 -rs=1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_unranked \
##  -RealEncryption=1 -rs=2 -argstest rank 1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_unranked \
##  -RealEncryption=1 -rs=3 -argstest rank 2
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_unranked \
##  -RealEncryption=1 -rs=4 -argstest rank 3
##
### Simulation time should be larger than the WAIT_TIME in common.h
##Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
##  -D=4 -sim_length=60e6 $@
##
##wait_for_background_jobs

##printf "\n\n======== Running test no size  ========\n\n"
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator_no_size \
##  -RealEncryption=1 -rs=1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_no_size \
##  -RealEncryption=1 -rs=2 -argstest rank 1
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##    -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_no_size \
##  -RealEncryption=1 -rs=3 -argstest rank 2
##
##Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
##  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_no_size \
##  -RealEncryption=1 -rs=4 -argstest rank 3
##
### Simulation time should be larger than the WAIT_TIME in common.h
##Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
##  -D=4 -sim_length=60e6 $@
##
##wait_for_background_jobs

printf "\n\n======== Running test invalid sirk  ========\n\n"
Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator_invalid_sirk \
  -RealEncryption=1 -rs=1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_invalid_sirk_1 \
  -RealEncryption=1 -rs=2 -argstest rank 1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
    -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_invalid_sirk_2 \
  -RealEncryption=1 -rs=3 -argstest rank 2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_invalid_sirk_2 \
  -RealEncryption=1 -rs=4 -argstest rank 3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

wait_for_background_jobs

