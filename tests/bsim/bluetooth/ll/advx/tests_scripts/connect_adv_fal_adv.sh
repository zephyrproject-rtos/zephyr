#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# An advertiser using the Filter Accept List enables a second advertising set, and an
# initiator on the list still connects to it
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_connect_adv_fal_adv"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=conn_adv_fal_adv

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=conn_init_id_delayed

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
