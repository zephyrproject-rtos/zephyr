#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

if [ "${BOARD_TS}" == "nrf5340bsim_nrf5340_cpuapp" ]; then
  app=tests/bsim/bluetooth/audio_samples/cap/initiator \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_initiator \
    cmake_extra_args="-DCONFIG_SAMPLE_UNICAST=n" \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/boards/nrf5340_audio_dk_nrf5340_cpuapp.conf \
    exe_name=bs_${BOARD_TS}_${app}_broadcast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/acceptor \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_acceptor \
    cmake_extra_args="-DCONFIG_SAMPLE_SCAN_SELF=y -DCONFIG_SAMPLE_UNICAST=n" \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/boards/nrf5340_audio_dk_nrf5340_cpuapp.conf \
    exe_name=bs_${BOARD_TS}_${app}_broadcast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/initiator \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_initiator \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/boards/nrf5340_audio_dk_nrf5340_cpuapp.conf \
    exe_name=bs_${BOARD_TS}_${app}_unicast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/acceptor \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_acceptor \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/boards/nrf5340_audio_dk_nrf5340_cpuapp.conf \
    exe_name=bs_${BOARD_TS}_${app}_unicast_prj_conf sysbuild=1 compile
else
  app=tests/bsim/bluetooth/audio_samples/cap/initiator \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_initiator \
    cmake_extra_args="-DCONFIG_SAMPLE_UNICAST=n" \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/overlay-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_broadcast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/acceptor \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_acceptor \
    cmake_extra_args="-DCONFIG_SAMPLE_SCAN_SELF=y -DCONFIG_SAMPLE_UNICAST=n" \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/overlay-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_broadcast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/initiator \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_initiator \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/overlay-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_unicast_prj_conf sysbuild=1 compile
  app=tests/bsim/bluetooth/audio_samples/cap/acceptor \
    sample=${ZEPHYR_BASE}/samples/bluetooth/cap_acceptor \
    conf_file=${sample}/prj.conf \
    conf_overlay=${sample}/overlay-bt_ll_sw_split.conf \
    exe_name=bs_${BOARD_TS}_${app}_unicast_prj_conf sysbuild=1 compile
fi

wait_for_background_jobs
