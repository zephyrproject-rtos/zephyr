#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Disable test: bt_enable and bt_disable are called in a loop.
# Each iteration the default ID is updated
simulation_id="disable_test_set_default_id"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_disable_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=disable_set_default_id

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=10e6 $@

wait_for_background_jobs
