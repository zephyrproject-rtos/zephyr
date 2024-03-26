#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be defined}"

INCR_BUILD=1

source ${ZEPHYR_BASE}/tests/bsim/compile.source
app="$(guess_test_relpath)" _compile
