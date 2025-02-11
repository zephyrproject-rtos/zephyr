#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="iso_bis"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_bis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=broadcaster

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_bis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=receiver

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=30e6 $@

wait_for_background_jobs
