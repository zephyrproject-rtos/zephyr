#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="ll-throughput"
verbosity_level=2
EXECUTE_TIMEOUT=180

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_throughput_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -RealEncryption=1 -d=0 \
  -testid=central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_throughput_prj_conf\
  -v=${verbosity_level} -s=${simulation_id} -RealEncryption=1 -d=1 \
  -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=120e6 $@ -argschannel -at=40

wait_for_background_jobs
