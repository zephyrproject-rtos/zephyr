#!/usr/bin/env bash
# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

# Extended advertising test:
#
# - Connectable: In addition to broadcasting advertisements, it is connectable
#   and restarts advertisements once disconnected. The scanner/central scans
#   for the packets and establishes the connection, to then disconnect
#   shortly-after.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="ext_adv_conn"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_advertiser_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=ext_adv_conn_advertiser -rs=23

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_scanner_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=0 \
  -testid=ext_adv_conn_scanner -rs=6

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
