#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The peripheral accepts the channel with 0 initial credits in its response
# and the central, the initiator, sends.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="${BOARD_TS}_$(guess_test_long_name)_zero_credits_in_rsp"
bsim_exe=./bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=zero_credits_central_sender
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=zero_credits_peripheral_receiver

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs
