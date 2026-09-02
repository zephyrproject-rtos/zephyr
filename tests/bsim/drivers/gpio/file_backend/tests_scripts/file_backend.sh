#!/usr/bin/env bash
# Copyright (c) 2026 Demant A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXE_NAME="bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
CSV_FILE="${ZEPHYR_BASE}/$(guess_test_relpath)/test_data/gpio_in.csv"

cd ${BSIM_OUT_PATH}/bin

./${EXE_NAME} \
  -v=2 -nosim \
  -gpio_in_file="${CSV_FILE}"
