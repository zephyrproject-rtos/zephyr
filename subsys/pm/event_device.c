/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/event_device.h>
#include <timeout_q.h>

#define UPDATE_UPTIME_TICKS_NONE INT64_MAX

static uint8_t clip_event_state(const struct pm_event_device *event_device,
				 uint8_t event_state)
{
	return MIN(event_state, event_device->event_state_count - 1);
}

static void add_event(const struct pm_event_device *event_device,
		      struct pm_event_device_event *event)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	sys_slist_append(&runtime->event_list, &event->node);
}

static void remove_event(const struct pm_event_device *event_device,
			 struct pm_event_device_event *event)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	(void)sys_slist_find_and_remove(&runtime->event_list, &event->node);
}

static int64_t get_event_active_uptime_ticks(const struct pm_event_device_event *event)
{
	const struct pm_event_device *event_device = event->event_device;

	return event->uptime_ticks - event_device->event_state_request_latency_ticks - 1;
}

static int64_t get_uptime_ticks_post_request(const struct pm_event_device *event_device,
					     int64_t uptime_ticks)
{
	return uptime_ticks + event_device->event_state_request_latency_ticks + 1;
}

static bool event_is_active(const struct pm_event_device_event *event, int64_t uptime_ticks)
{
	const struct pm_event_device *event_device = event->event_device;

	return get_event_active_uptime_ticks(event) <=
	       get_uptime_ticks_post_request(event_device, uptime_ticks);
}

static bool event_state_request_is_required(const struct pm_event_device *event_device,
					    uint8_t event_state)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	return runtime->requested_event_state != event_state;
}

static bool event_state_request_is_idle(const struct pm_event_device *event_device,
					int64_t uptime_ticks)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	if (runtime->request_uptime_ticks == -1) {
		return false;
	}

	return runtime->request_uptime_ticks <
	       uptime_ticks - event_device->event_state_request_latency_ticks;
}

static int64_t get_event_device_idle_uptime_ticks(const struct pm_event_device *event_device)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	return runtime->request_uptime_ticks +
	       event_device->event_state_request_latency_ticks +
	       1;
}

static void request_event_state(const struct pm_event_device *event_device,
				uint8_t event_state,
				int64_t uptime_ticks)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	runtime->requested_event_state = event_state;
	runtime->request_uptime_ticks = uptime_ticks;

	event_device->event_state_request(event_device->dev, event_state);
}

static bool update_timeout_is_required(int64_t update_uptime_ticks)
{
	return update_uptime_ticks != UPDATE_UPTIME_TICKS_NONE;
}

static void update(const struct pm_event_device *event_device, int64_t uptime_ticks);

static void update_timeout_handler(struct _timeout *update_timeout)
{
	struct pm_event_device_runtime *runtime = CONTAINER_OF(update_timeout,
							       struct pm_event_device_runtime,
							       update_timeout);
	k_spinlock_key_t key;

	key = k_spin_lock(&runtime->lock);
	update(runtime->event_device, k_uptime_ticks());
	k_spin_unlock(&runtime->lock, key);
}

static void set_update_timeout(const struct pm_event_device *event_device,
			       int64_t update_uptime_ticks)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	(void)z_abort_timeout(&runtime->update_timeout);
	z_add_timeout(&runtime->update_timeout,
		      update_timeout_handler,
		      K_TIMEOUT_ABS_TICKS(update_uptime_ticks));
}

static void clear_update_timeout(const struct pm_event_device *event_device)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;

	(void)z_abort_timeout(&runtime->update_timeout);
}

static bool event_state_is_active(const struct pm_event_device_event *event, uint8_t event_state)
{
	const struct pm_event_device *event_device = event->event_device;
	struct pm_event_device_runtime *runtime = event_device->runtime;

	return runtime->requested_event_state == event_state;
}

static int64_t get_uptime_ticks_post_idle_and_request(const struct pm_event_device *event_device)
{
	return get_event_device_idle_uptime_ticks(event_device) +
	       event_device->event_state_request_latency_ticks +
	       1;
}

static int64_t get_event_effective_uptime_ticks(const struct pm_event_device_event *event,
						int64_t uptime_ticks)
{
	const struct pm_event_device *event_device = event->event_device;

	if (event_state_is_active(event, event->event_state)) {
		return MAX(event->uptime_ticks, uptime_ticks);
	}

	if (event_state_request_is_idle(event_device, uptime_ticks)) {
		return MAX(event->uptime_ticks,
			   get_uptime_ticks_post_request(event_device, uptime_ticks));
	}

	return MAX(event->uptime_ticks, get_uptime_ticks_post_idle_and_request(event_device));
}

static void update(const struct pm_event_device *event_device, int64_t uptime_ticks)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;
	uint8_t required_event_state;
	int64_t update_uptime_ticks;
	struct pm_event_device_event *event;

	required_event_state = 0;
	update_uptime_ticks = UPDATE_UPTIME_TICKS_NONE;

	SYS_SLIST_FOR_EACH_CONTAINER(&runtime->event_list, event, node) {
		if (event_is_active(event, uptime_ticks)) {
			required_event_state = MAX(event->event_state,
						   required_event_state);
		} else {
			update_uptime_ticks = MIN(get_event_active_uptime_ticks(event),
						  update_uptime_ticks);
		}
	}

	if (event_state_request_is_required(event_device, required_event_state)) {
		if (event_state_request_is_idle(event_device, uptime_ticks)) {
			request_event_state(event_device,
					    required_event_state,
					    uptime_ticks);
		} else {
			update_uptime_ticks = get_event_device_idle_uptime_ticks(event_device);
		}
	}

	if (update_timeout_is_required(update_uptime_ticks)) {
		set_update_timeout(event_device, update_uptime_ticks);
	} else {
		clear_update_timeout(event_device);
	}
}

const struct device *pm_event_device_get_dev(const struct pm_event_device *event_device)
{
	return event_device->dev;
}

uint8_t pm_event_device_get_event_state_count(const struct pm_event_device *event_device)
{
	return event_device->event_state_count;
}

uint8_t pm_event_device_get_max_event_state(const struct pm_event_device *event_device)
{
	return event_device->event_state_count - 1;
}

void pm_event_device_init(const struct pm_event_device *event_device)
{
	request_event_state(event_device, 0, k_uptime_ticks());
}

int64_t pm_event_device_schedule_event(const struct pm_event_device *event_device,
				       struct pm_event_device_event *event,
				       uint8_t event_state,
				       int64_t uptime_ticks)
{
	struct pm_event_device_runtime *runtime = event_device->runtime;
	k_spinlock_key_t key;
	int64_t current_uptime_ticks;
	int64_t effective_uptime_ticks;

	event->event_device = event_device;
	event->event_state = clip_event_state(event_device, event_state);
	event->uptime_ticks = uptime_ticks;

	key = k_spin_lock(&runtime->lock);
	current_uptime_ticks = k_uptime_ticks();
	effective_uptime_ticks = get_event_effective_uptime_ticks(event, current_uptime_ticks);
	add_event(event_device, event);
	update(event_device, current_uptime_ticks);
	k_spin_unlock(&runtime->lock, key);

	return effective_uptime_ticks;
}

int64_t pm_event_device_reschedule_event(struct pm_event_device_event *event,
					 uint8_t event_state,
					 int64_t uptime_ticks)
{
	const struct pm_event_device *event_device = event->event_device;
	struct pm_event_device_runtime *runtime = event_device->runtime;
	k_spinlock_key_t key;
	int64_t current_uptime_ticks;
	int64_t effective_uptime_ticks;

	key = k_spin_lock(&runtime->lock);
	event->event_state = clip_event_state(event_device, event_state);
	event->uptime_ticks = uptime_ticks;
	current_uptime_ticks = k_uptime_ticks();
	effective_uptime_ticks = get_event_effective_uptime_ticks(event, current_uptime_ticks);
	update(event->event_device, k_uptime_ticks());
	k_spin_unlock(&runtime->lock, key);

	return effective_uptime_ticks;
}

int64_t pm_event_device_request_event(const struct pm_event_device *event_device,
				      struct pm_event_device_event *event,
				      uint8_t event_state)
{
	return pm_event_device_schedule_event(event_device, event, event_state, 0);
}

int64_t pm_event_device_rerequest_event(struct pm_event_device_event *event,
					uint8_t event_state)
{
	return pm_event_device_reschedule_event(event, event_state, 0);

}

void pm_event_device_release_event(struct pm_event_device_event *event)
{
	const struct pm_event_device *event_device = event->event_device;
	struct pm_event_device_runtime *runtime = event_device->runtime;
	k_spinlock_key_t key;

	key = k_spin_lock(&runtime->lock);
	remove_event(event_device, event);
	update(event_device, k_uptime_ticks());
	k_spin_unlock(&runtime->lock, key);
}
