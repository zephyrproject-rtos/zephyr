/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/* Used only on the rc receive work queue: no lock needed */

#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>

#include "rc_internal.h"

static uint64_t rc_now_ns(void)
{
	return k_ticks_to_ns_floor64(k_uptime_ticks());
}

static void rc_key_release(struct rc_dev_data *data)
{
	struct rc_event evt = data->key;

	data->key_pressed = false;
	evt.timestamp_ns = rc_now_ns();
	evt.flags = (data->key.flags & RC_EVENT_FLAG_TOGGLE) | RC_EVENT_FLAG_RELEASE;
	rc_event_report(data->dev, &evt);
}

static void rc_key_arm(struct rc_dev_data *data)
{
	int64_t delay = k_us_to_ticks_ceil64((uint32_t)atomic_get(&data->rx_timeout_us) +
					     (uint32_t)data->key_decoder->repeat_period_ms * 1000U);

	data->keyup_deadline = k_uptime_ticks() + delay;
	(void)rc_rx_reschedule(&data->keyup_work, K_TICKS(delay));
}

static void rc_keyup_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rc_dev_data *data = CONTAINER_OF(dwork, struct rc_dev_data, keyup_work);
	int64_t remaining = data->keyup_deadline - k_uptime_ticks();

	if (!data->key_pressed) {
		return;
	}

	/* A frame extended the deadline after this run was queued */
	if (remaining > 0) {
		(void)rc_rx_reschedule(&data->keyup_work, K_TICKS(remaining));
		return;
	}

	rc_key_release(data);
}

void rc_key_frame(const struct device *dev, const struct rc_protocol *p,
		  const struct rc_decode_result *res)
{
	struct rc_dev_data *data = rc_data(dev);
	uint16_t toggle = res->flags & RC_EVENT_FLAG_TOGGLE;
	struct rc_event evt;
	bool new_key;

	if ((res->flags & RC_EVENT_FLAG_REPEAT) != 0U) {
		if (!data->key_pressed) {
			return;
		}

		evt = data->key;
		evt.timestamp_ns = rc_now_ns();
		evt.flags = (data->key.flags & RC_EVENT_FLAG_TOGGLE) | RC_EVENT_FLAG_REPEAT;
		rc_key_arm(data);
		rc_event_report(dev, &evt);
		return;
	}

	new_key = !data->key_pressed || (data->key.proto != res->proto) ||
		  (data->key.scancode != res->scancode) ||
		  ((data->key.flags & RC_EVENT_FLAG_TOGGLE) != toggle);

	if (new_key && data->key_pressed) {
		rc_key_release(data);
	}

	data->key = (struct rc_event){
		.timestamp_ns = rc_now_ns(),
		.flags = toggle,
		.proto = res->proto,
		.scancode = res->scancode,
	};
	data->key_decoder = p;
	data->key_pressed = true;

	evt = data->key;
	if (!new_key) {
		evt.flags |= RC_EVENT_FLAG_REPEAT;
	}

	rc_key_arm(data);
	rc_event_report(dev, &evt);
}

void rc_key_init(const struct device *dev)
{
	struct rc_dev_data *data = rc_data(dev);

	data->key_pressed = false;
	k_work_init_delayable(&data->keyup_work, rc_keyup_work_handler);
}
