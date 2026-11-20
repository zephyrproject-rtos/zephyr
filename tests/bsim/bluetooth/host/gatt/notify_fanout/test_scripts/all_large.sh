#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
set -eu

source "${ZEPHYR_BASE}/tests/bsim/sh_common.source"

simulation_id="${BOARD_TS}_gatt_fanout_${FANOUT_CONFIG:-prj_conf}_all_large"
verbosity_level=2

cd "${BSIM_OUT_PATH}/bin"
exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_notify_fanout_${FANOUT_CONFIG:-prj_conf}"

Execute "$exe" -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1 -testid=large_first
Execute "$exe" -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 -testid=large_second
Execute "$exe" -v=${verbosity_level} -s=${simulation_id} -d=2 -RealEncryption=1 -testid=server

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=3 -sim_length=20e6 "$@"

wait_for_background_jobs
