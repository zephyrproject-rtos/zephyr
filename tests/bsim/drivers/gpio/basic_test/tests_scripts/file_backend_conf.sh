#!/usr/bin/env bash
# Copyright (c) 2026 Demant A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Test the GPIO driver and HW model, where the several GPIO inputs are driven from several csv files

REL_PATH="$(guess_test_relpath)"
EXE_NAME="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
GPIO_CONF_FILE="test_data/gpio_config.txt"
CSV_FILE="test_data/gpio_in.csv"

# conf file paths are relative to this test folder
cd ${ZEPHYR_BASE}/${REL_PATH}/

${BSIM_OUT_PATH}/bin/${EXE_NAME} \
  -v=2 -nosim \
  -gpio_conf_file="${GPIO_CONF_FILE}" \
  -gpio_in_file="${CSV_FILE}" \
  -testid=file_backend
