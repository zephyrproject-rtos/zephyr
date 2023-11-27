#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

: "${ZEPHYR_BASE:?is a missing required environment variable}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

set -eu

dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
bin_dir="${BSIM_OUT_PATH}/bin"

cd "${dotslash}"

compile_path="${bin_dir}/bs_${BOARD}_"
compile_path+="$(realpath --relative-to "$(west topdir)"/zephyr prj.conf | tr /. _)"

west build -b nrf52_bsim
cp -v build/zephyr/zephyr.exe "${compile_path}"
