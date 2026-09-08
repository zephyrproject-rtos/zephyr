#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_scan_ext_filter_policy_prj_conf"
simulation_id="${BOARD_TS}_ext_filter_policy"
verbosity_level=2

# The DUT scans for 2 simulated seconds with the basic filter policy before it
# scans with the extended one.
SIM_LEN_US=$((10 * 1000 * 1000))

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=0 -rs=420 -testid=dut

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=1 -rs=69 -testid=peer

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
  -D=2 -sim_length=${SIM_LEN_US} $@

wait_for_background_jobs
