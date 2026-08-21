/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

ZTEST(uhc_dwc3_build, test_build)
{
	zassert_true(IS_ENABLED(CONFIG_UHC_DWC3));
}

ZTEST_SUITE(uhc_dwc3_build, NULL, NULL, NULL, NULL, NULL);
