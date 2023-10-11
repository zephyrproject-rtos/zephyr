#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Usage: Place yourself in the test's root (i.e. ./../)

export BOARD=nrf52_bsim

# Path checks, etc
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

rm -rf ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_tests*

# terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

bsim_exe=bs_nrf52_bsim_tests_bsim_bluetooth_host_l2cap_credits_seg_recv_prj_conf
west build -b nrf52_bsim && \
    cp build/zephyr/zephyr.exe ${BSIM_OUT_PATH}/bin/${bsim_exe}

bsim_exe=bs_nrf52_bsim_tests_bsim_bluetooth_host_l2cap_credits_seg_recv_prj_ecred_conf
west build -b nrf52_bsim -d build_ecred -- -DCONF_FILE=prj_ecred.conf && \
    cp build_ecred/zephyr/zephyr.exe ${BSIM_OUT_PATH}/bin/${bsim_exe}
