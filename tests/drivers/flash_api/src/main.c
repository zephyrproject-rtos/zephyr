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
	/* Device mmap test values */
	void *mmap_base;
	ssize_t mmap_size;
	uint32_t mmap_flags;
} simulated_values = {
	.ret = 0,
	.size = 0,
};

/*** Device definition of API pseudo functions **/
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

static int some_mmap(const struct device *dev, void **base, uint64_t *size,
		     uint32_t flags)
{
	__ASSERT_NO_MSG(dev != NULL);

	if (flags != simulated_values.mmap_flags) {
		return -EINVAL;
	}

	if (base == NULL || size == NULL) {
		return -EINVAL;
	}

	*base = simulated_values.mmap_base;
	*size = simulated_values.mmap_size;

	return 0;
}

static int enotsup_mmap(const struct device *dev, void **base, uint64_t *size,
			uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(base);
	ARG_UNUSED(size);
	ARG_UNUSED(flags);

	return 0;
}

/** Device objects **/
/* The device state, just to make it "ready" device */
static struct device_state some_dev_state = {
	.init_res = 0,
	.initialized = 1,
};

/* Device with implemented api calls */
const static struct flash_driver_api some_fun_api = {
	.get_size = some_get_size,
	.mmap = some_mmap,
};

const static struct device some_fun_dev = {
	"some_fun",
	NULL,
	&some_fun_api,
	&some_dev_state,
};

/* No functions device */
const static struct flash_driver_api no_fun_api = {0};
const static struct device no_fun_dev = {
	"no_fun",
	NULL,
	&no_fun_api,
	&some_dev_state,
};

/* Device with get_size implemented but returning -ENOTSUP */
static struct flash_driver_api enotsup_fun_api = {
	.get_size = enotsup_get_size,
	.mmap = enotsup_mmap,
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
	zassert_ok(flash_get_size(&some_fun_dev, &size), "Expected success");
	zassert_equal(size, simulated_values.size, "Size mismatch");
	simulated_values.size = 46;
	zassert_ok(flash_get_size(&some_fun_dev, &size), "Expected success");
	zassert_equal(size, simulated_values.size, "Size mismatch");
	zassert_equal(flash_get_size(&no_fun_dev, &size), -ENOSYS);

	zassert_equal(flash_get_size(&enotsup_fun_dev, &size), -ENOTSUP);
}


ZTEST(flash_api, test_flash_mmap)
{
	void *base = NULL;
	uint64_t size = 0;

	simulated_values.mmap_size = 40;
	/* Just need some pointer */
	simulated_values.mmap_base = (void *)&simulated_values;
	/* 0 for error tests */
	simulated_values.mmap_flags = 0;

	zassert_equal(flash_mmap(&some_fun_dev, NULL, NULL, 0), -EINVAL);
	zassert_equal(flash_mmap(&some_fun_dev, &base, NULL, 0), -EINVAL);
	zassert_equal(flash_mmap(&some_fun_dev, NULL, &size, 0), -EINVAL);
	zassert_equal(flash_mmap(&some_fun_dev, &base, &size, FLASH_MMAP_F_READ), -EINVAL);
	zassert_equal(flash_mmap(&some_fun_dev, &base, &size, FLASH_MMAP_F_WRITE), -EINVAL);
	zassert_equal(flash_mmap(&some_fun_dev, &base, &size,
		      FLASH_MMAP_F_READ | FLASH_MMAP_F_WRITE), -EINVAL);

	simulated_values.mmap_flags = FLASH_MMAP_F_READ;
	zassert_equal(flash_mmap(&some_fun_dev, &base, &size, FLASH_MMAP_F_WRITE), -EINVAL);

	simulated_values.mmap_flags = FLASH_MMAP_F_WRITE;
	zassert_equal(flash_mmap(&some_fun_dev, &base, &size, FLASH_MMAP_F_READ), -EINVAL);

	zassert_equal(flash_get_size(&enotsup_fun_dev, &size), -ENOTSUP);

	/* After all failures the base and size are expected to be not modified */
	zassert_equal(base, NULL);
	zassert_equal(size, 0);

	simulated_values.mmap_flags = FLASH_MMAP_F_READ;
	zassert_ok(flash_mmap(&some_fun_dev, &base, &size, FLASH_MMAP_F_READ));
	/* Expected values to be read by API and updated */
	zassert_equal(base, simulated_values.mmap_base);
	zassert_equal(size, simulated_values.mmap_size);
}

ZTEST_SUITE(flash_api, NULL, NULL, NULL, NULL, NULL);
