#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="$(guess_test_long_name)"
verbosity_level=2
test_exe="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

cd ${BSIM_OUT_PATH}/bin

data_set=1

Execute ./"$test_exe" \
    -v=${verbosity_level} -s="${simulation_id}_${data_set}" -d=0 -testid=central \
    -RealEncryption=1 -argstest data-set="${data_set}"

Execute ./"$test_exe" \
    -v=${verbosity_level} -s="${simulation_id}_${data_set}" -d=1 -testid=peripheral \
    -RealEncryption=1 -argstest data-set="${data_set}"

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}_${data_set}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs

data_set=2

Execute ./"$test_exe" \
    -v=${verbosity_level} -s="${simulation_id}_${data_set}" -d=0 -testid=central \
    -RealEncryption=1 -argstest data-set="${data_set}"

Execute ./"$test_exe" \
    -v=${verbosity_level} -s="${simulation_id}_${data_set}" -d=1 -testid=peripheral \
    -RealEncryption=1 -argstest data-set="${data_set}"

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}_${data_set}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
