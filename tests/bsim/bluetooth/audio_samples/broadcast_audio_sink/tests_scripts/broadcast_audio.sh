#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the broadcast audio sink/source samples,
# It relies on the bs_tests hooks to register a test timer callback, which after a deadline
# will check how many audio packets the broadcast audio sink has received, and if over a threshold
# it considers the test passed

simulation_id="broadcast_audio_samples_test"
verbosity_level=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=200

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_samples_bluetooth_broadcast_audio_source_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_samples_broadcast_audio_sink_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=broadcast_audio_sink

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=120e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
