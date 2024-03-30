#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic Connected ISO test: a Central connects to 1 Peripheral and tests RTN=2,
# FT=2, skips 2 subevents in the central
simulation_id="connected_iso_acl_first_ft_cen_skip_2_se"
verbosity_level=2
EXECUTE_TIMEOUT=60

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_ft_cen_skip_2_se_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_ft_cen_skip_2_se_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
