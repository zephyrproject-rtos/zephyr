/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/pipelink.h>

#define TEST_NODE DT_NODELABEL(test_node)
#define TEST_PIPELINK_NAME test_pipelink_0

/* Define pipelink in separate source file to test MODEM_PIPELINK_DT_DECLARE() */
MODEM_PIPELINK_DT_DEFINE(TEST_NODE, TEST_PIPELINK_NAME);
