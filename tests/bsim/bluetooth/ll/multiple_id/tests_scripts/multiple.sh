#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Multiple connection between two devices with multiple peripheral identity
simulation_id="multiple"
verbosity_level=2
EXECUTE_TIMEOUT=1800

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_multiple_id_prj_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_multiple_id_prj_conf\
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=4500e6 $@ -argschannel -at=40

wait_for_background_jobs
