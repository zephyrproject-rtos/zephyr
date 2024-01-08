#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

test_name="id_addr_update"
bsim_bin="${BSIM_OUT_PATH}/bin"
verbosity_level=2
board="${BOARD:-nrf52_bsim}"
simulation_id="$test_name"
test_path="tests_bsim_bluetooth_host_security_${test_name}"
central_exe="${bsim_bin}/bs_${board}_${test_path}_central_prj_conf"
peripheral_exe="${bsim_bin}/bs_${board}_${test_path}_peripheral_prj_conf"

function print_var {
	# Print a shell-sourceable variable definition.
	local var_name="$1"
	local var_repr="${!var_name@Q}"
	echo "$var_name=$var_repr"
}

print_var test_name
print_var bsim_bin
print_var verbosity_level
print_var board
print_var simulation_id
print_var central_exe
print_var peripheral_exe
