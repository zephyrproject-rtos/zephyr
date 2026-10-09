#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The peripheral accepts the channel with 0 initial credits in its response
# and the central, the initiator, sends.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_l2cap_zero_credits_in_rsp"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_zero_initial_credits_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_sender

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_zero_initial_credits_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_receiver

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
