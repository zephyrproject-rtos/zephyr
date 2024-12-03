/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>

/** Test instrumentation **/
/*
 * The structure below gathers variables that may be used by the simulated API calls
 * to mock behavior of a flash device; the variables may be used however it is desired,
 * because it is up to the mocked functions and checks afterwards to relate these variables
 * to results.
 */
static struct {
	/* Set to value returned from any API call */
	int ret;
	/* Some size */
	uint64_t size;
} simulated_values = {
	.ret = 0,
	.size = 0,
};

/*** Device definition atd == API Test Dev ***/
static int some_get_size(const struct device *dev, uint64_t *size)
{
	__ASSERT_NO_MSG(dev != NULL);

	*size = simulated_values.size;

	return 0;
}

static int enotsup_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

/** Device objects **/
/* The device state, just to make it "ready" device */
static struct device_state some_dev_state = {
	.init_res = 0,
	.initialized = 1,
};

/* Device with get_size */
static DEVICE_API(flash, size_fun_api) = {
	.get_size = some_get_size,
};
const static struct device size_fun_dev = {
	"get_size",
	NULL,
	&size_fun_api,
	&some_dev_state,
};

/* No functions device */
static DEVICE_API(flash, no_fun_api) = {0};
const static struct device no_fun_dev = {
	"no_fun",
	NULL,
	&no_fun_api,
	&some_dev_state,
};

/* Device with get_size implemented but returning -ENOTSUP */
static DEVICE_API(flash, enotsup_fun_api) = {
	.get_size = enotsup_get_size,
};
static struct device enotsup_fun_dev = {
	"enotsup",
	NULL,
	&enotsup_fun_api,
	&some_dev_state,
};

ZTEST(flash_api, test_get_size)
{
	uint64_t size = 0;

	simulated_values.size = 45;
	zassert_ok(flash_get_size(&size_fun_dev, &size), "Expected success");
	zassert_equal(size, simulated_values.size, "Size mismatch");
	simulated_values.size = 46;
	zassert_ok(flash_get_size(&size_fun_dev, &size), "Expected success");
	zassert_equal(size, simulated_values.size, "Size mismatch");
	zassert_equal(flash_get_size(&no_fun_dev, &size), -ENOSYS);

	zassert_equal(flash_get_size(&enotsup_fun_dev, &size), -ENOTSUP);
}

ZTEST_SUITE(flash_api, NULL, NULL, NULL, NULL, NULL);
