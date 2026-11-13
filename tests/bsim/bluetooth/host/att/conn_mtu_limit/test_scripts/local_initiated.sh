#!/usr/bin/env bash
# Copyright (c) 2026 Embeint Holdings Pty Ltd
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"
simulation_id="${BOARD_TS}_${test_name}_local"
verbosity_level=2
sim_length_us=$((5 * 1000 * 1000))
test_exe="./bs_${BOARD_TS}_${test_name}_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 \
  -sim_length=${sim_length_us} $@

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_local
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_local

wait_for_background_jobs
