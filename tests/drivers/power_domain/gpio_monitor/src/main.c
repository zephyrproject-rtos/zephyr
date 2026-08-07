/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/irq_offload.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/ztest.h>

#define TEST_DOMAIN DT_NODELABEL(test_domain)
#define TEST_CHILD  DT_NODELABEL(test_child)

static const struct gpio_dt_spec monitor_gpio = GPIO_DT_SPEC_GET(TEST_DOMAIN, gpios);
static const struct device *test_child;
static atomic_t block_suspend;
static atomic_t callback_in_isr;
static atomic_t callback_before_edge_return;
static atomic_t edge_callback_returned;
static atomic_t track_edge_callback;
static atomic_t turn_off_count;
static atomic_t turn_on_count;
K_SEM_DEFINE(suspend_entered, 0, 1);
K_SEM_DEFINE(suspend_continue, 0, 1);
K_SEM_DEFINE(turn_on_done, 0, 1);

struct edge_context {
	int value;
	int ret;
};

static int child_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	if (k_is_in_isr()) {
		atomic_set(&callback_in_isr, 1);
	}
	if (atomic_get(&track_edge_callback) && !atomic_get(&edge_callback_returned)) {
		atomic_set(&callback_before_edge_return, 1);
	}

	if ((action == PM_DEVICE_ACTION_SUSPEND) && atomic_get(&block_suspend)) {
		k_sem_give(&suspend_entered);
		(void)k_sem_take(&suspend_continue, K_FOREVER);
	} else if (action == PM_DEVICE_ACTION_TURN_OFF) {
		atomic_inc(&turn_off_count);
	} else if (action == PM_DEVICE_ACTION_TURN_ON) {
		atomic_inc(&turn_on_count);
		k_sem_give(&turn_on_done);
	} else {
		/* No test synchronization is needed for RESUME or an unblocked SUSPEND. */
	}

	return 0;
}

static void set_monitor_input_from_isr(const void *arg)
{
	struct edge_context *context = (struct edge_context *)arg;

	context->ret = gpio_emul_input_set(monitor_gpio.port, monitor_gpio.pin, context->value);
	atomic_set(&edge_callback_returned, 1);
}

static void inject_monitor_edge(struct edge_context *edge)
{
	atomic_clear(&edge_callback_returned);
	atomic_set(&track_edge_callback, 1);
	irq_offload(set_monitor_input_from_isr, edge);
	zassert_ok(edge->ret);
}

PM_DEVICE_DT_DEFINE(TEST_CHILD, child_pm_action);
DEVICE_DT_DEFINE(TEST_CHILD, NULL, PM_DEVICE_DT_GET(TEST_CHILD), NULL, NULL, POST_KERNEL, 80, NULL);

static void *gpio_monitor_setup(void)
{
	enum pm_device_state state;

	test_child = DEVICE_DT_GET(TEST_CHILD);

	zassert_true(device_is_ready(monitor_gpio.port));
	zassert_true(device_is_ready(DEVICE_DT_GET(TEST_DOMAIN)));
	zassert_true(device_is_ready(test_child));

	if (IS_ENABLED(CONFIG_POWER_DOMAIN_GPIO_MONITOR_INITIAL_READ)) {
		zassert_true(WAIT_FOR((pm_device_state_get(test_child, &state) == 0) &&
					      (state == PM_DEVICE_STATE_OFF),
				      100000, k_msleep(1)));
		zassert_equal(atomic_get(&turn_off_count), 1);
		zassert_equal(atomic_get(&callback_in_isr), 0);
		atomic_set(&turn_off_count, 0);
	} else {
		pm_device_init_suspended(test_child);
	}
	zassert_ok(pm_device_runtime_enable(test_child));

	return NULL;
}

ZTEST(gpio_monitor, test_isr_deferral_and_resubmission)
{
	struct edge_context edge = {.value = 1};
	enum pm_device_state state;
	int ret = -EAGAIN;

	inject_monitor_edge(&edge);
	zassert_true(
		WAIT_FOR(((ret = pm_device_runtime_get(test_child)) == 0), 100000, k_msleep(1)),
		"child did not become available: %d", ret);
	if (IS_ENABLED(CONFIG_POWER_DOMAIN_GPIO_MONITOR_INITIAL_READ)) {
		zassert_ok(k_sem_take(&turn_on_done, K_MSEC(100)));
		zassert_equal(atomic_get(&turn_on_count), 1);
		atomic_set(&turn_on_count, 0);
	}

	atomic_set(&block_suspend, 1);
	edge.value = 0;
	inject_monitor_edge(&edge);
	zassert_ok(k_sem_take(&suspend_entered, K_MSEC(100)));
	zassert_equal(atomic_get(&callback_in_isr), 0);

	edge.value = 1;
	inject_monitor_edge(&edge);
	k_sem_give(&suspend_continue);
	zassert_ok(k_sem_take(&turn_on_done, K_MSEC(100)));

	atomic_clear(&track_edge_callback);
	zassert_equal(atomic_get(&callback_in_isr), 0);
	zassert_equal(atomic_get(&callback_before_edge_return), 0);
	zassert_equal(atomic_get(&turn_off_count), 1, "turn off count: %ld",
		      atomic_get(&turn_off_count));
	zassert_equal(atomic_get(&turn_on_count), 1, "turn on count: %ld",
		      atomic_get(&turn_on_count));
	zassert_ok(pm_device_state_get(test_child, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(test_child), 0);
}

ZTEST_SUITE(gpio_monitor, NULL, gpio_monitor_setup, NULL, NULL, NULL);
