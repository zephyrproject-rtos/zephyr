#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

SOURCE_SAMPLE="samples/bluetooth/audio/bap_broadcast_source"
SINK_SAMPLE="samples/bluetooth/audio/bap_broadcast_sink"

if [ "${BOARD_TS}" == "nrf5340bsim_nrf5340_cpuapp" ]; then
  app=${SOURCE_SAMPLE} extra_conf_file=extra-sequential.conf \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-sequential_conf sysbuild=1 compile
  app=${SOURCE_SAMPLE} extra_conf_file=extra-interleaved.conf \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-interleaved_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/bap_broadcast_sink sysbuild=1 \
    conf_file=${ZEPHYR_BASE}/${SINK_SAMPLE}/prj.conf \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf sysbuild=1 compile
else
  app=${SOURCE_SAMPLE} \
    extra_conf_file='extra-bt_ll_sw_split.conf;extra-sequential.conf' \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-sequential_conf sysbuild=1 compile
  app=${SOURCE_SAMPLE} \
    extra_conf_file='extra-bt_ll_sw_split.conf;extra-interleaved.conf' \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-interleaved_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/bap_broadcast_sink \
    conf_file=${ZEPHYR_BASE}/${SINK_SAMPLE}/prj.conf \
    extra_conf_file=${ZEPHYR_BASE}/${SINK_SAMPLE}/extra-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf sysbuild=1 compile
fi

wait_for_background_jobs
