#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the Zephyr root directory}"
source "${ZEPHYR_BASE}/tests/bsim/sh_common.source"

sim_seconds=60
verbosity_level=2
simulation_id="test_bt_conn_ltk_request"

rel="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

long_name="$(guess_test_long_name)"
exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${long_name}_prj_conf"

args_all=(-s="${simulation_id}")
args_dev=(-v=2 -RealEncryption=1 -testid=test_bt_conn_ltk_request)

echo "Simulation time: $sim_seconds seconds"

# bs_2G4_phy_v1 requires cwd to be at its location
cd "${BSIM_OUT_PATH}/bin"

EXECUTE_TIMEOUT=60
Execute ./bs_2G4_phy_v1 "${args_all[@]}" -v=6 -sim_length=$((sim_seconds * 10 ** 6)) -D=2
Execute "${exe}" "${args_all[@]}" "${args_dev[@]}" -d=0
Execute "${exe}" "${args_all[@]}" "${args_dev[@]}" -d=1

wait_for_background_jobs
