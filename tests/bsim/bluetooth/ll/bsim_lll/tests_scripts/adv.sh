#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Legacy advertising of the BabbleSim LLL, received by the Nordic LLL
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

advertiser_exe="bs_${BOARD_TS}_$(guess_test_long_name)_advertiser_prj_conf"
scanner_exe="bs_${BOARD_TS}_$(guess_test_long_name)_scanner_prj_conf"

simulation_id="bsim_lll_adv"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${advertiser_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=advertiser

Execute "./${scanner_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=scanner

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs

# The scanner seeing no SCAN_RSP in the last step, STEP_FAL_REJECT, proves
# nothing unless it sent SCAN_REQs, so check the Tx dumps from the first
# ADV_SCAN_IND with the advertising data of that step on: SCAN_REQs from the
# scanner, and no SCAN_RSP from the advertiser.
results="${BSIM_OUT_PATH}/results/${simulation_id}"
reject_start=$(grep -m 1 ',[0-9A-F]6 [0-9A-F][0-9A-F] .* 05 FF FF FF 01 04 ' \
  "${results}/d_2G4_00.Tx.csv" | cut -d, -f1)
if [ -z "${reject_start}" ]; then
  echo "No advertising of the Filter Accept List reject step"
  exit 1
fi

count_pdus() {
  local dump=$1
  local type=$2
  local count=0

  while IFS=, read -r start end freq addr mod power abort recheck size packet; do
    if [ "${start}" -ge "${reject_start}" ] &&
       [ $((16#${packet:0:2} & 0x0f)) -eq "${type}" ]; then
      count=$((count + 1))
    fi
  done < <(tail -n +2 "${dump}")

  echo ${count}
}

scan_req=$(count_pdus "${results}/d_2G4_01.Tx.csv" 3)
if [ ${scan_req} -eq 0 ]; then
  echo "No SCAN_REQ sent in the Filter Accept List reject step"
  exit 1
fi

scan_rsp=$(count_pdus "${results}/d_2G4_00.Tx.csv" 4)
if [ ${scan_rsp} -ne 0 ]; then
  echo "${scan_rsp} SCAN_RSPs sent in the Filter Accept List reject step"
  exit 1
fi
