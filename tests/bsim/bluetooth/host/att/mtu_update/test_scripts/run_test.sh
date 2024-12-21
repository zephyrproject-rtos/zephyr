#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="mtu_update"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

central_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_central_conf"
peripheral_exe="./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_mtu_update_prj_peripheral_conf"

Execute "$central_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -RealEncryption=1

Execute "$peripheral_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=60e6 $@ -argschannel -at=40

wait_for_background_jobs
