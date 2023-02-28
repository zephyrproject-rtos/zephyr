#!/usr/bin/env bash
# Copyright 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Basic Connected ISO test: a Central connects to 9 Peripherals and Establishes
# 1 CIS each to 9 Peripherals (9 CIS in a CIG)
simulation_id="connected_iso_acl_first"
verbosity_level=2
process_ids=""; exit_code=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 120 $@ & process_ids="$process_ids $!"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral1

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=peripheral2

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=3 -testid=peripheral3

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=4 -testid=peripheral4

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=5 -testid=peripheral5

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=6 -testid=peripheral6

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=7 -testid=peripheral7

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=8 -testid=peripheral8

Execute ./bs_${BOARD}_tests_bsim_bluetooth_ll_cis_prj_conf_overlay-acl_first_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=9 -testid=peripheral9

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=10 -sim_length=60e6 $@

for process_id in $process_ids; do
  wait $process_id || let "exit_code=$?"
done
exit $exit_code #the last exit code != 0
