#!/usr/bin/env bash
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

# Script to run all test scripts in subdirectories and generate JUnit XML report
#
# Usage:
#   1. Source Zephyr environment:
#      source zephyr/zephyr-env.sh
#
#   2. Run the script:
#      ./ci.bt.classic.sh
#

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

nice tests/bluetooth/classic/sim/ci.bt.classic.native_sim.compile.sh
tests/bluetooth/classic/sim/ci.bt.classic.native_sim.run.sh
