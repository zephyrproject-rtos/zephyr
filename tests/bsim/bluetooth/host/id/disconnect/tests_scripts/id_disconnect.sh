#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# A peripheral advertises with a secondary identity and a central connects to it
# without pairing. The peripheral then deletes the identity and, in a second
# round, resets it. Both operations must disconnect the connection, and must
# leave a connection of the default identity alone.
simulation_id="${BOARD_TS}_id_disconnect"
verbosity_level=2

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_id_disconnect_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=peripheral
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=central

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=20e6 $@

wait_for_background_jobs
