#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

CLIENT_SAMPLE="samples/bluetooth/audio/bap_unicast_client"
CLIENT_TEST="tests/bsim/bluetooth/audio_samples/bap_unicast_client"
BOARD_CONF="boards/nrf5340_audio_dk_nrf5340_cpuapp.conf"

if [ "${BOARD_TS}" == "nrf5340bsim_nrf5340_cpuapp" ]; then
  app=samples/bluetooth/audio/bap_unicast_server sysbuild=1 compile
  app=${CLIENT_TEST} \
    sample=${ZEPHYR_BASE}/${CLIENT_SAMPLE} \
    conf_file=${sample}/prj.conf \
    conf_overlay="${sample}/${BOARD_CONF};${sample}/overlay-sequential.conf" \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_overlay-sequential_conf sysbuild=1 compile
  app=${CLIENT_TEST} \
    sample=${ZEPHYR_BASE}/${CLIENT_SAMPLE} \
    conf_file=${sample}/prj.conf \
    conf_overlay="${sample}/${BOARD_CONF};${sample}/overlay-interleaved.conf" \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_overlay-interleaved_conf sysbuild=1 compile
else
  app=samples/bluetooth/audio/bap_unicast_server conf_overlay=overlay-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf sysbuild=1 compile
  app=${CLIENT_TEST} \
    sample=${ZEPHYR_BASE}/${CLIENT_SAMPLE} \
    conf_file=${sample}/prj.conf \
    conf_overlay="${sample}/overlay-bt_ll_sw_split.conf;${sample}/overlay-sequential.conf" \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_overlay-sequential_conf sysbuild=1 compile
  app=${CLIENT_TEST} \
    sample=${ZEPHYR_BASE}/${CLIENT_SAMPLE} \
    conf_file=${sample}/prj.conf \
    conf_overlay="${sample}/overlay-bt_ll_sw_split.conf;${sample}/overlay-interleaved.conf" \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_overlay-interleaved_conf sysbuild=1 compile
fi

wait_for_background_jobs
