#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# The DUT keeps a reference to a connection object past the disconnection, which
# must not get in the way of the next connection with the same peer, whichever
# side starts it, nor of notifying the peer and unpairing it. The same holds for
# an object being set up when the peer connects.

test_name="$(guess_test_long_name)"
simulation_id="${BOARD_TS}_${test_name}_kept_conn"
verbosity_level=2
sim_length_us=$((5 * 1000 * 1000))

test_exe="./bs_${BOARD_TS}_${test_name}_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 \
  -sim_length=${sim_length_us} $@

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=kept_conn_dut
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=kept_conn_peer

wait_for_background_jobs
