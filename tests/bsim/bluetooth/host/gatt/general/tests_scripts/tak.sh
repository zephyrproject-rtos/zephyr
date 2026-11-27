#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# Both devices supply the same Transient Application Key: the link is encrypted without pairing.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_general_tak"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=tak_central -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=tak_peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
