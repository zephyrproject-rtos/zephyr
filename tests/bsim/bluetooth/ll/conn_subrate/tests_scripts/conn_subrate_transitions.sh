#!/usr/bin/env bash
# Copyright 2026 The ZMK Contributors
# SPDX-License-Identifier: Apache-2.0

# LE Connection Subrating: M->N subrate transition (factor 4 -> 8)
# followed by a connection-parameter interval change while subrated, which must
# reset subrating to factor 1. The central drives the transitions and verifies
# the link stays in sync and re-skips / stops skipping accordingly.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="conn_subrate_transitions"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_subrate_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=peripheral_transitions -rs=23

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_subrate_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=0 \
  -testid=central_transitions -rs=6

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=40e6 $@

wait_for_background_jobs
