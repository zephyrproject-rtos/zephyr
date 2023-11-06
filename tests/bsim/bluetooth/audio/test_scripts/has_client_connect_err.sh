#!/usr/bin/env bash
#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id=$(basename $BASH_SOURCE)
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n Test HAS client connection fails to incomplete server (no Hearing Aid Features)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has_no_features_chrc -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_connect_err -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs

printf "\n\n Test HAS client connection fails to server error (no Active Preset Index)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has_no_active_index_chrc -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_connect_err -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs

printf "\n\n Test HAS client connection fails to server error (no Active Preset Index CCC)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has_no_active_index_ccc -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_connect_err -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs

printf "\n\n Test HAS client connection fails to server error (subscription error)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has_active_index_subscribe_err -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_connect_err -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs

printf "\n\n Test HAS client connection fails to server error (connection interrupt)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has_unexpected_disconnection -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_connect_err -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
