#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Simple selfchecking test for the 15.4 stack, based on the echo client / server sample apps
# It relies on the bs_tests hooks to register a test timer callback, which after a deadline
# will check how many packets the echo client has got back as expected, and if over a threshold
# it considers the test passed

simulation_id="echo_test_154"
verbosity_level=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=100

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_net_sockets_echo_test_prj_conf_overlay-802154_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -RealEncryption=1 \
  -testid=echo_client

Execute ./bs_${BOARD_TS}_samples_net_sockets_echo_server_prj_conf_overlay-802154_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=1 -RealEncryption=1 \

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=21e6 -argschannel -at=40 -argsmain $@

wait_for_background_jobs #Wait for all programs in background and return != 0 if any fails
