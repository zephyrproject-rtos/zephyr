/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_GNSS_TIMEPULSE));
BUILD_ASSERT(IS_ENABLED(CONFIG_GNSS_TIMEPULSE_GPIO));

int main(void)
{
	return 0;
}
