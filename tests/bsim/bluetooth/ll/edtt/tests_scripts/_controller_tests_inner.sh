#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

#
# ENVIRONMENT CONFIGURATION
# =========================
#
# This script can be configured with a number of environment variables.
# Values in [] are the default unless overridden.
#
# PROJECT CONFIGURATION
# ---------------------
#     PRJ_CONF:   Default bsim device configuration [prj_dut_conf]
#     PRJ_CONF_1: bsim device 1 configuration [PRJ_CONF]
#     PRJ_CONF_2: bsim device 2 configuration [PRJ_CONF]
#
# VERBOSITY
# ---------
#     VERBOSITY_LEVEL:        Global verbosity [2]
#     VERBOSITY_LEVEL_EDTT:   EDTT verbosity [VERBOSITY_LEVEL]
#     VERBOSITY_LEVEL_BRIDGE: EDTT bridge verbosity [VERBOSITY_LEVEL]
#     VERBOSITY_LEVEL_PHY:    bsim phy verbosity [VERBOSITY_LEVEL]
#     VERBOSITY_LEVEL_DEVS:   Global bsim device verbosity [VERBOSITY_LEVEL]
#     VERBOSITY_LEVEL_DEV1:   bsim device 1 verbosity [VERBOSITY_LEVEL_DEVS]
#     VERBOSITY_LEVEL_DEV1:   bsim device 2 verbosity [VERBOSITY_LEVEL_DEVS]
#
# RR DEBUG SUPPORT
# ----------------
#     RR:   Default run bsim device under rr [0]
#           0: disables; any other value enables.
#     RR_1: Run bsim device 1 under rr [RR]
#     RR_2: Run bsim device 2 under rr [RR]
#


# Common part of the test scripts for some of the EDTT tests
# in which 2 controller only builds of the stack are run against each other
VERBOSITY_LEVEL=${VERBOSITY_LEVEL:-2}
VERBOSITY_LEVEL_EDTT=${VERBOSITY_LEVEL_EDTT:-${VERBOSITY_LEVEL}}
VERBOSITY_LEVEL_BRIDGE=${VERBOSITY_LEVEL_BRIDGE:-${VERBOSITY_LEVEL}}
VERBOSITY_LEVEL_PHY=${VERBOSITY_LEVEL_PHY:-${VERBOSITY_LEVEL}}
VERBOSITY_LEVEL_DEVS=${VERBOSITY_LEVEL_DEVS:-${VERBOSITY_LEVEL}}
VERBOSITY_LEVEL_DEV1=${VERBOSITY_LEVEL_1:-${VERBOSITY_LEVEL_DEVS}}
VERBOSITY_LEVEL_DEV2=${VERBOSITY_LEVEL_2:-${VERBOSITY_LEVEL_DEVS}}


source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

function _Execute(){
  local rr=
  if [ "rr" = "$1" ]; then
    local devno=$2
    shift 2
    local exe=$(basename $1)
    local out=/tmp/rr/$$-${SIMULATION_ID}-${exe}-d_${devno}
    rm -rf ${out}
    rr="rr record -o ${out}"
  fi
  check_program_exists $1
  run_in_background timeout --kill-after=5 -v 800 ${rr} $@
}


: "${EDTT_PATH:?EDTT_PATH must be defined}"

#Give a default value to PRJ_CONF_x if it does not have one yet:
PRJ_CONF="${PRJ_CONF:-prj_dut_conf}"
PRJ_CONF_1="${PRJ_CONF_1:-${PRJ_CONF}}"
PRJ_CONF_2="${PRJ_CONF_2:-${PRJ_CONF}}"

#Give default value to RR_x if it does not have one yet:
RR="${RR:-0}"
RR_1="${RR_1:-${RR}}"
RR_2="${RR_2:-${RR}}"

#Check if rr was requested and is available
if [ "${RR_1}" != "0" -o "${RR_2}" != "0" ]; then
    if [ ! -x "$(command -v rr)" ]; then
      echo 'error: rr cannot be found in $PATH.' >&2
      exit 1
    fi

    #Set RR_ARGS_x based on RR_x
    [ "${RR_1}" != "0" ] && RR_ARGS_1="rr 1"
    [ "${RR_2}" != "0" ] && RR_ARGS_2="rr 2"
fi

cd ${EDTT_PATH}

_Execute ./src/edttool.py -s=${SIMULATION_ID} -d=2 --transport bsim \
  -T $TEST_MODULE -C $TEST_FILE -v=${VERBOSITY_LEVEL_EDTT} -S -l --low-level-device-nbr=3 \
  -D=2 -devs 0 1 -RxWait=2.5e3

cd ${BSIM_OUT_PATH}/bin

_Execute \
  ${RR_ARGS_1} ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_edtt_hci_test_app_${PRJ_CONF_1}\
  -s=${SIMULATION_ID} -d=0 -v=${VERBOSITY_LEVEL_DEV1} -RealEncryption=1

_Execute \
  ${RR_ARGS_2} ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_edtt_hci_test_app_${PRJ_CONF_2}\
  -s=${SIMULATION_ID} -d=1 -v=${VERBOSITY_LEVEL_DEV2} -RealEncryption=1

_Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL_PHY} -s=${SIMULATION_ID} \
  -D=4 -sim_length=3600e6 -dump_imm $@

wait_for_background_jobs
