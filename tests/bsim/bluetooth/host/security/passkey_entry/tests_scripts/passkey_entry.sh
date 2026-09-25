#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
simulation_id="${BOARD_TS}_passkey_entry"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -RealEncryption=1

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=60e6

wait_for_background_jobs
