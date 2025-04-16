#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic connection test: a central connects to a peripheral and expects a
# notification, using the split controller (ULL LLL)
simulation_id="conn_param"
verbosity_level=2
EXECUTE_TIMEOUT=900

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_param_prj-ll_sw_llcp_legacy_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1 -testid=central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_param_prj-ll_sw_llcp_legacy_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=1200e6 $@ -argschannel -at=40

wait_for_background_jobs
