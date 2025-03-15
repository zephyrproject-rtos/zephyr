/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/event_domain.h>

static uint8_t floor_event_latency_index(const struct pm_event_domain *event_domain,
					 uint32_t event_latency_us)
{
	uint32_t latency_us;
	uint16_t i;

	i = 0;
	do {
		latency_us = event_domain->event_latencies_us[i];

		if (latency_us <= event_latency_us) {
			break;
		}

		i++;
	} while (i < (event_domain->event_latencies_us_size - 1));

	return i;
}

static const uint8_t *floored_event_latency_states(const struct pm_event_domain *event_domain,
						   uint32_t event_latency_us)
{
	uint16_t i;

	i = floor_event_latency_index(event_domain, event_latency_us);
	return &event_domain->event_device_states[i * event_domain->event_devices_size];
}

uint32_t pm_event_domain_floor_event_latency_us(const struct pm_event_domain *event_domain,
						uint32_t event_latency_us)
{
	uint16_t i;

	i = floor_event_latency_index(event_domain, event_latency_us);
	return event_domain->event_latencies_us[i];
}

const uint32_t *pm_event_domain_get_event_latencies_us(const struct pm_event_domain *event_domain)
{
	return event_domain->event_latencies_us;
}

uint8_t pm_event_domain_get_event_latencies_us_size(const struct pm_event_domain *event_domain)
{
	return event_domain->event_latencies_us_size;
}

int64_t pm_event_domain_schedule_event(const struct pm_event_domain_event *event,
				       uint32_t event_latency_us,
				       int64_t event_uptime_ticks)
{
	const struct pm_event_domain *event_domain = event->event_domain;
	const uint8_t *event_device_event_states;
	int64_t max_effective_uptime_ticks;
	const struct pm_event_device *event_device;
	struct pm_event_device_event *event_device_event;
	uint8_t event_device_event_state;
	int64_t effective_uptime_ticks;

	event_device_event_states = floored_event_latency_states(event_domain, event_latency_us);
	max_effective_uptime_ticks = 0;

	for (size_t i = 0; i < event_domain->event_devices_size; i++) {
		event_device = event_domain->event_devices[i];
		event_device_event = &event->event_device_events[i];
		event_device_event_state = event_device_event_states[i];

		effective_uptime_ticks = pm_event_device_schedule_event(event_device,
									event_device_event,
									event_device_event_state,
									event_uptime_ticks);

		max_effective_uptime_ticks = MAX(max_effective_uptime_ticks,
						 effective_uptime_ticks);
	}

	return max_effective_uptime_ticks;
}

int64_t pm_event_domain_reschedule_event(const struct pm_event_domain_event *event,
					 uint32_t event_latency_us,
					 int64_t event_uptime_ticks)
{
	const struct pm_event_domain *event_domain = event->event_domain;
	const uint8_t *event_device_event_states;
	int64_t max_effective_uptime_ticks;
	struct pm_event_device_event *event_device_event;
	uint8_t event_device_event_state;
	int64_t effective_uptime_ticks;

	event_device_event_states = floored_event_latency_states(event_domain, event_latency_us);
	max_effective_uptime_ticks = 0;

	for (size_t i = 0; i < event_domain->event_devices_size; i++) {
		event_device_event = &event->event_device_events[i];
		event_device_event_state = event_device_event_states[i];

		effective_uptime_ticks = pm_event_device_reschedule_event(event_device_event,
									  event_device_event_state,
									  event_uptime_ticks);

		max_effective_uptime_ticks = MAX(max_effective_uptime_ticks,
						 effective_uptime_ticks);
	}

	return max_effective_uptime_ticks;
}

int64_t pm_event_domain_request_event(const struct pm_event_domain_event *event,
				      uint32_t event_latency_us)
{
	const struct pm_event_domain *event_domain = event->event_domain;
	const uint8_t *event_device_event_states;
	int64_t max_effective_uptime_ticks;
	const struct pm_event_device *event_device;
	struct pm_event_device_event *event_device_event;
	uint8_t event_device_event_state;
	int64_t effective_uptime_ticks;

	event_device_event_states = floored_event_latency_states(event_domain, event_latency_us);
	max_effective_uptime_ticks = 0;

	for (size_t i = 0; i < event_domain->event_devices_size; i++) {
		event_device = event_domain->event_devices[i];
		event_device_event = &event->event_device_events[i];
		event_device_event_state = event_device_event_states[i];

		effective_uptime_ticks = pm_event_device_request_event(event_device,
								       event_device_event,
								       event_device_event_state);

		max_effective_uptime_ticks = MAX(max_effective_uptime_ticks,
						 effective_uptime_ticks);
	}

	return max_effective_uptime_ticks;
}

int64_t pm_event_domain_rerequest_event(const struct pm_event_domain_event *event,
					uint32_t event_latency_us)
{
	const struct pm_event_domain *event_domain = event->event_domain;
	const uint8_t *event_device_event_states;
	int64_t max_effective_uptime_ticks;
	struct pm_event_device_event *event_device_event;
	uint8_t event_device_event_state;
	int64_t effective_uptime_ticks;

	event_device_event_states = floored_event_latency_states(event_domain, event_latency_us);
	max_effective_uptime_ticks = 0;

	for (size_t i = 0; i < event_domain->event_devices_size; i++) {
		event_device_event = &event->event_device_events[i];
		event_device_event_state = event_device_event_states[i];

		effective_uptime_ticks = pm_event_device_rerequest_event(event_device_event,
									 event_device_event_state);

		max_effective_uptime_ticks = MAX(max_effective_uptime_ticks,
						 effective_uptime_ticks);
	}

	return max_effective_uptime_ticks;
}

void pm_event_domain_release_event(const struct pm_event_domain_event *event)
{
	const struct pm_event_domain *event_domain = event->event_domain;
	struct pm_event_device_event *event_device_event;

	for (size_t i = 0; i < event_domain->event_devices_size; i++) {
		event_device_event = &event->event_device_events[i];
		pm_event_device_release_event(event_device_event);
	}
}

DT_FOREACH_STATUS_OKAY(event_domain, PM_EVENT_DOMAIN_DT_DEFINE)
