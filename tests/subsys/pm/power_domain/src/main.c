/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define NUMBER_OF_DEVICES 3

#define TEST_DOMAIN DT_NODELABEL(test_domain)
#define TEST_DEVA DT_NODELABEL(test_dev_a)
#define TEST_DEVB DT_NODELABEL(test_dev_b)

static const struct device *const domain = DEVICE_DT_GET(TEST_DOMAIN);
static const struct device *const deva = DEVICE_DT_GET(TEST_DEVA);
static const struct device *const devb = DEVICE_DT_GET(TEST_DEVB);
static const struct device *devc;
static int testing_domain_on_notitication;
static int testing_domain_off_notitication;

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
DEVICE_DT_DEFINE(TEST_DOMAIN, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVA, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEVA, NULL, PM_DEVICE_DT_GET(TEST_DEVA),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVB, devb_pm_action);
DEVICE_DT_DEFINE(TEST_DEVB, NULL, PM_DEVICE_DT_GET(TEST_DEVB),
		 NULL, NULL, POST_KERNEL, 30, NULL);

PM_DEVICE_DEFINE(devc, deva_pm_action);
DEVICE_DEFINE(devc, "devc", NULL, PM_DEVICE_GET(devc),
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
ZTEST(power_domain_1cpu, test_power_domain_device_runtime)
{
	int ret;
	enum pm_device_state state;

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
	zassert_equal(ret, -ENOENT);

	ret = pm_device_power_domain_add(devc, domain);
	zassert_equal(ret, 0);

	/* At this point all devices should be SUSPENDED */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(devc, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Now test if "get" a device will resume the domain */
	ret = pm_device_runtime_get(deva);
	zassert_equal(ret, 0);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	ret = pm_device_runtime_get(devc);
	zassert_equal(ret, 0);

	ret = pm_device_runtime_get(devb);
	zassert_equal(ret, 0);

	ret = pm_device_runtime_put(deva);
	zassert_equal(ret, 0);

	/*
	 * The domain has to still be active since device B
	 * is still in use.
	 */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	/*
	 * Now the domain should be suspended since there is no
	 * one using it.
	 */
	ret = pm_device_runtime_put(devb);
	zassert_equal(ret, 0);

	ret = pm_device_runtime_put(devc);
	zassert_equal(ret, 0);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/*
	 * With the domain suspended the device state should be OFF, since
	 * the power was completely cut.
	 */
	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF);

	/*
	 * Now lets test that devices are notified when the domain
	 * changes its state.
	 */

	/* Three devices has to get the notification */
	testing_domain_on_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_get(domain);
	zassert_equal(ret, 0);

	zassert_equal(testing_domain_on_notitication, 0);

	testing_domain_off_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_put(domain);
	zassert_equal(ret, 0);

	zassert_equal(testing_domain_off_notitication, 0);

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, 0);
}

#define TEST_DOMAIN_BALANCED DT_NODELABEL(test_domain_balanced)
#define TEST_DEV_BALANCED DT_NODELABEL(test_dev_balanced)

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_BALANCED, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_BALANCED, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_BALANCED),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_BALANCED, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_BALANCED, NULL, PM_DEVICE_DT_GET(TEST_DEV_BALANCED),
		 NULL, NULL, POST_KERNEL, 20, NULL);

/**
 * @brief Test power domain requests are balanced
 *
 * Scenarios tested:
 *
 * - get + put device with a PD while PM is disabled
 */
ZTEST(power_domain_1cpu, test_power_domain_device_balanced)
{
	const struct device *domain = DEVICE_DT_GET(TEST_DOMAIN_BALANCED);
	const struct device *dev = DEVICE_DT_GET(TEST_DEV_BALANCED);
	enum pm_device_state state;
	int ret;

	/* Init domain */
	pm_device_init_suspended(domain);
	pm_device_runtime_enable(domain);

	/* At this point domain should be SUSPENDED */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Get and put the device without PM enabled should not change the domain */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Same thing with the domain in active state */
	ret = pm_device_runtime_get(domain);
	zassert_equal(ret, 0);
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0);

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
}

ZTEST_SUITE(power_domain_1cpu, NULL, NULL, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);
