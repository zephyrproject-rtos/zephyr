/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/irq_offload.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device_runtime_internal.h>
#include <zephyr/ztest.h>

struct test_pm_data {
	uint32_t action_count[PM_DEVICE_ACTION_TURN_ON + 1];
	int action_ret[PM_DEVICE_ACTION_TURN_ON + 1];
	enum pm_device_action action_log[32];
	size_t action_log_count;
	atomic_t block_suspend;
	atomic_t callback_concurrency;
	atomic_t callback_max_concurrency;
	int suspend_ret_once;
};

struct isr_action_context {
	const struct device *dev;
	int ret;
};

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
K_SEM_DEFINE(async_suspend_entered, 0, 1);
K_SEM_DEFINE(async_suspend_continue, 0, 1);
K_SEM_DEFINE(power_domain_action_done, 0, 1);
K_THREAD_STACK_DEFINE(power_domain_action_stack, 1024);
static struct k_thread power_domain_action_thread;
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

static int test_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct test_pm_data *data = dev->data;
	atomic_val_t current;
	atomic_val_t maximum;
	int ret;

	current = atomic_inc(&data->callback_concurrency) + 1;
	do {
		maximum = atomic_get(&data->callback_max_concurrency);
	} while ((current > maximum) &&
		 !atomic_cas(&data->callback_max_concurrency, maximum, current));

	data->action_count[action]++;
	zassert_true(data->action_log_count < ARRAY_SIZE(data->action_log));
	data->action_log[data->action_log_count++] = action;
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
	if ((action == PM_DEVICE_ACTION_SUSPEND) && atomic_get(&data->block_suspend)) {
		k_sem_give(&async_suspend_entered);
		(void)k_sem_take(&async_suspend_continue, K_FOREVER);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

	ret = data->action_ret[action];
	if ((action == PM_DEVICE_ACTION_SUSPEND) && (data->suspend_ret_once != 0)) {
		ret = data->suspend_ret_once;
		data->suspend_ret_once = 0;
	}
	atomic_dec(&data->callback_concurrency);

	return ret;
}

static void power_domain_action_from_isr(const void *arg)
{
	struct isr_action_context *context = (struct isr_action_context *)arg;

	context->ret = z_pm_device_runtime_power_domain_action_run(context->dev,
								   PM_DEVICE_ACTION_TURN_OFF);
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
static void power_domain_action_from_thread(void *context_ptr, void *arg2, void *arg3)
{
	struct isr_action_context *context = context_ptr;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	context->ret = z_pm_device_runtime_power_domain_action_run(context->dev,
								   PM_DEVICE_ACTION_TURN_OFF);
	k_sem_give(&power_domain_action_done);
}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

static void run_power_domain_action_test(const struct device *dev)
{
	struct test_pm_data *data = dev->data;
	struct isr_action_context context = {.dev = dev};
	uint32_t suspend_count;
	uint32_t turn_off_count;
	enum pm_device_state state;

	*data = (struct test_pm_data){0};
	pm_device_init_suspended(dev);
	zassert_ok(pm_device_runtime_enable(dev));

	irq_offload(power_domain_action_from_isr, &context);
	zassert_equal(context.ret, -EWOULDBLOCK);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(dev), 0);

	zassert_ok(pm_device_runtime_get(dev));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_RESUME], 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_SUSPEND], 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_OFF], 1);
	zassert_equal(data->action_log[0], PM_DEVICE_ACTION_RESUME);
	zassert_equal(data->action_log[1], PM_DEVICE_ACTION_SUSPEND);
	zassert_equal(data->action_log[2], PM_DEVICE_ACTION_TURN_OFF);
	zassert_equal(atomic_get(&data->callback_max_concurrency), 1);
	zassert_true(pm_device_runtime_is_enabled(dev));
	zassert_equal(pm_device_runtime_get(dev), -EAGAIN);

	suspend_count = data->action_count[PM_DEVICE_ACTION_SUSPEND];
	turn_off_count = data->action_count[PM_DEVICE_ACTION_TURN_OFF];
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(data->action_count[PM_DEVICE_ACTION_SUSPEND], suspend_count);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_OFF], turn_off_count);

	zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(dev));
	zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND));
	zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(pm_device_runtime_usage(dev), 1);
	ARRAY_FOR_EACH(data->action_count, i) {
		data->action_count[i] = 0;
	}
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_ON], 1);
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -EALREADY);

	zassert_ok(pm_device_runtime_get(dev));
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(pm_device_runtime_usage(dev), 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_RESUME], 1);
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -ENOTSUP);
	zassert_equal(pm_device_runtime_usage(dev), 1);

	zassert_ok(pm_device_runtime_put(dev));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(pm_device_runtime_put(dev), -EALREADY);
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_equal(pm_device_runtime_put(dev), -EALREADY);
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(pm_device_runtime_put(dev), -EALREADY);
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));

	zassert_ok(pm_device_runtime_get(dev));
	zassert_ok(pm_device_runtime_put(dev));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(pm_device_runtime_get(dev), -EAGAIN);
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));

	zassert_equal(pm_device_runtime_get(dev), 0);
	zassert_equal(pm_device_runtime_usage(dev), 1);
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(pm_device_runtime_usage(dev), 0);

	data->action_ret[PM_DEVICE_ACTION_TURN_ON] = -EIO;
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -EIO);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_true(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_TURN_ON_FAILED));

	data->action_ret[PM_DEVICE_ACTION_TURN_ON] = 0;
	data->action_ret[PM_DEVICE_ACTION_TURN_OFF] = -EFAULT;
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF),
		      -EFAULT);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_false(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_TURN_ON_FAILED));

	data->action_ret[PM_DEVICE_ACTION_TURN_OFF] = 0;
	data->action_ret[PM_DEVICE_ACTION_TURN_ON] = -ENOTSUP;
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -ENOTSUP);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_false(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_TURN_ON_FAILED));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));

	data->action_ret[PM_DEVICE_ACTION_TURN_ON] = 0;
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(dev));
	data->action_ret[PM_DEVICE_ACTION_SUSPEND] = -EIO;
	data->action_ret[PM_DEVICE_ACTION_TURN_OFF] = -EFAULT;
	atomic_set_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_TURN_ON_FAILED);
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF),
		      -EIO);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_false(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_TURN_ON_FAILED));

	data->action_ret[PM_DEVICE_ACTION_SUSPEND] = 0;
	data->action_ret[PM_DEVICE_ACTION_TURN_OFF] = 0;
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_disable(dev));
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
static void prepare_async_test(const struct device *dev)
{
	struct test_pm_data *data = dev->data;

	*data = (struct test_pm_data){0};
	k_sem_reset(&async_suspend_entered);
	k_sem_reset(&async_suspend_continue);
	k_sem_reset(&power_domain_action_done);
	pm_device_init_suspended(dev);
	zassert_ok(pm_device_runtime_enable(dev));
	zassert_ok(pm_device_runtime_get(dev));
}

static void finish_async_test(const struct device *dev)
{
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_disable(dev));
}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

PM_DEVICE_DEFINE(test_generic_pm, test_pm_action);
static struct test_pm_data test_generic_data;
DEVICE_DEFINE(test_generic, "test_generic", NULL, PM_DEVICE_GET(test_generic_pm),
	      &test_generic_data, NULL, POST_KERNEL, 80, NULL);

PM_DEVICE_DEFINE(test_isr_safe_pm, test_pm_action, PM_DEVICE_ISR_SAFE);
static struct test_pm_data test_isr_safe_data;
DEVICE_DEFINE(test_isr_safe, "test_isr_safe", NULL, PM_DEVICE_GET(test_isr_safe_pm),
	      &test_isr_safe_data, NULL, POST_KERNEL, 80, NULL);

DEVICE_DEFINE(test_unsupported, "test_unsupported", NULL, NULL, NULL, NULL, POST_KERNEL, 80, NULL);

ZTEST(device_runtime_power_domain_action, test_generic)
{
	run_power_domain_action_test(DEVICE_GET(test_generic));
}

ZTEST(device_runtime_power_domain_action, test_isr_safe)
{
	run_power_domain_action_test(DEVICE_GET(test_isr_safe));
}

ZTEST(device_runtime_power_domain_action, test_validation)
{
	const struct device *dev = DEVICE_GET(test_unsupported);
	const struct device *generic_dev = DEVICE_GET(test_generic);
	struct test_pm_data *data = generic_dev->data;
	uint32_t flags;
	enum pm_device_state state;

	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF),
		      -ENOTSUP);

	*data = (struct test_pm_data){0};
	pm_device_init_suspended(generic_dev);
	zassert_ok(pm_device_runtime_enable(generic_dev));
	flags = generic_dev->pm_base->flags;
	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(generic_dev, PM_DEVICE_ACTION_SUSPEND),
		-EINVAL);
	zassert_ok(pm_device_state_get(generic_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(generic_dev), 0);
	zassert_equal(generic_dev->pm_base->flags, flags);
	ARRAY_FOR_EACH(data->action_count, i) {
		zassert_equal(data->action_count[i], 0);
	}
	zassert_ok(pm_device_runtime_disable(generic_dev));
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
ZTEST(device_runtime_power_domain_action, test_async_queued_suspend)
{
	const struct device *dev = DEVICE_GET(test_generic);
	struct test_pm_data *data = dev->data;
	enum pm_device_state state;

	prepare_async_test(dev);
	zassert_ok(pm_device_runtime_put_async(dev, K_SECONDS(1)));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_SUSPEND], 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_OFF], 1);
	zassert_equal(atomic_get(&data->callback_max_concurrency), 1);
	finish_async_test(dev);
}

ZTEST(device_runtime_power_domain_action, test_turn_on_rejects_queued_suspend)
{
	const struct device *dev = DEVICE_GET(test_generic);
	struct test_pm_data *data = dev->data;
	uint32_t flags;
	enum pm_device_state state;

	prepare_async_test(dev);
	zassert_ok(pm_device_runtime_put_async(dev, K_SECONDS(1)));
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);
	flags = dev->pm_base->flags;
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -ENOTSUP);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(dev->pm_base->flags, flags);
	ARRAY_FOR_EACH(data->action_count, i) {
		zassert_equal(data->action_count[i], i == PM_DEVICE_ACTION_RESUME);
	}
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	finish_async_test(dev);
}

ZTEST(device_runtime_power_domain_action, test_turn_on_rejects_running_suspend)
{
	const struct device *dev = DEVICE_GET(test_generic);
	struct test_pm_data *data = dev->data;
	uint32_t flags;
	enum pm_device_state state;

	prepare_async_test(dev);
	atomic_set(&data->block_suspend, 1);
	zassert_ok(pm_device_runtime_put_async(dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&async_suspend_entered, K_MSEC(100)));
	flags = dev->pm_base->flags;
	zassert_equal(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON),
		      -ENOTSUP);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(dev->pm_base->flags, flags);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_RESUME], 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_SUSPEND], 1);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_OFF], 0);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_ON], 0);
	atomic_clear(&data->block_suspend);
	k_sem_give(&async_suspend_continue);
	zassert_true(WAIT_FOR((pm_device_state_get(dev, &state) == 0) &&
				      (state == PM_DEVICE_STATE_SUSPENDED),
			      100000, k_msleep(1)));
	zassert_ok(z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF));
	finish_async_test(dev);
}

static void run_async_running_suspend(int suspend_ret_once, uint32_t expected_suspend_count)
{
	const struct device *dev = DEVICE_GET(test_generic);
	struct test_pm_data *data = dev->data;
	struct isr_action_context context = {.dev = dev};
	enum pm_device_state state;
	k_tid_t thread;

	prepare_async_test(dev);
	atomic_set(&data->block_suspend, 1);
	data->suspend_ret_once = suspend_ret_once;
	zassert_ok(pm_device_runtime_put_async(dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&async_suspend_entered, K_MSEC(100)));

	thread = k_thread_create(&power_domain_action_thread, power_domain_action_stack,
				 K_THREAD_STACK_SIZEOF(power_domain_action_stack),
				 power_domain_action_from_thread, &context, NULL, NULL,
				 K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_equal(k_sem_take(&power_domain_action_done, K_MSEC(10)), -EAGAIN);
	atomic_clear(&data->block_suspend);
	k_sem_give(&async_suspend_continue);
	zassert_ok(k_sem_take(&power_domain_action_done, K_MSEC(100)));
	zassert_ok(k_thread_join(thread, K_MSEC(100)));
	zassert_ok(context.ret);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(dev), 0);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_SUSPEND], expected_suspend_count);
	zassert_equal(data->action_count[PM_DEVICE_ACTION_TURN_OFF], 1);
	zassert_equal(atomic_get(&data->callback_max_concurrency), 1);
	finish_async_test(dev);
}

ZTEST(device_runtime_power_domain_action, test_async_running_suspend_failure)
{
	run_async_running_suspend(-EIO, 2);
}

ZTEST(device_runtime_power_domain_action, test_async_running_suspend_success)
{
	run_async_running_suspend(0, 1);
}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

ZTEST_SUITE(device_runtime_power_domain_action, NULL, NULL, NULL, NULL, NULL);
