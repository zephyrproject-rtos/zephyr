#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

bsim_bin="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"
central_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_privacy_peripheral_prj_conf"
peripheral_exe="${central_exe}"
central_exe_rpa_sharing="\
${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_privacy_peripheral_prj_rpa_sharing_conf"
peripheral_exe_rpa_sharing="${central_exe_rpa_sharing}"
