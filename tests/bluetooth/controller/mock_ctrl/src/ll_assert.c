/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <stdlib.h>
#include <inttypes.h>

void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	printf("Assertion failed in %s:%" PRIu32 "\n", file, line);
	exit(-1);
}
