#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The Peripheral has no Transient Application Key: it refuses the encryption the Central starts.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_general_tak_no_key"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=tak_central_no_key -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_general_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=tak_peripheral_no_key -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
