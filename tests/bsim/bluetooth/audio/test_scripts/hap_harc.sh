#!/usr/bin/env bash
#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id=$(basename $BASH_SOURCE)
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n Test HAP HARC\n\n"
Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=hap_harc_test_binaural \
  -rs=1 -D=3

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_binaural \
  -rs=2 -D=3 -argstest rank 1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=has_binaural \
  -rs=3 -D=3 -argstest rank 2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs
