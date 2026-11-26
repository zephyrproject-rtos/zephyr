#!/usr/bin/env bash
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# As per_adv_long_data.sh, but with CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS=y so the
# scanner syncs to a chained periodic train using the chained scan-aux design.
# Without the lll_sync cleanup fix this asserts [lll_aux] in ull_scan_aux_setup().

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_per_adv_long_data_chains"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_long_data_chains_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=per_adv_long_data_advertiser

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_long_data_chains_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=per_adv_long_data_sync

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=15e6 $@

wait_for_background_jobs
