#!/usr/bin/env bash
# Copyright 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic ISO broadcast test: a broadcaster transmits a BIS and a receiver listens
# to the BIS.
simulation_id="broadcast_iso_ll_interface"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_conf_overlay-ll_interface_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_conf_overlay-ll_interface_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
