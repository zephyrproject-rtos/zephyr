#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# CI entry point for the MQTT fuzz campaign. Direct analogue of
# tests/bsim/ci.bt.sh: everything else here is a component; this is the one
# file a CI job needs to know about.
#
# Sequence: compile (sanitized) -> campaign, bounded ONLY by an externally
# imposed timeout -> triage -> fail iff a real reproducer was found.
#
# The bound must come from OUTSIDE the fuzzer: MQTT_FUZZ_CI_DURATION seconds
# (default 3300), enforced here with timeout(1). Being killed by timeout
# (exit 124) is a normal ending, not a failure -- pass/fail comes entirely
# from triage, so that CI and a local indefinite run are the same binary
# with the same flags, differing only in how long they're allowed to live.

set -ue

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/fuzz_common.source"

MQTT_FUZZ_CI_DURATION="${MQTT_FUZZ_CI_DURATION:-3300}"

"${SCRIPT_DIR}/compile.sh"

set +e
timeout --kill-after=10 "${MQTT_FUZZ_CI_DURATION}" \
	"${SCRIPT_DIR}/fuzz_scripts/mqtt_campaign.sh"
campaign_rc=$?
set -e

if [ "${campaign_rc}" -ne 0 ] && [ "${campaign_rc}" -ne 124 ]; then
	echo "campaign exited abnormally (rc=${campaign_rc}, not a timeout) -- treating as a failure"
	exit "${campaign_rc}"
fi

"${SCRIPT_DIR}/fuzz_scripts/mqtt_triage.sh"
