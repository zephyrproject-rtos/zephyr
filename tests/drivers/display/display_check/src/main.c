/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "display.h"

ZTEST(display_test, test_display_by_capture)
{
	sample_display_draw();
}

ZTEST_SUITE(display_test, NULL, NULL, NULL, NULL, NULL);
