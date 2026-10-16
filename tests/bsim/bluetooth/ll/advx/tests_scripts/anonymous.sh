#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# The scanner reports anonymous extended advertising undirected and directed
# to it, without and with AD data in an AUX_ADV_IND
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_anonymous"
verbosity_level=2

# The anonymous advertising with an AUX_ADV_IND, which the playback device sends
playback="${ZEPHYR_BASE}/$(guess_test_relpath)/test_data/anonymous"

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=anon_adv

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=anon_scan

Execute ./bs_device_2G4_playback \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -inputf=${playback} -rxoff -rssioff

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=20e6 $@

wait_for_background_jobs
