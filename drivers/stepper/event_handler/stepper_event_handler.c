/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_event_handler.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_event_handler, CONFIG_STEPPER_LOG_LEVEL);

int stepper_post_event(const struct device *dev, stepper_event_callback_t cb,
		       enum stepper_event event, void *user_data)
{
	if (cb == NULL) {
		LOG_ERR("Callback is NULL for device %s", dev->name);
		return -EINVAL;
	}

	if (!k_is_in_isr()) {
		cb(dev, event, user_data);
		return 0;
	}

	STRUCT_SECTION_FOREACH(stepper_event_handler, entry) {
		if (entry->dev == dev) {
			struct stepper_event_data data = {
				.event_cb = cb,
				.event = event,
				.user_data = user_data,
			};
			LOG_DBG("Posting event %d for device %s with cb %p", event, dev->name,
				data.event_cb);
			k_msgq_put(&entry->event_msgq, &data, K_NO_WAIT);
			k_work_submit(&entry->event_callback_work);
			return 0;
		}
	}
	return -ENODEV;
}

static void stepper_work_event_handler(struct k_work *work)
{
	struct stepper_event_handler *entry =
		CONTAINER_OF(work, struct stepper_event_handler, event_callback_work);
	struct stepper_event_data event_data;
	int ret;

	LOG_DBG("Starting event handler for device %s", entry->dev->name);

	ret = k_msgq_get(&entry->event_msgq, &event_data, K_NO_WAIT);
	if (ret != 0) {
		return;
	}

	/* Run the callback */
	if (event_data.event_cb != NULL) {
		LOG_DBG("Handling event %d for device %s with cb %p", event_data.event,
			entry->dev->name, event_data.event_cb);
		event_data.event_cb(entry->dev, event_data.event, event_data.user_data);
	}

	/* If there are more pending events, resubmit this work item to handle them */
	if (k_msgq_num_used_get(&entry->event_msgq) > 0) {
		k_work_submit(work);
	}
}

static int stepper_event_handler_init(void)
{
	STRUCT_SECTION_FOREACH(stepper_event_handler, entry) {
		k_msgq_init(&entry->event_msgq, entry->event_msgq_buffer,
			    sizeof(struct stepper_event_data),
			    CONFIG_STEPPER_EVENT_HANDLER_QUEUE_LEN);
		k_work_init(&entry->event_callback_work, stepper_work_event_handler);
	}
	return 0;
}

SYS_INIT(stepper_event_handler_init, APPLICATION, CONFIG_STEPPER_INIT_PRIORITY);
