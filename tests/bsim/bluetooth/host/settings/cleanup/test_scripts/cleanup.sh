#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_exe="bs_${BOARD}_tests_bsim_bluetooth_host_settings_cleanup_prj_conf"
simulation_id="settings_cleanup"
verbosity_level=2
EXECUTE_TIMEOUT=30

settings_file="${simulation_id}.log.bin"

cd ${BSIM_OUT_PATH}/bin

if [ -e "${settings_file}" ]; then
  rm "${settings_file}"
fi

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}_1" -d=0 -testid=tester \
  -flash="${settings_file}"

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}_1" \
  -D=1 -sim_length=60e6

wait_for_background_jobs

Execute "./${test_exe}" \
  -v=${verbosity_level} -s="${simulation_id}_2" -d=0 -testid=dut \
  -flash="${settings_file}" -flash_rm

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}_2" \
  -D=1 -sim_length=60e6

wait_for_background_jobs
