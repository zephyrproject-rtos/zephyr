#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Chain variant: peer sends ~1000 B of advertising data spanning multiple
# AUX_CHAIN_IND PDUs, producing fragmented HCI ext adv reports.
# Tests chain-aware discard logic and the host reassembly timer.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# This variant only runs on nRF5340 as it uses the IPC driver with chain-aware discard logic.
if [ "${BOARD_TS}" != "nrf5340bsim_nrf5340_cpuapp" ]; then
    echo "Skipping test for ${BOARD_TS}"
    exit 0
fi

test_name="$(guess_test_long_name)"
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

simulation_id="${BOARD_TS}_${test_name}_chain"
verbosity_level=2
zephyr_log_level=3

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
  -D=2 -sim_length=100e6

Execute "${test_exe}" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 \
    -testid=peer -argstest log_level=${zephyr_log_level} chain=1

Execute "${test_exe}" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 \
    -testid=dut -argstest log_level=${zephyr_log_level} chain=1

wait_for_background_jobs
