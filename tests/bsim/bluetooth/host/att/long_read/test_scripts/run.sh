#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="long_read"
dev_exe=bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf
args_all=(-s=${simulation_id} -D=2)
args_dev=(-v=2 -RealEncryption=1 -testid=the_test)

cd "${BSIM_OUT_PATH}/bin"

Execute ./${dev_exe} "${args_all[@]}" "${args_dev[@]}" -d=0

Execute ./${dev_exe} "${args_all[@]}" "${args_dev[@]}" -d=1

Execute ./bs_2G4_phy_v1 "${args_all[@]}" -v=6 -sim_length=60e6

wait_for_background_jobs
