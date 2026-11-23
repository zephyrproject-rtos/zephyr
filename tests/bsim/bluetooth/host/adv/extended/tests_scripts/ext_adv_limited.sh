#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# Extended advertising test:
#
# - Limited: the advertiser starts advertising sets with an event limit or a
#   timeout. Its sent callback has to run when the limit is reached and must
#   not run when the scanner/central connects before that.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_ext_adv_limited"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_advertiser_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=ext_adv_limited_advertiser

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_extended_prj_scanner_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=ext_adv_conn_scanner

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
