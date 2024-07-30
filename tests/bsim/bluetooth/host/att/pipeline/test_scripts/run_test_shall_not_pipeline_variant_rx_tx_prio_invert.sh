#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#  Test purpose:
#
#  Basic check that the DUT GATT client does not pipeline ATT
#  requests.
#
#  Sending a new request on a bearer before the response to the
#  previous request has been received ('pipelining') is a
#  protocol violation.
#
#  Test variant 'rx_tx_prio_invert':
#
#  - The priority of the RX and TX threads is inverted compared
#    to the default configuration. The result shall be the same.
#
#  Test procedure:
#
#  - DUT is excercised by calling `gatt_write` in a loop.
#  - Tester does not immediately respond but delays the response
#    a bit to ensure the LL has time to transport any extra
#    requests, exposing a bug.
#  - Tester verifies there is no such extra request while it's
#    delaying the response. Detecting an extra request proves a
#    protocol violation.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

dut_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_dut_prj_conf"
dut_exe+="_rx_tx_prio_invert_extra_conf"
tester_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_tester_prj_conf"

simulation_id="att_pipeline_test_shall_not_pipeline_variant_rx_tx_prio_invert"
verbosity_level=2
sim_length_us=100e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=${sim_length_us} $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=tester_1 -RealEncryption=1 -rs=100

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut_1 -RealEncryption=1

wait_for_background_jobs
