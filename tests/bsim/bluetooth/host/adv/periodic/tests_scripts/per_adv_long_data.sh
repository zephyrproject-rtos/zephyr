#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Basic periodic advertising sync test: an advertiser advertises with periodic
# advertising, and a scanner scans for and syncs to the periodic advertising.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="per_adv_long_data"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_long_data_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=per_adv_long_data_advertiser -rs=23

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_long_data_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=0 \
  -testid=per_adv_long_data_sync -rs=6

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
