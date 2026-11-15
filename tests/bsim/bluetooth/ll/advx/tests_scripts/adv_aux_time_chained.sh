#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Validate auxiliary event time reservation when updating AD data that is
# fragmented into chain PDUs while the advertising set is enabled
source $(dirname "$0")/adv_aux_time.source

run_sim ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_advx_prj_conf_overlay-chained_conf \
    chained_adv_aux_time
