/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define NUMBER_OF_DEVICES 3

#define TEST_DOMAIN DT_NODELABEL(test_domain)
#define TEST_DEVA DT_NODELABEL(test_dev_a)
#define TEST_DEVB DT_NODELABEL(test_dev_b)

static const struct device *domain, *deva, *devb, *devc;
static int testing_domain_on_notitication;
static int testing_domain_off_notitication;

static int dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int domain_pm_action(const struct device *dev,
	enum pm_device_action action)
{
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Switch power on */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		__fallthrough;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;

}

static int deva_pm_action(const struct device *dev,
		     enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);

	if (testing_domain_on_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
			testing_domain_on_notitication--;
		}
	} else if (testing_domain_off_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
			testing_domain_off_notitication--;
		}
	}

	return 0;
}

/*
 * Device B will return -ENOTSUP for TURN_ON and TURN_OFF actions.
 * This way we can check if the subsystem properly handled its state.
 */
static int devb_pm_action(const struct device *dev,
		     enum pm_device_action pm_action)
{
	int ret = 0;

	ARG_UNUSED(dev);

	if (testing_domain_on_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
			ret = -ENOTSUP;
			testing_domain_on_notitication--;
		}
	} else if (testing_domain_off_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
			ret = -ENOTSUP;
			testing_domain_off_notitication--;
		}
	}

	return ret;
}


PM_DEVICE_DT_DEFINE(TEST_DOMAIN, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN, dev_init, PM_DEVICE_DT_GET(TEST_DOMAIN),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVA, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEVA, dev_init, PM_DEVICE_DT_GET(TEST_DEVA),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVB, devb_pm_action);
DEVICE_DT_DEFINE(TEST_DEVB, dev_init, PM_DEVICE_DT_GET(TEST_DEVB),
		 NULL, NULL, POST_KERNEL, 30, NULL);

PM_DEVICE_DEFINE(devc, deva_pm_action);
DEVICE_DEFINE(devc, "devc", dev_init, PM_DEVICE_GET(devc),
	      NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

/**
 * @brief Test the power domain behavior
 *
 * Scenarios tested:
 *
 * - get + put multiple devices under a domain
 * - notification when domain state changes
 */
static void test_power_domain_device_runtime(void)
{
	int ret;
	enum pm_device_state state;

	domain = DEVICE_DT_GET(TEST_DOMAIN);
	deva = DEVICE_DT_GET(TEST_DEVA);
	devb = DEVICE_DT_GET(TEST_DEVB);
	devc = DEVICE_GET(devc);

	pm_device_init_suspended(domain);
	pm_device_init_suspended(deva);
	pm_device_init_suspended(devb);
	pm_device_init_suspended(devc);

	pm_device_runtime_enable(domain);
	pm_device_runtime_enable(deva);
	pm_device_runtime_enable(devb);
	pm_device_runtime_enable(devc);

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, -ENOENT, NULL);

	ret = pm_device_power_domain_add(devc, domain);
	zassert_equal(ret, 0, NULL);

	/* At this point all devices should be SUSPENDED */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	pm_device_state_get(devc, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/* Now test if "get" a device will resume the domain */
	ret = pm_device_runtime_get(deva);
	zassert_equal(ret, 0, NULL);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	ret = pm_device_runtime_get(devc);
	zassert_equal(ret, 0, NULL);

	ret = pm_device_runtime_get(devb);
	zassert_equal(ret, 0, NULL);

	ret = pm_device_runtime_put(deva);
	zassert_equal(ret, 0, NULL);

	/*
	 * The domain has to still be active since device B
	 * is still in use.
	 */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	/*
	 * Now the domain should be suspended since there is no
	 * one using it.
	 */
	ret = pm_device_runtime_put(devb);
	zassert_equal(ret, 0, NULL);

	ret = pm_device_runtime_put(devc);
	zassert_equal(ret, 0, NULL);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/*
	 * With the domain suspended the device state should be OFF, since
	 * the power was completely cut.
	 */
	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF, NULL);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF, NULL);

	/*
	 * Now lets test that devices are notified when the domain
	 * changes its state.
	 */

	/* Three devices has to get the notification */
	testing_domain_on_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_get(domain);
	zassert_equal(ret, 0, NULL);

	zassert_equal(testing_domain_on_notitication, 0, NULL);

	testing_domain_off_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_put(domain);
	zassert_equal(ret, 0, NULL);

	zassert_equal(testing_domain_off_notitication, 0, NULL);

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, 0, NULL);
}

void test_main(void)
{
	ztest_test_suite(power_domain_test,
			 ztest_1cpu_unit_test(test_power_domain_device_runtime));

	ztest_run_test_suite(power_domain_test);
}
