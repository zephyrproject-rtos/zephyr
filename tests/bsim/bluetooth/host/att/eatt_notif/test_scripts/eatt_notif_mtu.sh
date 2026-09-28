#!/usr/bin/env bash
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# A notification restricted to EATT that exceeds the ATT_MTU of every EATT
# bearer must not hold up the notification queued after it.

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_eatt_notif_mtu"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_eatt_notif_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=client_mtu -RealEncryption=1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_eatt_notif_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=server_mtu -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=10e6 $@

wait_for_background_jobs
