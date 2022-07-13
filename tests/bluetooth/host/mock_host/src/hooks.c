/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>

#include "hooks.h"

void hooks_net_buf_alloc_fixed_timeout_validation_hook(uint32_t value)
{
	ztest_check_expected_value(value);
}
