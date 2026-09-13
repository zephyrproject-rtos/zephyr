#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# Indefinite MQTT fuzz campaign runner.  Twister cannot express an unbounded
# run -- every scenario has a timeout, and exceeding it is by definition a
# failure -- so real, days-long campaigns run through this script instead,
# mirroring how tests/bsim/*/tests_scripts/*.sh drive execution twister
# cannot own.
#
# Deliberately NO -max_total_time and NO -runs=N.  If a caller wants a bound
# they impose it externally (timeout(1), or their CI job's own limit) --
# never bake one in here.  See ci.fuzz.sh for the externally-bounded case.

set -ue

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${SCRIPT_DIR}/fuzz_common.source"

check_program_exists "${MQTT_FUZZ_EXE}"

"${MQTT_FUZZ_EXE}" \
	"${MQTT_FUZZ_CORPUS}" \
	-dict="${SCRIPT_DIR}/mqtt5.dict" \
	-artifact_prefix="${MQTT_FUZZ_ARTIFACTS}/" \
	-max_len=4096 \
	-fork=1 \
	-jobs="$(nproc)" \
	-print_final_stats=1
