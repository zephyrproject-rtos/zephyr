/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Run this test from zephyr directory as:
 *
 *     ./scripts/twister --coverage -p native_posix -v -T tests/bluetooth/ctrl_isoal/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/ztest_mock.h>

#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

/* Include the DUT */
#include "ll_sw/isoal.c"

#include "isoal_test_common.h"
#include "isoal_test_debug.h"

/* Include Test Subsets */
#include "sub_sets/isoal_test_rx.c"
#include "sub_sets/isoal_test_tx.c"


ZTEST_SUITE(test_rx_basics, NULL, NULL, isoal_test_rx_common_before, NULL, NULL);
ZTEST_SUITE(test_rx_unframed, NULL, NULL, isoal_test_rx_common_before, NULL, NULL);
ZTEST_SUITE(test_rx_framed, NULL, NULL, NULL, isoal_test_rx_common_before, NULL);

ZTEST_SUITE(test_tx_basics, NULL, NULL, isoal_test_tx_common_before, NULL, NULL);
ZTEST_SUITE(test_tx_unframed, NULL, NULL, isoal_test_tx_common_before, NULL, NULL);
ZTEST_SUITE(test_tx_framed, NULL, NULL, isoal_test_tx_common_before, NULL, NULL);

ZTEST_SUITE(test_tx_framed_ebq, NULL, NULL, isoal_test_tx_common_before, NULL, NULL);
