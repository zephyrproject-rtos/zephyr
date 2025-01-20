#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

export BOARD="nrf5340bsim/nrf5340/cpuapp"
#export BOARD="nrf52_bsim"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/host/l2cap/stress sysbuild=1 compile
#app=tests/bsim/bluetooth/host/l2cap/stress conf_file=prj_nofrag.conf compile
#app=tests/bsim/bluetooth/host/l2cap/stress conf_file=prj_syswq.conf compile

wait_for_background_jobs
