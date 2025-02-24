/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/event_device.h>

#define SAMPLE_EVENT_DEVICE_NODE DT_ALIAS(event_device)
#define SAMPLE_EVENT_DEVICE      PM_EVENT_DEVICE_DT_GET(SAMPLE_EVENT_DEVICE_NODE)
#define SAMPLE_TIMEOUT           K_SECONDS(10)

static const struct pm_event_device *sample_event_device = SAMPLE_EVENT_DEVICE;

int main(void)
{
	uint32_t max_event_state;
	const char *dev_name;
	struct pm_event_device_event event;
	int64_t effective_uptime_ticks;

	max_event_state = pm_event_device_get_max_event_state(sample_event_device);
	dev_name = pm_event_device_get_dev(sample_event_device)->name;

	while (1) {
		printk("requesting event state %u for %s\n", max_event_state, dev_name);
		effective_uptime_ticks = pm_event_device_request_event(sample_event_device,
								       &event,
								       max_event_state);

		printk("wait until event state is effective\n");
		k_sleep(K_TIMEOUT_ABS_TICKS(effective_uptime_ticks));
		printk("event state is effective\n");
		k_sleep(SAMPLE_TIMEOUT);

		printk("releasing event state %u for %s\n", max_event_state, dev_name);
		pm_event_device_release_event(&event);
		k_sleep(SAMPLE_TIMEOUT);
	}

	return 0;
}
