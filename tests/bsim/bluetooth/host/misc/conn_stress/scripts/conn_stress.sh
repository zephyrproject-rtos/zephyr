#!/usr/bin/env bash
# Copyright (c) 2023-2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="conn_stress"

test_path="tests_bsim_bluetooth_host_misc_conn_stress"
bsim_central_exe_name="bs_${BOARD_TS}_${test_path}_central_prj_conf"
bsim_peripheral_exe_name="bs_${BOARD_TS}_${test_path}_peripheral_prj_conf"
bsim_args="-RealEncryption=1 -v=2 -s=${simulation_id}"
test_args="-argstest notify_size=220 conn_interval=32"

EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

for device in `seq 1 12`; do
    let rs=$device*100

    Execute "./${bsim_peripheral_exe_name}" ${bsim_args} \
        -d=$device -rs=$rs -testid=peripheral ${test_args}
done

Execute "./${bsim_central_exe_name}" ${bsim_args} -d=0 -rs=1 -testid=central ${test_args}

Execute ./bs_2G4_phy_v1 -dump -v=2 -s=${simulation_id} -D=13 -sim_length=100e6

wait_for_background_jobs
