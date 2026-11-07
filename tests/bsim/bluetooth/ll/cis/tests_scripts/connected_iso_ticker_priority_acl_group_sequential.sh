#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic Connected ISO test: a Central connects to 4 Peripherals and Establishes
# 1 CIS each to 4 Peripherals (4 CIS in a CIG), using ticker priority based
# collision resolution
simulation_id="${BOARD_TS}_connected_iso_ticker_priority_acl_group_sequential"
verbosity_level=2
EXECUTE_TIMEOUT=200

exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_cis_prj_conf"
exe="${exe}_overlay-ticker_priority_acl_group_sequential_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=peripheral

Execute ${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=3 -testid=peripheral

Execute ${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=4 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=5 -sim_length=60e6 $@

wait_for_background_jobs
