#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all bluetooth applications needed for the split stack tests

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

export BOARD="${BOARD:-nrf5340bsim/nrf5340/cpuapp}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf sysbuild=1  compile
app=tests/bsim/bluetooth/ll/bis sysbuild=1 compile
app=tests/bsim/bluetooth/ll/cis conf_overlay=overlay-acl_group.conf sysbuild=1 compile

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/samples/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/compile.sh

wait_for_background_jobs
