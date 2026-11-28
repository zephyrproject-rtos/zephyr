#!/usr/bin/env bash
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# As per_adv_sync_term_leak.sh, but with CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS=y.
# Terminating a periodic sync while a chain is in flight must stop the auxiliary
# context; without the lll_sync cleanup fix the chain is stranded and no later
# sync can establish.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_per_adv_sync_term_leak_chains"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_sync_term_leak_chains_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 \
  -testid=per_adv_sync_term_leak_advertiser

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_periodic_prj_sync_term_leak_chains_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 \
  -testid=per_adv_sync_term_leak

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
