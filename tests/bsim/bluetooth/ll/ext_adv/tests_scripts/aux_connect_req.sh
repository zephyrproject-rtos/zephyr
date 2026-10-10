#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# ChSel is RFU in AUX_CONNECT_REQ, so the initiator sends it as 0
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

simulation_id="ll_ext_adv_aux_connect_req"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=conn_adv

Execute "./${exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=conn_init

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=20e6 $@

wait_for_background_jobs

# The AUX_CONNECT_REQs in the Tx dump of the initiator are the PDUs of the
# CONNECT_IND type sent with the advertising access address on a secondary
# advertising channel
aux_connect_req=0
while IFS=, read -r start end freq addr mod power abort recheck size packet; do
  case "${freq}" in
    2.000000|26.000000|80.000000) continue ;;
  esac
  hdr=$((16#${packet:0:2}))
  if [ "${addr}" != "0x8E89BED6" ] || [ $((hdr & 0x0f)) -ne 5 ]; then
    continue
  fi
  aux_connect_req=$((aux_connect_req + 1))
  if [ $((hdr & 0x20)) -ne 0 ]; then
    echo "AUX_CONNECT_REQ sent with ChSel set: ${packet}"
    exit 1
  fi
done < <(tail -n +2 "${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4_01.Tx.csv")

if [ ${aux_connect_req} -eq 0 ]; then
  echo "No AUX_CONNECT_REQ sent"
  exit 1
fi
