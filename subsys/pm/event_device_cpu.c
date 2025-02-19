/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/event_device.h>
#include <zephyr/pm/policy.h>

struct cpu_event_device_data {
	bool requested;
	struct pm_policy_latency_request request;
};

struct cpu_event_device_config {
	const struct pm_event_device *event_device;
	const uint32_t *exit_latencies_us;
};

static void cpu_request_latency(const struct device *dev, uint8_t event_state)
{
	struct cpu_event_device_data *data = dev->data;
	const struct cpu_event_device_config *config = dev->config;
	const struct pm_event_device *event_device = config->event_device;
	struct pm_policy_latency_request *request = &data->request;
	const uint32_t *exit_latencies_us = config->exit_latencies_us;
	uint8_t max_event_state;
	uint32_t latency_us;

	/* cpu power states are ordered in increasing event latency */
	max_event_state = pm_event_device_get_max_event_state(event_device);
	latency_us = exit_latencies_us[max_event_state - event_state];

	if (data->requested) {
		pm_policy_latency_request_update(request, latency_us + 1);
	} else {
		pm_policy_latency_request_add(request, latency_us + 1);
		data->requested = true;
	}
}

static int cpu_event_device_init(const struct device *dev)
{
	const struct cpu_event_device_config *config = dev->config;
	const struct pm_event_device *event_device = config->event_device;

	pm_event_device_init(event_device);
	return 0;
}

#define CPU_EVENT_DEVICE_SYMNAME(_node, _name) \
	_CONCAT(_name, DT_NODE_CHILD_IDX(_node))

#define CPU_EVENT_DEVICE_EXIT_LATENCY_NS_LISTIFY(_idx, _node) \
	DT_PROP(DT_PROP_BY_IDX(_node, cpu_power_states, _idx), exit_latency_us)

#define CPU_EVENT_DEVICE_EXIT_LATENCIES_DEFINE(_node)						\
	static const uint32_t CPU_EVENT_DEVICE_SYMNAME(_node, exit_latencies_us)[] = {		\
		0,										\
		LISTIFY(									\
			DT_PROP_LEN(_node, cpu_power_states),					\
			CPU_EVENT_DEVICE_EXIT_LATENCY_NS_LISTIFY,				\
			(,),									\
			_node									\
		)										\
	}

#define CPU_EVENT_DEVICE_DEFINE(_node)								\
	CPU_EVENT_DEVICE_EXIT_LATENCIES_DEFINE(_node);						\
	static struct cpu_event_device_data CPU_EVENT_DEVICE_SYMNAME(_node, data);		\
	static const struct cpu_event_device_config CPU_EVENT_DEVICE_SYMNAME(_node, config) = {	\
		.event_device = PM_EVENT_DEVICE_DT_GET(_node),					\
		.exit_latencies_us = CPU_EVENT_DEVICE_SYMNAME(_node, exit_latencies_us),	\
	};											\
												\
	PM_EVENT_DEVICE_DT_DEFINE(								\
		_node,										\
		cpu_request_latency,								\
		0,										\
		DT_PROP_LEN(_node, cpu_power_states)						\
	);											\
												\
	DEVICE_DT_DEFINE(									\
		_node,										\
		cpu_event_device_init,								\
		NULL,										\
		&CPU_EVENT_DEVICE_SYMNAME(_node, data),						\
		&CPU_EVENT_DEVICE_SYMNAME(_node, config),					\
		PRE_KERNEL_1,									\
		0,										\
		NULL										\
	);

#define CPU_EVENT_DEVICE_DEFINE_IF(_node)							\
	IF_ENABLED(										\
		DT_NODE_EXISTS(DT_PROP(_node, cpu_power_states)),				\
		(CPU_EVENT_DEVICE_DEFINE(_node))						\
	)

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(cpus), CPU_EVENT_DEVICE_DEFINE_IF)
