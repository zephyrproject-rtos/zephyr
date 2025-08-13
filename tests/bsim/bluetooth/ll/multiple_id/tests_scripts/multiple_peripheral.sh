#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Multiple connection between two devices with multiple peripheral identity
simulation_id="central_multiple_peripheral_single"
verbosity_level=2
EXECUTE_TIMEOUT=1600

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_multiple_id_prj_conf -RealEncryption=1 \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_multiple

peripheral_count=20

for device in `seq 1 $peripheral_count`; do
	let rs=$device*7

	Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_multiple_id_prj_conf -RealEncryption=1 \
	  -v=${verbosity_level} -s=${simulation_id} -d=$device -rs=$rs \
	  -testid=peripheral_single
done

let device_count=$peripheral_count+1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=$device_count -sim_length=1800e6 $@ -argschannel -at=40

wait_for_background_jobs
