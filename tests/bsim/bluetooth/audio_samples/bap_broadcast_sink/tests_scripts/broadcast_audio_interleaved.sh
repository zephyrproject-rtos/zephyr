#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the broadcast audio sink/source samples,
# It relies on the bs_tests hooks to register a test timer callback, which after a deadline
# will check how many audio packets the broadcast audio sink has received, and if over a threshold
# it considers the test passed

simulation_id="samples_bluetooth_bap_broadcast_source_interleaved_test"
sample_long_name="samples_bluetooth_bap_broadcast_source"
verbosity_level=2
EXECUTE_TIMEOUT=200

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_${sample_long_name}_prj_conf_overlay-interleaved_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_samples_bap_broadcast_sink_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=bap_broadcast_sink

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=120e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
