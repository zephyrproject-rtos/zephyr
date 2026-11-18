#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The scanner does not report anonymous extended advertising directed to another
# device, without and with AD data in an AUX_ADV_IND
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_anonymous_other"
verbosity_level=2

# The anonymous advertising with an AUX_ADV_IND, which the playback device
# sends, written by test_data/gen_anonymous.py into the results of the
# simulation
results="${BSIM_OUT_PATH}/results/${simulation_id}"
mkdir -p "${results}"
python3 "${ZEPHYR_BASE}/$(guess_test_relpath)/test_data/gen_anonymous.py" "${results}" ||
  exit 1
playback="${results}/anonymous_other"

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=anon_adv_other

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=anon_scan_other

Execute ./bs_device_2G4_playback \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -inputf=${playback} -rxoff -rssioff

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=20e6 $@

wait_for_background_jobs
