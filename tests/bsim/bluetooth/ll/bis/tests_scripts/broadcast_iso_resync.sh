#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# ISO broadcast re-sync test: the receiver synchronizes to and terminates the
# BIG repeatedly, the Controller must get back all ISO Rx buffers each time.
simulation_id="${BOARD_TS}_broadcast_iso_resync"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_conf_overlay-sequential_conf \
  -v=${verbosity_level} -s=${simulation_id} -RealEncryption=1 -d=0 -testid=receive_resync

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_conf_overlay-sequential_conf \
  -v=${verbosity_level} -s=${simulation_id} -RealEncryption=1 -d=1 -testid=broadcast_resync

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
