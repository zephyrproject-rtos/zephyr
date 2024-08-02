#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#  Test purpose:
#
#  Check that DUT GATT server gracefully handles a pipelining
#  client.
#
#  The DUT GATT server must remain available to a well-behaved
#  peer while a bad peer tries to spam ATT requests.
#
#  Test variant 'rx_tx_prio_invert':
#
#  - The priority of the RX and TX threads is inverted compared
#    to the default configuration. The result shall be the same.
#
#  Test procedure:
#
#  - The well-behaved peer performs a discovery procedure
#    repeatedly.
#  - The bad peer spams ATT requests as fast as possible.
#  - The connection with the well-behaved peer shall remain
#    responsive.
#  - Either: The DUT may disconnect the bad peer ACL after
#    receiving a protocol violation occurs. The bad peer shall
#    be able to reconnect and continue the bad behavior.
#  - Or: The DUT may process and respond to the pipelined
#    requests, preserving their ordering.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

dut_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_dut_prj_conf"
dut_exe+="_rx_tx_prio_invert_extra_conf"
tester_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_tester_prj_conf"

simulation_id="att_pipeline_test_tolerate_pipeline_variant_rx_tx_prio_invert"
verbosity_level=2
sim_length_us=100e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=3 -sim_length=${sim_length_us} $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=2 -testid=tester -RealEncryption=1 -rs=100

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=dut -RealEncryption=1 -rs=2000

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut -RealEncryption=1

wait_for_background_jobs
