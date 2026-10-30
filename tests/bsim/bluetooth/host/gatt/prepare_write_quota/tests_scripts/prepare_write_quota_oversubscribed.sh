#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Same as prepare_write_quota.sh, but with CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN
# set above the equal share of the pool: the holder gets to keep more
# buffers, and the writer's long write is rejected with Prepare Queue Full.

test_name="$(guess_test_long_name)"
simulation_id="${BOARD_TS}_${test_name}_oversubscribed"
verbosity_level=2
sim_length_us=$((5 * 1000 * 1000))

test_exe="./bs_${BOARD_TS}_${test_name}_overlay-oversubscribed_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=3 \
  -sim_length=${sim_length_us} $@

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=holder
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=writer
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=server

wait_for_background_jobs
