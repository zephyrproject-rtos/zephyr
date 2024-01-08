#!/bin/env bash
# Copyright 2023 Codecoup
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

test_name="$(basename "$(realpath "$bash_source_dir/..")")"
bsim_bin="${BSIM_OUT_PATH}/bin"
verbosity_level=2
BOARD="${BOARD:-nrf52_bsim}"
test_exe_d0="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_att_${test_name}_client_prj_conf"
test_exe_d1="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_att_${test_name}_server_prj_conf"
