/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define FAKE_FUEL_GAUGE_BUF_SIZE 32U
#define FAKE_FUEL_GAUGE_PROP     ((fuel_gauge_prop_t)FUEL_GAUGE_CUSTOM_BEGIN)

struct fake_fuel_gauge_data {
	/* Arguments of the last set_buffer_property call */
	fuel_gauge_prop_t prop_type;
	const void *src;
	size_t src_len;
	uint8_t buf[FAKE_FUEL_GAUGE_BUF_SIZE];
	unsigned int calls;
	/* Value returned by set_buffer_property when non-zero */
	int ret;
};

/* Placed in the ztest memory partition so user mode tests can inspect and configure the fake */
static ZTEST_DMEM struct fake_fuel_gauge_data fake_data;

/* Kernel memory a user mode thread must not be able to pass as source buffer */
static uint8_t kernel_only_buf[FAKE_FUEL_GAUGE_BUF_SIZE];

static int fake_fuel_gauge_set_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop_type,
					   const void *src, size_t src_len)
{
	struct fake_fuel_gauge_data *data = dev->data;

	data->calls++;
	data->prop_type = prop_type;
	data->src = src;
	data->src_len = src_len;

	if (data->ret != 0) {
		return data->ret;
	}

	if (src_len > sizeof(data->buf)) {
		return -EINVAL;
	}

	memcpy(data->buf, src, src_len);

	return 0;
}

static int fake_fuel_gauge_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(fuel_gauge, fake_fuel_gauge_api) = {
	.set_buffer_property = fake_fuel_gauge_set_buffer_prop,
};

DEVICE_DEFINE(fake_fuel_gauge, "fake_fuel_gauge", fake_fuel_gauge_init, NULL, &fake_data,
	      NULL, POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY, &fake_fuel_gauge_api);

struct fuel_gauge_api_fixture {
	const struct device *dev;
};

static void *fuel_gauge_api_setup(void)
{
	static ZTEST_DMEM struct fuel_gauge_api_fixture fixture = {
		.dev = DEVICE_GET(fake_fuel_gauge),
	};

	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fake fuel gauge not ready");

	return &fixture;
}

static void fuel_gauge_api_before(void *f)
{
	ARG_UNUSED(f);

	memset(&fake_data, 0, sizeof(fake_data));
}

ZTEST_USER_F(fuel_gauge_api, test_set_buffer_prop__forwards_arguments)
{
	const uint8_t src[] = {0x01, 0x02, 0x03, 0x04, 0x05};

	zassert_ok(
		fuel_gauge_set_buffer_prop(fixture->dev, FAKE_FUEL_GAUGE_PROP, src, sizeof(src)));

	zassert_equal(fake_data.calls, 1U);
	zassert_equal(fake_data.prop_type, FAKE_FUEL_GAUGE_PROP);
	zassert_equal_ptr(fake_data.src, src);
	zassert_equal(fake_data.src_len, sizeof(src));
	zassert_mem_equal(fake_data.buf, src, sizeof(src));
}

ZTEST_USER_F(fuel_gauge_api, test_set_buffer_prop__propagates_driver_error)
{
	const uint8_t src[] = {0xAA};

	fake_data.ret = -EIO;

	zassert_equal(
		fuel_gauge_set_buffer_prop(fixture->dev, FAKE_FUEL_GAUGE_PROP, src, sizeof(src)),
		-EIO);
	zassert_equal(fake_data.calls, 1U);
}

ZTEST_USER_F(fuel_gauge_api, test_set_buffer_prop__rejects_unreadable_src)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_USERSPACE);

	ztest_set_fault_valid(true);
	(void)fuel_gauge_set_buffer_prop(fixture->dev, FAKE_FUEL_GAUGE_PROP, kernel_only_buf,
					 sizeof(kernel_only_buf));

	zassert_unreachable("kernel memory was accepted as source buffer");
}

ZTEST_SUITE(fuel_gauge_api, NULL, fuel_gauge_api_setup, fuel_gauge_api_before, NULL, NULL);
