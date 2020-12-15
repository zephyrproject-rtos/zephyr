/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <ztest.h>
#include <tc_util.h>

#include <drivers/edac.h>

#define DEVICE_NAME		"IBECC"

#if defined(CONFIG_EDAC_ERROR_INJECT)
#include <ibecc.h>

#define TEST_ADDRESS		0x2000
#define TEST_DATA		0xface
#define TEST_ADDRESS_MASK	INJ_ADDR_BASE_MASK_MASK
#define DURATION		100
#endif

const struct device *dev;

static void test_ibecc_initialized(void)
{
	dev = device_get_binding(DEVICE_NAME);
	zassert_not_null(dev, "Device not found");

	TC_PRINT("Test ibecc driver is initialized\n");

}

static volatile int interrupt;

static void callback(const struct device *d, void *data)
{
	interrupt++;
}

static void test_ibecc_api(void)
{
	uint64_t ret;

	/* Error log API */

	ret = edac_ecc_error_log_get(dev);
	zassert_equal(ret, 0, "Error log not zero");

	edac_ecc_error_log_clear(dev);

	ret = edac_parity_error_log_get(dev);
	zassert_equal(ret, 0, "Error log not zero");

	edac_parity_error_log_clear(dev);

	/* Error stat API */

	ret = edac_errors_cor_get(dev);
	zassert_equal(ret, 0, "Error correctable count not zero");

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, 0, "Error uncorrectable count not zero");

	/* Notification API */

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static void test_ibecc_error_inject_api(void)
{
	uint64_t val;
	int ret;

	ret = edac_inject_addr_set(dev, TEST_ADDRESS);
	zassert_equal(ret, 0, "Error setting inject address");

	val = edac_inject_addr_get(dev);
	zassert_equal(val, TEST_ADDRESS, "Read back value differs");

	ret = edac_inject_addr_mask_set(dev, TEST_ADDRESS_MASK);
	zassert_equal(ret, 0, "Error setting inject address mask");

	val = edac_inject_addr_mask_get(dev);
	zassert_equal(val, TEST_ADDRESS_MASK, "Read back value differs");

	/* Clearing registers */

	val = edac_inject_addr_set(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address");

	val = edac_inject_addr_get(dev);
	zassert_equal(val, 0, "Read back value differs");

	ret = edac_inject_addr_mask_set(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address mask");

	val = edac_inject_addr_mask_get(dev);
	zassert_equal(val, 0, "Read back value differs");
}
#else
static void test_ibecc_error_inject_api(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	ztest_test_suite(ibecc,
			 ztest_unit_test(test_ibecc_initialized),
			 ztest_unit_test(test_ibecc_api),
			 ztest_unit_test(test_ibecc_error_inject_api)
			);
	ztest_run_test_suite(ibecc);
}
