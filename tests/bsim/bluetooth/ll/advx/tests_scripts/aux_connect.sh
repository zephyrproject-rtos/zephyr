#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# A peripheral stops its connectable extended advertising at points across the
# AUX_CONNECT_REQ and AUX_CONNECT_RSP of a connection attempt, and has to accept
# a connection after that.
simulation_id="${BOARD_TS}_aux_connect"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_advx_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=peripheral_aux_connect

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_advx_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=central_aux_connect

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=5e6 $@

wait_for_background_jobs
