#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

test_name="$(basename "$(realpath "$bash_source_dir/..")")"
bsim_bin="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"
test_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_adv_resume_prj_conf"
test_2_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_adv_resume_prj_2_conf"
