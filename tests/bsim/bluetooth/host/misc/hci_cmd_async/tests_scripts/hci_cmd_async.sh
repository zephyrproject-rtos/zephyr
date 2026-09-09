#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Exercises the host-internal asynchronous HCI command API on a single device.
simulation_id="${BOARD_TS}_hci_cmd_async"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_hci_cmd_async_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=hci_cmd_async

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=20e6 $@

wait_for_background_jobs
