#!/usr/bin/env bash

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

set -eu
dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

exe_dst="${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_tests_bsim_bluetooth_host_adv_set_adva_prj_conf"

cd "$dotslash"
west build -b nrf52_bsim
cp -v --remove-destination ./build/zephyr/zephyr.exe "$exe_dst"
