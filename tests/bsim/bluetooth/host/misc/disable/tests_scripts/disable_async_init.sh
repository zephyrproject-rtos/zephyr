#!/usr/bin/env bash
# Copyright (c) 2026 NXP
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Disable test: bt_disable() called immediately after bt_enable(cb) must
# return -EAGAIN while async init is still in progress.
simulation_id="${BOARD_TS}_disable_async_init"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_disable_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=disable_async_init

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=10e6 $@

wait_for_background_jobs
