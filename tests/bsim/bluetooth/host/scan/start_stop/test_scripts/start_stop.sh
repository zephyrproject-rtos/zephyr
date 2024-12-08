#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_scan_start_stop_prj_conf"
simulation_id="start_stop"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=scanner

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=periodic_adv

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
  -D=2 -sim_length=5e6

wait_for_background_jobs
