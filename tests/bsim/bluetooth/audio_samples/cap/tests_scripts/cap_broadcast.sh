#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the CAP samples for broadcast.
# It relies on the bs_tests hooks to register a test timer callback, which after a deadline
# will check how many broadcast audio packets have been tranferred, and if over a threshold
# it considers the test passed

simulation_id="cap_broadcast_test"
verbosity_level=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_samples_cap_initiator_broadcast_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1 -testid=cap_initiator

# TODO: Add CAP acceptor

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=20e6 $@ -argschannel -at=40

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
