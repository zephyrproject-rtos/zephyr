/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "udc_common.h"

struct fake_udc {
	struct udc_data data;
	unsigned int disable_calls;
	unsigned int shutdown_calls;
	int disable_error;
	int shutdown_error;
};

static struct fake_udc fixture;

static int fake_success(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int fake_disable(const struct device *dev)
{
	struct fake_udc *fake = udc_get_private(dev);

	fake->disable_calls++;
	return fake->disable_error;
}

static int fake_shutdown(const struct device *dev)
{
	struct fake_udc *fake = udc_get_private(dev);

	fake->shutdown_calls++;
	return fake->shutdown_error;
}

static void fake_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void fake_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api fake_api = {
	.init = fake_success,
	.enable = fake_success,
	.disable = fake_disable,
	.shutdown = fake_shutdown,
	.lock = fake_lock,
	.unlock = fake_unlock,
};

DEVICE_DEFINE(fake_udc, "fake_udc", NULL, NULL, &fixture.data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_api);

static const struct device *const udc = DEVICE_GET(fake_udc);

static int event_cb(const struct device *dev, const struct udc_event *const event)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(event);
	return 0;
}

static void reset_fixture(void *unused)
{
	ARG_UNUSED(unused);
	fixture = (struct fake_udc){0};
	fixture.data.priv = &fixture;
	k_mutex_init(&fixture.data.mutex);
	zassert_ok(udc_init(udc, event_cb, &fixture));
}

ZTEST(udc_common, test_retry_failed_disable)
{
	zassert_ok(udc_enable(udc));
	fixture.disable_error = -ENOMEM;
	zassert_equal(udc_disable(udc), -ENOMEM);
	zassert_true(udc_is_enabled(udc), "failed disable cleared enabled state");
	zassert_equal(udc_enable(udc), -EALREADY);
	zassert_equal(udc_shutdown(udc), -EBUSY);
	zassert_equal(fixture.shutdown_calls, 0);

	fixture.disable_error = 0;
	zassert_ok(udc_disable(udc));
	zassert_false(udc_is_enabled(udc));
	zassert_true(udc_is_initialized(udc));
	zassert_equal(udc_disable(udc), -EALREADY);
	zassert_equal(fixture.disable_calls, 2);
}

ZTEST(udc_common, test_retry_failed_shutdown)
{
	fixture.shutdown_error = -EIO;
	zassert_equal(udc_shutdown(udc), -EIO);
	zassert_true(udc_is_initialized(udc), "failed shutdown cleared initialized state");
	zassert_equal(udc_init(udc, event_cb, &fixture), -EALREADY);
	zassert_equal(udc_purge_queues(udc), -EBUSY);

	fixture.shutdown_error = 0;
	zassert_ok(udc_shutdown(udc));
	zassert_false(udc_is_initialized(udc));
	zassert_equal(udc_shutdown(udc), -EALREADY);
	zassert_equal(fixture.shutdown_calls, 2);
	zassert_ok(udc_purge_queues(udc));
}

ZTEST_SUITE(udc_common, NULL, NULL, reset_fixture, NULL, NULL);
