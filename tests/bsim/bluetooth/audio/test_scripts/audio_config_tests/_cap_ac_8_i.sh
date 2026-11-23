#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

set -e # Exit on error

SEARCH_PATH="${dir_path}"/cap_ac_8_i_*.sh  tests/bsim/run_parallel.sh
