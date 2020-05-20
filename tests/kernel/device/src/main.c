/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <ztest.h>
#include <sys/printk.h>


#define DUMMY_PORT_1    "dummy"
#define DUMMY_PORT_2    "dummy_driver"
#define BAD_DRIVER	"bad_driver"

/**
 * @brief Test cases to verify device objects
 *
 * Verify zephyr device driver apis with different device types
 *
 * @defgroup kernel_device_tests Device
 *
 * @ingroup all_tests
 *
 * @{
 */

/**
 * @brief Test device object binding
 *
 * Validates device binding for an existing and a non-existing device object.
 * It creates a dummy_driver device object with basic init and configuration
 * information and validates its binding.
 *
 * @see device_get_binding(), device_busy_set(), device_busy_clear(),
 * DEVICE_AND_API_INIT()
 */
void test_dummy_device(void)
{
	struct device *dev;

	dev = device_get_binding(DUMMY_PORT_1);
	zassert_equal(dev, NULL, NULL);
	dev = device_get_binding(DUMMY_PORT_2);
	zassert_false((dev == NULL), NULL);

	device_busy_set(dev);
	device_busy_clear(dev);

	/* device_get_binding() returns false for device object
	 * with failed init.
	 */
	dev = device_get_binding(BAD_DRIVER);
	zassert_true((dev == NULL), NULL);
}

/**
 * @brief Test device binding for existing device
 *
 * Validates device binding for an existing device object.
 *
 * @see device_get_binding(), DEVICE_AND_API_INIT()
 */
static void test_dynamic_name(void)
{
	struct device *mux;
	char name[sizeof(DUMMY_PORT_2)];

	snprintk(name, sizeof(name), "%s", DUMMY_PORT_2);
	mux = device_get_binding(name);
	zassert_true(mux != NULL, NULL);
}

/**
 * @brief Test device binding for non-existing device
 *
 * Validates binding of a random device driver(non-defined driver) named
 * "ANOTHER_BOGUS_NAME".
 *
 * @see device_get_binding(), DEVICE_AND_API_INIT()
 */
static void test_bogus_dynamic_name(void)
{
	struct device *mux;
	char name[64];

	snprintk(name, sizeof(name), "ANOTHER_BOGUS_NAME");
	mux = device_get_binding(name);
	zassert_true(mux == NULL, NULL);
}

static struct init_record {
	bool pre_kernel;
	bool is_in_isr;
	bool is_pre_kernel;
} init_records[4];

static struct init_record *rp = init_records;

static int add_init_record(bool pre_kernel)
{
	rp->pre_kernel = pre_kernel;
	rp->is_pre_kernel = k_is_pre_kernel();
	rp->is_in_isr = k_is_in_isr();
	++rp;
	return 0;
}

static int pre1_fn(struct device *dev)
{
	return add_init_record(true);
}

static int pre2_fn(struct device *dev)
{
	return add_init_record(true);
}

static int post_fn(struct device *dev)
{
	return add_init_record(false);
}

static int app_fn(struct device *dev)
{
	return add_init_record(false);
}

SYS_INIT(pre1_fn, PRE_KERNEL_1, 0);
SYS_INIT(pre2_fn, PRE_KERNEL_2, 0);
SYS_INIT(post_fn, POST_KERNEL, 0);
SYS_INIT(app_fn, APPLICATION, 0);

/**
 * @brief Test detection of initialization before kernel services available.
 *
 * Confirms check is correct.
 *
 * @see k_is_pre_kernel()
 */
void test_pre_kernel_detection(void)
{
	struct init_record *rpe = rp;

	zassert_equal(rp - init_records, 4U,
		      "bad record count");
	rp = init_records;
	while ((rp < rpe) && rp->pre_kernel) {
		zassert_equal(rp->is_in_isr, false,
			      "rec %zu isr", rp - init_records);
		zassert_equal(rp->is_pre_kernel, true,
			      "rec %zu pre-kernel", rp - init_records);
		++rp;
	}
	zassert_equal(rp - init_records, 2U,
		      "bad pre-kernel count");

	while (rp < rpe) {
		zassert_equal(rp->is_in_isr, false,
			      "rec %zu isr", rp - init_records);
		zassert_equal(rp->is_pre_kernel, false,
			      "rec %zu post-kernel", rp - init_records);
		++rp;
	}
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/**
 * @brief Test system device list query API with PM enabled.
 *
 * It queries the list of devices in the system, used to suspend or
 * resume the devices in PM applications.
 *
 * @see device_list_get()
 */
static void test_build_suspend_device_list(void)
{
	int devcount;
	struct device *devices;

	device_list_get(&devices, &devcount);
	zassert_false((devcount == 0), NULL);
}

/**
 * @brief Test device binding for existing device with PM enabled.
 *
 * Validates device binding for an existing device object with Power management
 * enabled. It also checks if the device is in the middle of a transaction,
 * sets/clears busy status and validates status again.
 *
 * @see device_get_binding(), device_busy_set(), device_busy_clear(),
 * device_busy_check(), device_any_busy_check(),
 * device_list_get(), device_set_power_state()
 */
void test_dummy_device_pm(void)
{
	struct device *dev;
	int busy, ret;

	dev = device_get_binding(DUMMY_PORT_2);
	zassert_false((dev == NULL), NULL);

	busy = device_any_busy_check();
	zassert_true((busy == 0), NULL);

	/* Set device state to DEVICE_PM_ACTIVE_STATE */
	ret = device_set_power_state(dev, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
	if (ret == -ENOTSUP) {
		zassert_true((ret == -ENOTSUP),
			     "Power management not supported on device");
		return;
	}
	zassert_true((ret == 0), "Unable to set active state to device");

	device_busy_set(dev);

	busy = device_any_busy_check();
	zassert_false((busy == 0), NULL);

	busy = device_busy_check(dev);
	zassert_false((busy == 0), NULL);

	device_busy_clear(dev);

	busy = device_busy_check(dev);
	zassert_true((busy == 0), NULL);

	/* Set device state to DEVICE_PM_FORCE_SUSPEND_STATE */
	ret = device_set_power_state(dev,
			DEVICE_PM_FORCE_SUSPEND_STATE, NULL, NULL);
	zassert_true((ret == 0), "Unable to force suspend device");

	test_build_suspend_device_list();
}
#else
static void test_build_suspend_device_list(void)
{
	ztest_test_skip();
}

void test_dummy_device_pm(void)
{
	ztest_test_skip();
}
#endif

/**
 * @}
 */

void test_main(void)
{
	ztest_test_suite(device,
			 ztest_unit_test(test_dummy_device_pm),
			 ztest_unit_test(test_build_suspend_device_list),
			 ztest_unit_test(test_dummy_device),
			 ztest_unit_test(test_pre_kernel_detection),
			 ztest_user_unit_test(test_bogus_dynamic_name),
			 ztest_user_unit_test(test_dynamic_name));
	ztest_run_test_suite(device);
}
