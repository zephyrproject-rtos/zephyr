#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_path=$(guess_test_long_name)
dev_exe="bs_${BOARD_TS}_${test_path}_prj_conf"
simulation_id="${test_path}"

EXECUTE_TIMEOUT=120

cd "${BSIM_OUT_PATH}/bin"

args_all=(-s="${simulation_id}" -D=2)
args_dev=(-v=2 -RealEncryption=1 -testid=the_test)

Execute ./"${dev_exe}" "${args_all[@]}" "${args_dev[@]}" -d=1
Execute ./bs_2G4_phy_v1 "${args_all[@]}" -v=6 -sim_length=200e6
Execute ./"${dev_exe}" "${args_all[@]}" "${args_dev[@]}" -d=0

wait_for_background_jobs
