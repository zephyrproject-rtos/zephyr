#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Validate auxiliary event time reservation when updating AD data that is
# fragmented into chain PDUs while the advertising set is enabled
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_adv_aux_time"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_advx_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=conn_adv_aux_time

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=1 -sim_length=5e6 $@

wait_for_background_jobs
