#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

test_name="$(basename "$(realpath "$bash_source_dir/..")")"
bsim_bin="${BSIM_OUT_PATH}/bin"
verbosity_level=2
BOARD="${BOARD:-nrf52_bsim}"
simulation_id="$test_name"
central_exe="${bsim_bin}/bs_${BOARD}_tests_bluetooth_bsim_bt_${test_name}_prj_conf"
peripheral_exe="${bsim_bin}/bs_${BOARD}_tests_bluetooth_bsim_bt_${test_name}_prj_conf"

function print_var {
	# Print a shell-sourceable variable definition.
	local var_name="$1"
	local var_repr="${!var_name@Q}"
	echo "$var_name=$var_repr"
}

print_var test_name
print_var bsim_bin
print_var verbosity_level
print_var BOARD
print_var simulation_id
print_var central_exe
print_var peripheral_exe
