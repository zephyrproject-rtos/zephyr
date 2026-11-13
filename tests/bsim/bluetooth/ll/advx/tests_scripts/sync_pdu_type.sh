#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Periodic sync ignores PDUs of types other than AUX_SYNC_IND and
# AUX_CHAIN_IND received with the Access Address and CRCInit of the train,
# while establishing the synchronization, while synchronized and in place of
# an AUX_CHAIN_IND
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_sync_pdu_type"
verbosity_level=2

# The periodic advertising, which the playback device sends, written by
# test_data/gen_sync_pdu_type.py into the results of the simulation
results="${BSIM_OUT_PATH}/results/${simulation_id}"
mkdir -p "${results}"
python3 "${ZEPHYR_BASE}/$(guess_test_relpath)/test_data/gen_sync_pdu_type.py" "${results}" ||
  exit 1
playback="${results}/sync_pdu_type"

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=sync_pdu_type

Execute ./bs_device_2G4_playback \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -inputf=${playback} -rxoff -rssioff

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs
