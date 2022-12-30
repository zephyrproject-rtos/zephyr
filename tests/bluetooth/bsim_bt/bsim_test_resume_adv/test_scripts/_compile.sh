#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

# Read variable definitions output by _env.sh
source <("${bash_source_dir}/_env.sh")

cd ${peripheral_path}
# Place yourself in the test's root (i.e. ./../)
west build -b nrf52_bsim && \
    cp build/zephyr/zephyr.exe "${peripheral_exe}"

cd ${central_path}
# Place yourself in the test's root (i.e. ./../)
west build -b nrf52_bsim && \
    cp build/zephyr/zephyr.exe "${central_exe}"
    
 cd ${central_peripheral_path}
# Place yourself in the test's root (i.e. ./../)
west build -b nrf52_bsim && \
    cp build/zephyr/zephyr.exe "${central_peripheral_exe}"
