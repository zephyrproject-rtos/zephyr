#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Validate auxiliary event time reservation when updating AD data
# for connectable advertising
source $(dirname "$0")/adv_aux_time.source

run_sim ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_advx_prj_conf \
    conn_adv_aux_time
