#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The central connects the channel with 0 initial credits in its request and
# the peripheral, the acceptor, sends.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="${BOARD_TS}_$(guess_test_long_name)_zero_credits_in_req"
bsim_exe=./bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=zero_credits_central_receiver
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=zero_credits_peripheral_sender

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs
