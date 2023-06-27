/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sensing/sensing.h>
#include <zephyr/ztest.h>

static void sensing_after(void *f)
{
	ARG_UNUSED(f);

	sensing_reset_connections();
}

ZTEST_SUITE(sensing, NULL, NULL, NULL, sensing_after, NULL);
