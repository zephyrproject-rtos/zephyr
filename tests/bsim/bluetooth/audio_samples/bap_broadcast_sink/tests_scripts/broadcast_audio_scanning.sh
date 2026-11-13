#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Broadcast audio source and sink samples, both continuously scanning. The source updates the
# Broadcast ISO channel map from the channel metrics of its scanning. The sink checks that ISO
# packets are received and none is lost or has errors.

simulation_id="${BOARD_TS}_samples_bt_bap_bc_src_scan"
verbosity_level=2
EXECUTE_TIMEOUT=200

cd ${BSIM_OUT_PATH}/bin

Execute \
  ./bs_${BOARD_TS}_samples_bluetooth_audio_bap_broadcast_source_prj_conf_overlay-scan_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_samples_bap_broadcast_sink_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \
  -testid=bap_broadcast_sink_no_loss

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=120e6 $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
