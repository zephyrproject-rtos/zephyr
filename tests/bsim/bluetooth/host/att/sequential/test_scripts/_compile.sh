#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be defined}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"
INCR_BUILD=1
source ${ZEPHYR_BASE}/tests/bsim/compile.source

app="$(guess_test_relpath)"/dut compile
app="$(guess_test_relpath)"/tester compile

wait_for_background_jobs
