#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

bsim_bin="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"
test_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_id_settings_prj_conf"

west build -b nrf52_bsim -d build && \
        cp -v build/zephyr/zephyr.exe "${test_exe}"
