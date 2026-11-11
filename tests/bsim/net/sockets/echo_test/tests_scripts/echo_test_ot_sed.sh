#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# Copyright 2023 Nordic Semiconductor ASA
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Same as echo_test_ot.sh, but the echo client is a Thread Sleepy End Device
# attached to the echo server. The client's radio sleeps between transmissions,
# including right after it sends an Ack.

simulation_id="${BOARD_TS}_echo_test_ot_sed"
verbosity_level=2

EXECUTE_TIMEOUT=100

cd ${BSIM_OUT_PATH}/bin

client_exe=bs_${BOARD_TS}_tests_bsim_net_sockets_echo_test_prj_conf_overlay-ot_conf

Execute ./${client_exe}_ot-sed_conf \
  -v=${verbosity_level} -s=${simulation_id} -start_offset=2e6 -d=0 -RealEncryption=1 \
  -testid=echo_client

Execute ./bs_${BOARD_TS}_samples_net_sockets_echo_server_prj_conf_overlay-ot_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=40e6 -argschannel -at=40 -argsmain $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
