#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# A scanner with a peer in its resolving list in Network Privacy mode enables an advertising set,
# and still does not report the identity address the peer advertises with
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_scan_rl_adv"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=scan_rl_adv

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=nconn_adv_id

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
