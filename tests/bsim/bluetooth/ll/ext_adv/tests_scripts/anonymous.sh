#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# The scanner reports anonymous extended advertising undirected and directed
# to it, without and with AD data in an AUX_ADV_IND
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source
source $(dirname "$0")/_anonymous_aux.sh

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="ll_ext_adv_anonymous"
verbosity_level=2

playback="${BSIM_OUT_PATH}/results/${simulation_id}/anonymous_aux"
anonymous_aux_tx "${playback}" "02 00 00 00 00 C0"

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
