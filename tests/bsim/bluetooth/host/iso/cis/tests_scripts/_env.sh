#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

test_name="$(basename "$(realpath "$bash_source_dir/..")")"
bsim_bin="${BSIM_OUT_PATH}/bin"
verbosity_level=2
BOARD="${BOARD:-nrf52_bsim}"
simulation_id="$test_name"
central_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_iso_cis_prj_conf"
peripheral_exe="${central_exe}"
