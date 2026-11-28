#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The devices supply different Transient Application Keys: the link ends on a MIC failure,
# in a build without the Security Manager.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_ccc_props_tak_key_mismatch"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_ccc_props_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=tak_central_bad_key -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_ccc_props_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=tak_peripheral_bad_key -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
