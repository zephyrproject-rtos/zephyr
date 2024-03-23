#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Validate Extended Advertising AD Data fragment operation, PDU chaining and
# Extended Scanning of chain PDUs
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="adv_chain"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_chain_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=adv

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_chain_prj_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=scan

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
