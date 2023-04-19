#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic connection test: a central connects to a peripheral and expects a
# notification, using the split controller (ULL LLL)
simulation_id="basic_advx_ticker_expire_info"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_advx_prj_conf_overlay-ticker_expire_info_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=advx

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_advx_prj_conf_overlay-ticker_expire_info_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=scanx

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
