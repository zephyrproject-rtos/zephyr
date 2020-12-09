/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <devicetree.h>
#include <device.h>
#include <drivers/gpio.h>

#define GPIO DT_NODELABEL(test_gpio_0)
#define I2C DT_NODELABEL(test_i2c)
#define DEVA DT_NODELABEL(test_dev_a)
#define GPIOX DT_NODELABEL(test_gpiox)
#define DEVB DT_NODELABEL(test_dev_b)

static const struct device *devlist;
static const struct device *devlist_end;

static device_handle_t init_order[10];

static int dev_init(const struct device *dev)
{
	static uint8_t init_idx;

	__ASSERT_NO_MSG(init_idx < ARRAY_SIZE(init_order));
	init_order[init_idx++] = device_handle_get(dev);

	return 0;
}

DEVICE_DT_DEFINE(GPIO, dev_init, device_pm_control_nop,
		 NULL, NULL, PRE_KERNEL_1, 90, NULL);
DEVICE_DT_DEFINE(I2C, dev_init, device_pm_control_nop,
		 NULL, NULL, POST_KERNEL, 10, NULL);
DEVICE_DT_DEFINE(DEVA, dev_init, device_pm_control_nop,
		 NULL, NULL, POST_KERNEL, 20, NULL);
/* NB: Intentional init devb before required gpiox */
DEVICE_DT_DEFINE(DEVB, dev_init, device_pm_control_nop,
		 NULL, NULL, POST_KERNEL, 30, NULL);
DEVICE_DT_DEFINE(GPIOX, dev_init, device_pm_control_nop,
		 NULL, NULL, POST_KERNEL, 40, NULL);

#define DEV_HDL(node_id) device_handle_get(DEVICE_DT_GET(node_id))

static void test_init_order(void)
{
	zassert_equal(init_order[0], DEV_HDL(GPIO),
		      NULL);
	zassert_equal(init_order[1], DEV_HDL(I2C),
		      NULL);
	zassert_equal(init_order[2], DEV_HDL(DEVA),
		      NULL);
	zassert_equal(init_order[3], DEV_HDL(DEVB),
		      NULL);
	zassert_equal(init_order[4], DEV_HDL(GPIOX),
		      NULL);
}

static bool check_handle(device_handle_t hdl,
			 const device_handle_t *hdls,
			 size_t nhdls)
{
	const device_handle_t *hdle = hdls + nhdls;

	while (hdls < hdle) {
		if (*hdls == hdl) {
			return true;
		}
		++hdls;
	}

	return false;
}

static void test_requires(void)
{
	size_t nhdls = 0;
	const device_handle_t *hdls;
	const struct device *dev;

	/* GPIO: no req */
	dev = device_get_binding(DT_LABEL(GPIO));
	zassert_equal(dev, DEVICE_DT_GET(GPIO), NULL);
	hdls = device_get_requires_handles(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* I2C: no req */
	dev = device_get_binding(DT_LABEL(I2C));
	zassert_equal(dev, DEVICE_DT_GET(I2C), NULL);
	hdls = device_get_requires_handles(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* DEVA: I2C GPIO */
	dev = device_get_binding(DT_LABEL(DEVA));
	zassert_equal(dev, DEVICE_DT_GET(DEVA), NULL);
	hdls = device_get_requires_handles(dev, &nhdls);
	zassert_equal(nhdls, 2, NULL);
	zassert_true(check_handle(DEV_HDL(I2C), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(GPIO), hdls, nhdls), NULL);

	/* GPIOX: I2C */
	dev = device_get_binding(DT_LABEL(GPIOX));
	zassert_equal(dev, DEVICE_DT_GET(GPIOX), NULL);
	hdls = device_get_requires_handles(dev, &nhdls);
	zassert_equal(nhdls, 1, NULL);
	zassert_true(check_handle(DEV_HDL(I2C), hdls, nhdls), NULL);

	/* DEVB: I2C GPIOX */
	dev = device_get_binding(DT_LABEL(DEVB));
	zassert_equal(dev, DEVICE_DT_GET(DEVB), NULL);
	hdls = device_get_requires_handles(dev, &nhdls);
	zassert_equal(nhdls, 2, NULL);
	zassert_true(check_handle(DEV_HDL(I2C), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(GPIOX), hdls, nhdls), NULL);
}

void test_main(void)
{
	size_t ndevs;

	ndevs = z_device_get_all_static(&devlist);
	devlist_end = devlist + ndevs;

	ztest_test_suite(devicetree_driver,
			 ztest_unit_test(test_init_order),
			 ztest_unit_test(test_requires)
		);
	ztest_run_test_suite(devicetree_driver);
}
