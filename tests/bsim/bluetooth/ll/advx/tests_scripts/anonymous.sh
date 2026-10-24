#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The scanner reports anonymous extended advertising undirected and directed
# to it, without and with AD data in an AUX_ADV_IND, and sends AUX_SCAN_REQs
# only to scannable advertising with an AdvA. It reports advertising directed
# to a resolvable private address in an ADV_EXT_IND without AdvA when the
# address resolves with its local IRK.
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="${BOARD_TS}_advx_anonymous"
verbosity_level=2

# The advertising with an AUX_ADV_IND, which the playback device sends,
# written by test_data/gen_anonymous.py into the results of the simulation
results="${BSIM_OUT_PATH}/results/${simulation_id}"
mkdir -p "${results}"
python3 "${ZEPHYR_BASE}/$(guess_test_relpath)/test_data/gen_anonymous.py" "${results}" ||
  exit 1
playback="${results}/anonymous"

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=anon_adv

# The scanner resolves a TargetA with its local IRK, which takes the AES of
# the simulated hardware instead of the identity function of the simulation
Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=anon_scan \
  -RealEncryption=1

Execute ./bs_device_2G4_playback \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -inputf=${playback} -rxoff -rssioff

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=20e6 $@

wait_for_background_jobs

# The scanner scans actively. Its AUX_SCAN_REQs, the PDUs of the SCAN_REQ
# type, have to go to the AdvA of the valid scannable advertising,
# C0:00:00:00:00:04, which follows the PDU header and the ScanA.
aux_scan_req=0
while IFS=, read -r start end freq addr mod power abort recheck size packet; do
  hdr=$((16#${packet:0:2}))
  if [ $((hdr & 0x0f)) -ne 3 ]; then
    continue
  fi
  if [ "${packet:24:17}" != "04 00 00 00 00 C0" ]; then
    echo "AUX_SCAN_REQ sent to ${packet:24:17}"
    exit 1
  fi
  aux_scan_req=$((aux_scan_req + 1))
done < <(tail -n +2 "${results}/d_2G4_01.Tx.csv")

if [ ${aux_scan_req} -eq 0 ]; then
  echo "No AUX_SCAN_REQ sent to the scannable advertising"
  exit 1
fi
