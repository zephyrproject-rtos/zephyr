#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Disable test: bt_enable and bt_disable are called in a loop.
simulation_id="disable_test"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bsim_bluetooth_host_misc_disable_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=disable

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=60e6 $@

wait_for_background_jobs
