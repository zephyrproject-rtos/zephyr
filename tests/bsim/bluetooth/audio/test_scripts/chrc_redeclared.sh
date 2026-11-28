#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# A server declares the GTBS Call State and the CSIS SIRK characteristics twice, and a client
# discovers them with the TBS client and the CSIP set coordinator
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_chrc_redeclared"
VERBOSITY_LEVEL=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running repeated characteristic declaration test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=chrc_redeclared_server -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=chrc_redeclared_client -D=2

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
