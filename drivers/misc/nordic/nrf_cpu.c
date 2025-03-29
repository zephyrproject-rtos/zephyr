/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/event_device.h>

static atomic_t block_cpu_idle;

struct shim_config {
	const struct pm_event_device *event_device;
};

static void shim_request_latency(const struct device *dev, uint8_t event_state)
{
	ARG_UNUSED(dev);

	atomic_set(&block_cpu_idle, event_state);
}

static int shim_init(const struct device *dev)
{
	const struct shim_config *config = dev->config;

	pm_event_device_init(config->event_device);
	return 0;
}

void idle_enter_hook(void)
{
	if (atomic_get(&block_cpu_idle)) {
		return;
	}

	(void)arch_irq_lock();
	k_cpu_idle();
}

BUILD_ASSERT(DT_CHILD_NUM(DT_PATH(cpus)) == 1);

#define NRF_CPU_DEFINE(_node)									\
	PM_EVENT_DEVICE_DT_DEFINE(_node, shim_request_latency, 0, 2);				\
												\
	static const struct shim_config config = {						\
		.event_device = PM_EVENT_DEVICE_DT_GET(_node),					\
	};											\
												\
	DEVICE_DT_DEFINE(_node, shim_init, NULL, NULL, &config, PRE_KERNEL_1, 0, NULL);

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(cpus), NRF_CPU_DEFINE)
