#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

SEARCH_PATH="${dir_path}"/gmap_ac_6_ii_*.sh  tests/bsim/run_parallel.sh
