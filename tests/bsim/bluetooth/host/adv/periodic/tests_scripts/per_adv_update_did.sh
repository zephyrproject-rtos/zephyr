#!/usr/bin/env bash
# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# Periodic advertising DID update test: verifies that
# bt_le_per_adv_update_did() succeeds when periodic advertising is active
# with ADI enabled and data set.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_per_adv_update_did"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=0 \
  -testid=per_adv_update_did -rs=23

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=10e6 $@

wait_for_background_jobs
