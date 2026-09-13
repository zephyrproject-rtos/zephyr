#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# Build the MQTT fuzz campaign binary.  Direct analogue of tests/bsim/compile.sh:
# this is the "twister owns building" half of the split for the one binary
# twister itself cannot own, because the indefinite campaign is not a twister
# scenario.
#
# Usage:
#   compile.sh              build with CONFIG_ASAN=y CONFIG_UBSAN=y CONFIG_ASSERT=y
#   compile.sh --coverage   build with CONFIG_COVERAGE=y instead (gcov/lcov flow)
#
# Sanitizers and coverage instrumentation are deliberately separate builds:
# combined, each distorts the other's numbers (throughput under coverage,
# and instruction counts under sanitizers).

set -ue

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/fuzz_common.source"

EXTRA_CONFIGS=(-DCONFIG_ASAN=y -DCONFIG_UBSAN=y -DCONFIG_ASSERT=y -DCONFIG_BOOT_BANNER=n)

if [ "${1:-}" = "--coverage" ]; then
	EXTRA_CONFIGS=(-DCONFIG_COVERAGE=y -DCONFIG_BOOT_BANNER=n)
fi

west build -p always -b native_sim/native/64 "${SCRIPT_DIR}" \
	-d "${MQTT_FUZZ_BUILD}" \
	-- "${EXTRA_CONFIGS[@]}"
