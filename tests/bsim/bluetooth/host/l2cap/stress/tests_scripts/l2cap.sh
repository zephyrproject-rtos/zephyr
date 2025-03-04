#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -x

BOARD="nrf5340bsim/nrf5340/cpuapp"
#BOARD="nrf52_bsim"
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# EATT test
simulation_id="l2cap_stress"
verbosity_level=2
EXECUTE_TIMEOUT=240

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_stress_prj_conf

cd ${BSIM_OUT_PATH}/bin

#ATTACH_CMD=("btmon" "--tty" "%s" "--tty-speed" "115200" "&")
ATTACH_CMD="xterm -e btmon --tty %s --tty-speed 115200 &"

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=43 \
    -uart0_pty_attach_cmd="${ATTACH_CMD}" -uart_pty_wait

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=42 #--uart0_pty --uart_pty_wait
#Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=peripheral -rs=10
#Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=3 -testid=peripheral -rs=23
#Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=4 -testid=peripheral -rs=7884
#Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=5 -testid=peripheral -rs=230
#Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=6 -testid=peripheral -rs=9

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=400e6 $@

wait_for_background_jobs
