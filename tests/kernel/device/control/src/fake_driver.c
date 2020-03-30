/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fake_api.h"

static K_THREAD_STACK_DEFINE(fake_irq_stack, 512 + CONFIG_TEST_EXTRA_STACKSIZE);
static struct k_thread fake_irq_thread;

static K_SEM_DEFINE(fake_trigger, 0, 1);

static struct device *fake_dev;
static k_timeout_t timeout;
static int value;
static bool lock_test;
static int *ret_val;

static void fake_irq(void)
{
	while (1) {
		k_sem_take(&fake_trigger, K_FOREVER);

		k_sleep(timeout);

		if (lock_test) {
			value++;
			*ret_val = value;
			lock_test = false;
		}

		device_call_complete(fake_dev, 0);
	}
}

static int fake_sync_interrupt_call(struct device *dev)
{
	if (device_lock(dev) != 0) {
		return -EAGAIN;
	}

	timeout = K_MSEC(10);

	k_sem_give(&fake_trigger);

	return device_release(dev);
}

static int fake_sync_polling_call(struct device *dev)
{
	if (device_lock(dev) != 0) {
		return -EAGAIN;
	}

	device_call_complete(dev, 0);

	return device_release(dev);
}

static int fake_lock_interrupt_call(struct device *dev, int *val)
{
	if (device_lock(dev) != 0) {
		return -EAGAIN;
	}

	ret_val = val;
	lock_test = true;
	timeout = K_MSEC(100);

	k_sem_give(&fake_trigger);

	return device_release(dev);
}

static int fake_lock_polling_call(struct device *dev, int *val)
{
	if (device_lock(dev) != 0) {
		return -EAGAIN;
	}

	value++;
	*val = value;
	k_sleep(K_MSEC(100));

	device_call_complete(dev, 0);

	return device_release(dev);
}

static int fake_no_to_call(struct device *dev, int *val)
{
	if (device_lock_timeout(dev, K_NO_WAIT) != 0) {
		return -EAGAIN;
	}

	value++;
	*val = value;
	k_sleep(K_MSEC(100));

	device_call_complete(dev, 0);

	return device_release(dev);
}

static int fake_driver_init(struct device *dev)
{
	fake_dev = dev;

	value = 0;
	lock_test = false;

	k_thread_create(&fake_irq_thread, fake_irq_stack,
			K_THREAD_STACK_SIZEOF(fake_irq_stack),
			(k_thread_entry_t)fake_irq,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	return 0;
}

static const struct fake_api api = {
	.sync_int_call = fake_sync_interrupt_call,
	.sync_poll_call = fake_sync_polling_call,
	.lock_int_call = fake_lock_interrupt_call,
	.lock_poll_call = fake_lock_polling_call,
	.no_to_call = fake_no_to_call,
};


DEVICE_AND_API_INIT(fake_driver, FAKE_DRV_NAME, fake_driver_init,
		    NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
