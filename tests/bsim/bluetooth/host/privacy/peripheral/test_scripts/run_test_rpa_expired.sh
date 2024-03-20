#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="rpa_expired"

central_exe_rpa_expired="\
${BSIM_OUT_PATH}/bin/bs_${BOARD}_$(guess_test_long_name)_prj_rpa_expired_conf"
peripheral_exe_rpa_expired="${central_exe_rpa_expired}"

cd ${BSIM_OUT_PATH}/bin

Execute "$central_exe_rpa_expired" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central_rpa_check \
    -RealEncryption=1 -flash="${simulation_id}.central.log.bin" -flash_erase

Execute "$peripheral_exe_rpa_expired" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral_rpa_expired \
    -RealEncryption=1 -flash="${simulation_id}.peripheral.log.bin" -flash_erase

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=70e6 $@

wait_for_background_jobs
