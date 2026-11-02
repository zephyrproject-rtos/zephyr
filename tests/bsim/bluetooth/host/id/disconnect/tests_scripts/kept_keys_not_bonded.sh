#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# The DUT keeps a connection object whose peer paired without bonding. Another
# peer then bonds, gets the same key slot, and must keep its bond when the DUT
# unpairs the first peer.
simulation_id="${BOARD_TS}_id_kept_keys_not_bonded"
verbosity_level=2

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_id_disconnect_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=kept_keys_not_bonded_dut
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=kept_keys_not_bonded_peer

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs
