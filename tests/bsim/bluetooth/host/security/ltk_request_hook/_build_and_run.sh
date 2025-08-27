#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

: "${ZEPHYR_BASE:?is a missing required environment variable}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source
set -eu

dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

cd "${dotslash}"

echo "Running twister --no-clean"
west twister -T . -p "${BOARD}" -i --no-clean
exec ./run.sh
