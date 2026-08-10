#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# Minimize the working corpus to a coverage-equivalent subset via
# -merge=1. Long campaigns run with a read-write corpus that grows without
# bound (by design -- see mqtt_campaign.sh); periodic merging is the
# required companion so throughput doesn't degrade from replaying
# thousands of redundant inputs.
#
# Merges into a NEW directory then atomically swaps -- never merges in
# place, so an interrupted merge cannot destroy the accumulated corpus,
# which lives outside the repo with no version control behind it.

set -ue

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${SCRIPT_DIR}/fuzz_common.source"

check_program_exists "${MQTT_FUZZ_EXE}"

before_count=$(find "${MQTT_FUZZ_CORPUS}" -type f | wc -l)

MERGE_DEST="${MQTT_FUZZ_CORPUS}.merged.$$"
MERGE_CONTROL="${MQTT_FUZZ_OUT}/merge_control"
mkdir -p "${MERGE_DEST}"

"${MQTT_FUZZ_EXE}" -merge=1 -merge_control_file="${MERGE_CONTROL}" \
	"${MERGE_DEST}" "${MQTT_FUZZ_CORPUS}"

after_count=$(find "${MERGE_DEST}" -type f | wc -l)

echo "corpus merge: ${before_count} -> ${after_count} files"

OLD_CORPUS="${MQTT_FUZZ_CORPUS}.pre-merge.$$"
mv "${MQTT_FUZZ_CORPUS}" "${OLD_CORPUS}"
mv "${MERGE_DEST}" "${MQTT_FUZZ_CORPUS}"
rm -rf "${OLD_CORPUS}"
