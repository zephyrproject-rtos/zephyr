#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by all bsim tests

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/net/compile.sh

wait_for_background_jobs
