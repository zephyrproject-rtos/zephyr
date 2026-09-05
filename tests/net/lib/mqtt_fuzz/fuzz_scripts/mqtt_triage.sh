#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# Triage artifacts left in ${MQTT_FUZZ_OUT}/artifacts by a fuzz campaign.
#
# The target can reach exit() via nsi_exit(), which libFuzzer cannot
# distinguish from a crash -- so "there is a crash-* file" does NOT imply
# "there is a bug".  This script turns a directory of opaque blobs into
# either false-positive corpus candidates or real, minimized reproducers
# with sanitizer output attached.
#
# Exit status: nonzero if ANY artifact reproduces, zero otherwise (including
# an empty/missing artifacts directory).  This is what lets ci.fuzz.sh fail a
# job on real findings only.

set -ue

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${SCRIPT_DIR}/fuzz_common.source"

check_program_exists "${MQTT_FUZZ_EXE}"

# Accept a directory of artifacts to triage; defaults to the campaign's own
# artifact directory but can point at e.g. stray root-level crash-* files.
ARTIFACT_DIR="${1:-${MQTT_FUZZ_ARTIFACTS}}"

found_real=0

if [ -d "${ARTIFACT_DIR}" ]; then
	shopt -s nullglob
	for artifact in "${ARTIFACT_DIR}"/*; do
		[ -f "${artifact}" ] || continue

		name="$(basename "${artifact}")"

		if "${MQTT_FUZZ_EXE}" "${artifact}" >/tmp/mqtt_triage_replay.$$ 2>&1; then
			echo "FALSE POSITIVE (exit 0, nsi_exit() path): ${name}"
			mv "${artifact}" "${MQTT_FUZZ_CORPUS}/${name}"
		else
			echo "REAL: ${name} reproduces a crash"
			minimized="${MQTT_FUZZ_ARTIFACTS}/minimized-${name}"
			"${MQTT_FUZZ_EXE}" -minimize_crash=1 -exact_artifact_path="${minimized}" "${artifact}" \
				>/tmp/mqtt_triage_minimize.$$ 2>&1 || true
			echo "  sanitizer output:"
			sed 's/^/    /' /tmp/mqtt_triage_replay.$$
			if [ -f "${minimized}" ]; then
				echo "  minimized reproducer (hex): $(xxd -p "${minimized}" | tr -d '\n')"
				echo "  reproduce with: ${MQTT_FUZZ_EXE} ${minimized}"
			else
				echo "  reproduce with: ${MQTT_FUZZ_EXE} ${artifact}"
			fi
			found_real=1
		fi
		rm -f /tmp/mqtt_triage_replay.$$ /tmp/mqtt_triage_minimize.$$
	done
fi

exit "${found_real}"
