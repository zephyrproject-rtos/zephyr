#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The central first finds the decoy, which is connectable but accepts no connection request, so
# that attempt fails. The peripheral starts 3 s late, so that the central finds it only when it
# scans again.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_ead_sample_conn_retry"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_encrypted_ead_sample_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_retry -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_encrypted_ead_sample_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1 \
  -delay_init -start_offset=3e6

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_adv_encrypted_ead_sample_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=decoy

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs
