#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

dut_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_dut_prj_conf"
tester_exe="bs_${BOARD_TS}_tests_bsim_bluetooth_host_att_pipeline_tester_prj_conf"

simulation_id="att_pipeline"
verbosity_level=2
sim_length_us=100e6
EXECUTE_TIMEOUT=240

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=${sim_length_us} $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=tester_1 -RealEncryption=1 -rs=100

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut_1 -RealEncryption=1

wait_for_background_jobs

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=3 -sim_length=${sim_length_us} $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=2 -testid=tester -RealEncryption=1 -rs=100

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=dut -RealEncryption=1 -rs=2000

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut -RealEncryption=1

wait_for_background_jobs
