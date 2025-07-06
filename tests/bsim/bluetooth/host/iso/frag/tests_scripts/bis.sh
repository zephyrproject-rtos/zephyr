#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="iso_frag"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=1 -sim_length=30e6 $@

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_frag_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=broadcaster

wait_for_background_jobs
