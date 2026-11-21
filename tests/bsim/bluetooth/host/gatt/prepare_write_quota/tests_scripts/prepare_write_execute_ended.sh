#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# A GATT server and a client whose Execute Write Requests cancel, fail and write
# the prepared values. The server's application must be told how each one ended,
# and not when the queue is empty.

simulation_id="${BOARD_TS}_$(guess_test_long_name)_execute_ended"
verbosity_level=2
sim_length_us=$((5 * 1000 * 1000))

test_exe="./bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 \
  -sim_length=${sim_length_us} $@

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=executer
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=execute_server

wait_for_background_jobs
