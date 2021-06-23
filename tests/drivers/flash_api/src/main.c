/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/flash.h>
#include <device.h>

/** Test instrumentation **/
/*
 * Below structure gathers variables that may be used by simulate API calls to mock behaviour
 * of a flash device; the variables may be used however it is desired, because the it is
 * up to mocked function and check afterwards to relate a variable to result
 */
static struct {
	/* Set to value returned from any API call */
	int ret;
	/* Some offset */
	off_t offset;
	/* Some size */
	size_t size;
	/* Allowed offset alignment */
} api_test_values = {
	.ret = 0,
	.offset = 0,
	.size = 0,
};

/*** Device definition atd == API Test Dev ***/
/** Device API callbacks **/
static int atd_get_page_info(const struct device *dev, off_t offset, struct flash_page_info *fpi)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(fpi != NULL);

	fpi->offset = offset;
	fpi->size = api_test_values.size;

	return api_test_values.ret;
}

static ssize_t atd_get_page_count(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	return api_test_values.size;
}

static ssize_t atd_get_size(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	return api_test_values.size;
}

/** Device objects **/
/* The device state */
static struct device_state atd_state = {
	.init_res = 0,
	.initialized = 1,
};

/* Device structure */
static struct flash_driver_api atd_op = {
	.get_page_info = atd_get_page_info,
	.get_page_count = atd_get_page_count,
	.get_size = atd_get_size,
};

/* The device */
static struct device atd = {
	"test_flash", NULL, &atd_op, &atd_state, NULL, NULL,
};

static void test_get_page_info(void)
{
	struct flash_page_info pi;

	/* Check if flash_get_page_info properly calls the get_page_info API call */
	api_test_values.offset = 10;
	api_test_values.size = 30;
	api_test_values.ret = 40;
	zassert_equal(flash_get_page_info(&atd, api_test_values.offset, &pi),
		api_test_values.ret, "Other ret value expected");
	zassert_equal(pi.offset, api_test_values.offset, "Offset mismatch");
	zassert_equal(pi.size, api_test_values.size, "Size mismatch");
	/* Never trust one call */
	api_test_values.offset = 11;
	api_test_values.size = 32;
	api_test_values.ret = 43;
	zassert_equal(flash_get_page_info(&atd, api_test_values.offset, &pi),
		api_test_values.ret, "Other ret value expected");
	zassert_equal(pi.offset, api_test_values.offset, "Offset mismatch");
	zassert_equal(pi.size, api_test_values.size, "Size mismatch");
}

static void test_get_page_count(void)
{
	api_test_values.size = 30;
	zassert_equal(flash_get_page_count(&atd), api_test_values.size, "Page count mismatch");
	api_test_values.size = 31;
	zassert_equal(flash_get_page_count(&atd), api_test_values.size, "Page count mismatch");
}

static void test_get_size(void)
{
	api_test_values.size = 45;
	zassert_equal(flash_get_size(&atd), api_test_values.size, "Size mismatch");
	api_test_values.size = 46;
	zassert_equal(flash_get_size(&atd), api_test_values.size, "Size mismatch");
}

void test_main(void)
{
	ztest_test_suite(flash_api,
			 ztest_unit_test(test_get_page_info),
			 ztest_unit_test(test_get_page_count),
			 ztest_unit_test(test_get_size)
			);

	ztest_run_test_suite(flash_api);
}
