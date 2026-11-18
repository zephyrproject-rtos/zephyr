#!/usr/bin/env bash
# Copyright 2026 The ZMK Contributors
# SPDX-License-Identifier: Apache-2.0

# Additional LE Connection Subrating scenarios:
#   - continuation events (continuation_number > 0)
#   - peripheral latency stacking (max_latency > 0)
#   - simultaneous bidirectional re-negotiation (LLCP collision)

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
bin=./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_subrate_prj_conf

cd ${BSIM_OUT_PATH}/bin

run_scenario() {
  local sim_id=$1
  local periph_testid=$2
  local central_testid=$3

  Execute ${bin} -v=${verbosity_level} -s=${sim_id} -d=0 -RealEncryption=0 \
    -testid=${periph_testid} -rs=23
  Execute ${bin} -v=${verbosity_level} -s=${sim_id} -d=1 -RealEncryption=0 \
    -testid=${central_testid} -rs=6
  Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${sim_id} -D=2 -sim_length=40e6

  wait_for_background_jobs
}

# Three-device scenario: one Central holding two links (a subrating peer and a
# plain full-rate peer) -- the real split topology (subrated split + host link).
run_multi() {
  local sim_id=$1
  local periph_subrate=$2
  local periph_plain=$3
  local central_testid=$4

  Execute ${bin} -v=${verbosity_level} -s=${sim_id} -d=0 -RealEncryption=0 \
    -testid=${periph_subrate} -rs=23
  Execute ${bin} -v=${verbosity_level} -s=${sim_id} -d=1 -RealEncryption=0 \
    -testid=${periph_plain} -rs=44
  Execute ${bin} -v=${verbosity_level} -s=${sim_id} -d=2 -RealEncryption=0 \
    -testid=${central_testid} -rs=6
  Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${sim_id} -D=3 -sim_length=40e6

  wait_for_background_jobs
}

run_scenario conn_subrate_continuation peripheral_continuation central_continuation
run_scenario conn_subrate_latency       peripheral_latency       central_latency
run_scenario conn_subrate_latcount      peripheral_latency       central_lat_count
run_scenario conn_subrate_cadence8      peripheral               central_cadence
run_scenario conn_subrate_cadence5      peripheral_oddfactor     central_cadence
run_scenario conn_subrate_collision     peripheral_collision     central_collision
run_scenario conn_subrate_phy           peripheral               central_phy
run_scenario conn_subrate_disable       peripheral               central_disable
run_scenario conn_subrate_notify        peripheral_notify        central_notify
run_scenario conn_subrate_oddfactor     peripheral_oddfactor     central
run_scenario conn_subrate_supervision   peripheral_supervision   central_supervision
run_scenario conn_subrate_cwrite        peripheral_cwrite        central_cwrite
run_multi    conn_subrate_multi peripheral peripheral_plain central_multi
