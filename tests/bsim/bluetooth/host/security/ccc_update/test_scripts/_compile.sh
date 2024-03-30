#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

source "${bash_source_dir}/_env.sh"

west build -b nrf52_bsim -d build_test && \
        cp -v build_test/zephyr/zephyr.exe "${test_exe}"

west build -b nrf52_bsim -d build_test_2 -- -DCONF_FILE=prj_2.conf && \
        cp -v build_test_2/zephyr/zephyr.exe "${test_exe_2}"
