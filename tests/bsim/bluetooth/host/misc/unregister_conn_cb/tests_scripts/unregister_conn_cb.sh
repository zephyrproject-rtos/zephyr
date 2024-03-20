#!/usr/bin/env bash
# Copyright (c) 2024 NXP
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

#Unregister connection callbacks : A central device scans for and connects to a peripheral
#after registered connection callbacks.When the connection state changes, few printing
#will be printed and the flag_is_connected will be changed by the callback function.
#After unregister the connection callbacks, reconnect to peer device, then no printing
#neither flag change can be found, callback function was unregistered as expected
simulation_id="unregister_conn_cb"
verbosity_level=2

bsim_exe=./bs_${BOARD}_tests_bsim_bluetooth_host_misc_unregister_conn_cb_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=420
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=100

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=30e6 $@

wait_for_background_jobs
