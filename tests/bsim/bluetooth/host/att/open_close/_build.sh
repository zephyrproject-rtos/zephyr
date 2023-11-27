#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu
dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
bin_dir="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"

cd "${dotslash}"

compile_path="${bin_dir}/bs_${BOARD}_"
compile_path+="$(realpath --relative-to "$(west topdir)"/zephyr prj.conf | tr /. _)"

west build -b nrf52_bsim
cp -v build/zephyr/zephyr.exe "${compile_path}"
