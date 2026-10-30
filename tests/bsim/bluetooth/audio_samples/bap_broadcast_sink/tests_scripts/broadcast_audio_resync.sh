#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_samples_bt_bap_bc_src_resync"
verbosity_level=2
EXECUTE_TIMEOUT=200

cd ${BSIM_OUT_PATH}/bin

Execute \
  ./bs_${BOARD_TS}_samples_bluetooth_audio_bap_broadcast_source_prj_conf_overlay-sequential_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_samples_bap_broadcast_sink_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=bap_broadcast_sink_resync

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=150e6 $@

wait_for_background_jobs
