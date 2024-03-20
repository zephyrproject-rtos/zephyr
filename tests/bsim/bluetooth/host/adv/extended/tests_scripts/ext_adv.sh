#!/usr/bin/env bash
# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

# Extended advertising test:
#
# - Broadcasting Only: a BLE broadcaster advertises with extended
#   advertising, and a scanner scans the extended advertisement packets.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="ext_adv"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_advertiser_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=ext_adv_advertiser -rs=23

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_scanner_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=0 \
  -testid=ext_adv_scanner -rs=6

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
