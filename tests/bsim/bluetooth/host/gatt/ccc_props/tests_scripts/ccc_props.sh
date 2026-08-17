#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Verify that CCC writes are validated against the properties of the owning
# characteristic: unsupported configurations are rejected with CCC Improperly
# Configured, supported ones are accepted and delivered, and a CCC without a
# characteristic declaration is left unvalidated.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_gatt_ccc_props"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_ccc_props_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=gatt_client

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_gatt_ccc_props_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=gatt_server

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
