#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_scan_adv_shared_addr_prj_conf"
simulation_id="${BOARD_TS}_adv_shared_addr_public_dir"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=public_dir

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=peer

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
  -D=2 -sim_length=5e6

wait_for_background_jobs
