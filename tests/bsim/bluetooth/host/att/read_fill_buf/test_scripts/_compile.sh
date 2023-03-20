#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

source "${bash_source_dir}/_env.sh"

pushd client
west build -b nrf52_bsim && \
	cp -v build/zephyr/zephyr.exe "${test_exe_d0}"
popd

pushd server
west build -b nrf52_bsim && \
	cp -v build/zephyr/zephyr.exe "${test_exe_d1}"
popd
