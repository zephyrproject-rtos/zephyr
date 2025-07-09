#!/usr/bin/env bash
# Copyright (c) 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Tests cleanup within the ble stack for L2CAP connections when
# peripheral disconnects early while the central still has
# buffers queued for sending. Using the base code, which
# has the central sending 20 SDUs to 6 different peripherals,
# the test has the peripherals disconnect as follows:
#
# Peripheral 1: disconnects after all 20 SDUs as before
# Peripheral 2: disconnects immediately before receiving anything
# Peripheral 3: disconnects after receiving first SDU
# Peripheral 4: disconnects after receiving first PDU in second SDU
# Peripheral 5: disconnects after receiving third PDU in third SDU
# Peripheral 6: disconnects atfer receiving tenth PDU in tenth SDU
#
# The central and peripherals check that all tx_pool and rx_pool
# buffers have been returned after the disconnect.
simulation_id="l2cap_stress_early_disconnect"
verbosity_level=2
EXECUTE_TIMEOUT=240

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_stress_prj_conf_overlay-early-disc_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=43

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
	-testid=peripheral_early_disconnect -rs=42
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=2 \
	-testid=peripheral_early_disconnect -rs=10
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=3 \
	-testid=peripheral_early_disconnect -rs=23
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=4 \
	-testid=peripheral_early_disconnect -rs=7884
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=5 \
	-testid=peripheral_early_disconnect -rs=230
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=6 \
	-testid=peripheral_early_disconnect -rs=9

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=7 -sim_length=400e6 $@

wait_for_background_jobs
