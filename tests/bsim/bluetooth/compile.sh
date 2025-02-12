#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the Bluetooth bsim tests

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Note: We do not parallelize the call into the build of the host, ll and mesh images as those
# are already building many images in parallel in themselves, and otherwise we would be
# launching too many parallel builds which can lead to a too high system load.
# On the other hand the audio compile script, only builds one image. So we parallelize it with
# the rest to save a couple of seconds.
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/audio_samples/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/ll/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/mesh/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/samples/compile.sh
if [ ${BOARD} == "nrf52_bsim" ]; then
	${ZEPHYR_BASE}/tests/bsim/bluetooth/hci_uart/compile.sh
fi

wait_for_background_jobs
