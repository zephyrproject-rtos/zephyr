#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"

simulation_id=${test_name}

SIM_LEN_US=$((40 * 1000 * 1000))

test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -dump_imm -s=${simulation_id} -D=3 -sim_length=${SIM_LEN_US} $@

Execute "${test_exe}" -s=${simulation_id} -d=0 -rs=420 -RealEncryption=1 -testid=dut

# Start centrals with an offset, so the CONN_IND packets don't clash on-air.
# Since code executes in zero-time, this will always happen if we don't stagger
# the connection creation somehow.
Execute "${test_exe}" -s=${simulation_id} \
    -d=1 -rs=169 -RealEncryption=1 -testid=central -delay_init -start_offset=1e3
Execute "${test_exe}" -s=${simulation_id} \
    -d=2 -rs=690 -RealEncryption=1 -testid=central -delay_init -start_offset=10e3

wait_for_background_jobs
