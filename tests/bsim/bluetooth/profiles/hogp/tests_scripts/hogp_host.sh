#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# HOGP Host test: a HOGP Report Host connects to a HID Device built on the HID
# Service, discovers it, receives an Input Report, reads and writes Reports,
# and does it again after reconnecting to the bonded Device.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_hogp_host"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_profiles_hogp_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=hogp_device -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_profiles_hogp_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=hogp_host -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
