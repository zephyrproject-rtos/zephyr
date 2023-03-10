#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

test_name="$(basename "$(realpath "$bash_source_dir/..")")"
bsim_bin="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"
test_exe="${bsim_bin}/bs_${BOARD}_tests_bluetooth_bsim_host_gatt_settings_prj_conf"
test_2_exe="${bsim_bin}/bs_${BOARD}_tests_bluetooth_bsim_host_gatt_settings_prj_2_conf"
