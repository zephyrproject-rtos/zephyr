#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# Legacy advertising of the Nordic LLL, received by the BabbleSim LLL
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

advertiser_exe="bs_${BOARD_TS}_$(guess_test_long_name)_advertiser_prj_conf_nordic_lll"
scanner_exe="bs_${BOARD_TS}_$(guess_test_long_name)_scanner_prj_conf_bsim_lll"

simulation_id="bsim_lll_scan"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${advertiser_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=advertiser

Execute "./${scanner_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=scanner

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
