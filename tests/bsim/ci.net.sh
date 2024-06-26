#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI networking tests.
# It can also be run locally.
# Note it will produce its output in ${ZEPHYR_BASE}/bsim_bt/

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

WORK_DIR=${ZEPHYR_BASE}/bsim_net nice tests/bsim/net/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.net.52.xml \
SEARCH_PATH=tests/bsim/net/ tests/bsim/run_parallel.sh
