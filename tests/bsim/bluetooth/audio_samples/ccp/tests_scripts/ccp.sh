#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the CCP samples.

simulation_id="ccp_samples_test"
verbosity_level=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_samples_ccp_call_control_server_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1 -testid=ccp_call_control_server

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=20e6 $@ -argschannel -at=40

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
