#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/bap_broadcast_sink/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/bap_unicast_client/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/cap/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/ccp/compile.sh

wait_for_background_jobs
