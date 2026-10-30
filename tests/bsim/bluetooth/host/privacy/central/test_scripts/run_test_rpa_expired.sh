#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="${BOARD_TS}_test_rpa_expired"
dut_exe=bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute ./${dut_exe} -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=rpa_expired

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=1 -sim_length=20e6 $@

wait_for_background_jobs
