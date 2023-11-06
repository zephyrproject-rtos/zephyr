#!/usr/bin/env bash
#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id=$(basename $BASH_SOURCE)
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n Test HAS client bond deleted (ACL is up)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_bond_deleted_acl -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs

printf "\n\n Test HAS client bond deleted (ACL is down)\n\n"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=has -rs=24 -D=2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=has_client_bond_deleted_no_acl -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
