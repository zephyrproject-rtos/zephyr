#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# GATT regression tests based on the EDTTool
SIMULATION_ID="edtt_gatt_llcp"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=300

CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)"

: "${EDTT_PATH:?EDTT_PATH must be defined}"

cd ${EDTT_PATH}

Execute ./src/edttool.py -s=${SIMULATION_ID} -d=0 --transport bsim \
  -T gatt_verification -C "${CWD}/gatt.llcp.test_list" -v=${VERBOSITY_LEVEL} \
   -D=2 -devs 1 2 -RxWait=2.5e3

cd ${BSIM_OUT_PATH}/bin

Execute \
 ./bs_${BOARD}_tests_bsim_bluetooth_ll_edtt_hci_test_app_prj_tst_llcp_conf\
  -s=${SIMULATION_ID} -d=1 -v=${VERBOSITY_LEVEL} -RealEncryption=1

Execute \
 ./bs_${BOARD}_tests_bsim_bluetooth_ll_edtt_gatt_test_app_prj_llcp_conf\
  -s=${SIMULATION_ID} -d=2 -v=${VERBOSITY_LEVEL} -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=3 -sim_length=3600e6 $@

wait_for_background_jobs
