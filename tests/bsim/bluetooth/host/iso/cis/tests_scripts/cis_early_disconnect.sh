#!/usr/bin/env bash
# Copyright (c) 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Tests cleanup within the ble stack for ISO connections when
# peripheral disconnects early while the central still has
# buffers queued for sending. Using the base code, which
# has the central sending 100 ISO packets, the peripheral
# triggers a disconnect after receiving 10 packets.
# The central checks that all tx_pool buffers have been
# returned to the pool after the disconnect.
simulation_id="iso_cis_early_disconnect"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_cis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_host_iso_cis_prj_conf \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_early_disconnect

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=30e6 $@

wait_for_background_jobs
