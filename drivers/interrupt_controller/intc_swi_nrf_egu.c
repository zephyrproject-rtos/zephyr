/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for nrf_irq_offload subsystem
 */

#include <init.h>
#include <kernel.h>
#include <drivers/interrupt_controller/intc_nrf_egu.h>
#include <drivers/interrupt_controller/intc_swi.h>

struct swi_data {
	const struct device *egu;
	int channel;
	sys_slist_t swi_channels;
	struct k_spinlock lock;
};

static struct swi_data data = {
	.swi_channels = SYS_SLIST_STATIC_INIT(&data.swi_channels)
};

static void egu_callback(const struct device *dev, int channel, void *ctx)
{
	sys_slist_t sig_list = SYS_SLIST_STATIC_INIT(&sig_list);
	struct swi_channel *swi;
	k_spinlock_key_t key;
	sys_snode_t *node;

	ARG_UNUSED(ctx);

	key = k_spin_lock(&data.lock);
	SYS_SLIST_FOR_EACH_CONTAINER(&data.swi_channels, swi, node) {
		if (atomic_cas(&swi->signaled, 1, 0)) {
			sys_slist_append(&sig_list, &swi->aux_node);
		}
	}
	k_spin_unlock(&data.lock, key);

	while ((node = sys_slist_get(&sig_list)) != NULL) {
		swi = CONTAINER_OF(node, struct swi_channel, aux_node);
		swi->cb(swi);
	}
}

int swi_channel_init(struct swi_channel *swi, swi_channel_cb_t cb)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (cb == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data.lock);
	if (swi->initialized == true) {
		ret = -EALREADY;
	} else {
		sys_slist_append(&data.swi_channels, &swi->node);
		atomic_clear(&swi->signaled);
		swi->cb = cb;
		swi->initialized = true;
	}
	k_spin_unlock(&data.lock, key);

	return ret;
}

int swi_channel_deinit(struct swi_channel *swi)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&data.lock);
	if (swi->initialized) {
		sys_slist_find_and_remove(&data.swi_channels, &swi->node);
		swi->initialized = false;
	}
	k_spin_unlock(&data.lock, key);

	return 0;
}

int swi_channel_trigger(struct swi_channel *swi)
{
	if (!atomic_cas(&swi->signaled, 0, 1)) {
		return -EALREADY;
	}
	return egu_channel_task_trigger(data.egu, data.channel);
}

static int intc_swi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *egu;
	int channel;
	int result;

	egu = DEVICE_DT_GET(DT_CHOSEN(nordic_swi_egu));
	if (!device_is_ready(egu)) {
		return -ENODEV;
	}

	channel = egu_channel_alloc(egu);
	if (channel < 0) {
		return -ENODEV;
	}

	result = egu_channel_callback_set(egu, channel, egu_callback, NULL);
	__ASSERT(result == 0, "unexpected EGU channel state");

	data.egu = egu;
	data.channel = channel;

	return 0;
}

SYS_INIT(intc_swi_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
