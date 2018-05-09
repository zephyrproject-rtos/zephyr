/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <ztest.h>
#include <misc/printk.h>


#define DUMMY_PORT_1    "dummy"
#define DUMMY_PORT_2    "dummy_driver"


void test_dummy_device(void)
{
	struct device *dev;

	dev = device_get_binding(DUMMY_PORT_1);
	zassert_equal(dev, NULL, NULL);
	dev = device_get_binding(DUMMY_PORT_2);
	zassert_false((dev == NULL), NULL);

	device_busy_set(dev);
	device_busy_clear(dev);

}

static void test_dynamic_name(void)
{
	struct device *mux;
	char name[sizeof(DUMMY_PORT_2)];

	snprintk(name, sizeof(name), "%s", DUMMY_PORT_2);
	mux = device_get_binding(name);
	zassert_true(mux != NULL, NULL);
}

static void test_bogus_dynamic_name(void)
{
	struct device *mux;
	char name[64];

	snprintk(name, sizeof(name), "ANOTHER_BOGUS_NAME");
	mux = device_get_binding(name);
	zassert_true(mux == NULL, NULL);
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void build_suspend_device_list(void)
{
	int devcount;
	struct device *devices;

	device_list_get(&devices, &devcount);
	zassert_false((devcount == 0), NULL);
}

void test_dummy_device_pm(void)
{
	struct device *dev;
	int busy;

	dev = device_get_binding(DUMMY_PORT_2);
	zassert_false((dev == NULL), NULL);

	busy = device_any_busy_check();
	zassert_true((busy == 0), NULL);

	device_busy_set(dev);

	busy = device_any_busy_check();
	zassert_false((busy == 0), NULL);

	busy = device_busy_check(dev);
	zassert_false((busy == 0), NULL);

	device_busy_clear(dev);

	busy = device_busy_check(dev);
	zassert_true((busy == 0), NULL);

	build_suspend_device_list();
}
#else
static void build_suspend_device_list(void)
{
	ztest_test_skip();
}

void test_dummy_device_pm(void)
{
	ztest_test_skip();
}
#endif


void test_main(void)
{
	ztest_test_suite(device,
			 ztest_unit_test(test_dummy_device_pm),
			 ztest_unit_test(build_suspend_device_list),
			 ztest_unit_test(test_dummy_device),
			 ztest_unit_test(test_bogus_dynamic_name),
			 ztest_unit_test(test_dynamic_name));
	ztest_run_test_suite(device);
}
