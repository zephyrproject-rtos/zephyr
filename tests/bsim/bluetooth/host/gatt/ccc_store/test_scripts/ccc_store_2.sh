#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name='ccc_store'
test_exe="bs_${BOARD}_tests_bsim_bluetooth_host_gatt_${test_name}_prj_2_conf"
simulation_id="${test_name}_2"
verbosity_level=2
EXECUTE_TIMEOUT=30

cd ${BSIM_OUT_PATH}/bin

if [ "${1}" != 'debug0' ]; then
  Execute "./${test_exe}" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central \
    -flash="${simulation_id}_client.log.bin" -flash_rm -argstest 10
fi

if [ "${1}" != 'debug1' ]; then
  Execute "./${test_exe}" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral \
    -flash="${simulation_id}_server.log.bin" -flash_rm -argstest 10
fi

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6

if [ "${1}" == 'debug0' ]; then
  gdb --args "./${test_exe}" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central \
    -flash="${simulation_id}_client.log.bin" -flash_rm -argstest 10
fi

if [ "${1}" == 'debug1' ]; then
  gdb --args "./${test_exe}" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral \
    -flash="${simulation_id}_server.log.bin" -flash_rm -argstest 10
fi

wait_for_background_jobs
