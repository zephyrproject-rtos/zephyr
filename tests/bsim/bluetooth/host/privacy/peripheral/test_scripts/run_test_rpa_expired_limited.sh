#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# An advertising set whose rpa_expired callback keeps the RPA ends limited advertising, with
# RPA sharing enabled: once stopped by the application, once by its timeout.

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="${BOARD_TS}_$(guess_test_long_name)_rpa_expired_limited"

dut_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_rpa_expired_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$dut_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=peripheral_rpa_expired_limited \
    -RealEncryption=1 -flash="${simulation_id}.dut.log.bin" -flash_rm

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=1 -sim_length=10e6 $@

wait_for_background_jobs
