#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Directed scannable extended advertising answers the scan requests of its
# TargetA only
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="ll_ext_adv_scan_req_tgta"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=tgta_adv

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=tgta_scan

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs

# The advertiser not answering proves nothing unless the scanner asked, so
# its Tx dump has to have AUX_SCAN_REQs, the PDUs of the SCAN_REQ type on a
# secondary advertising channel, from the first PDU directed to the other
# RPA (40:00:00:00:00:03) on
results="${BSIM_OUT_PATH}/results/${simulation_id}"
other_start=$(grep -m 1 ' 03 00 00 00 00 40' "${results}/d_2G4_00.Tx.csv" |
  cut -d, -f1)
if [ -z "${other_start}" ]; then
  echo "No advertising directed to the other RPA"
  exit 1
fi

aux_scan_req=0
while IFS=, read -r start end freq addr mod power abort recheck size packet; do
  case "${freq}" in
    2.000000|26.000000|80.000000) continue ;;
  esac
  hdr=$((16#${packet:0:2}))
  if [ "${start}" -ge "${other_start}" ] && [ "${addr}" = "0x8E89BED6" ] &&
     [ $((hdr & 0x0f)) -eq 3 ]; then
    aux_scan_req=$((aux_scan_req + 1))
  fi
done < <(tail -n +2 "${results}/d_2G4_01.Tx.csv")

if [ ${aux_scan_req} -eq 0 ]; then
  echo "No AUX_SCAN_REQ sent to the advertising directed to the other RPA"
  exit 1
fi
