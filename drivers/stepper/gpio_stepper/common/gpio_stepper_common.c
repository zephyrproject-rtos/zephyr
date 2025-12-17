/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpio_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_stepper_common, CONFIG_STEPPER_LOG_LEVEL);

void gpio_stepper_trigger_callback(const struct device *dev, enum stepper_ctrl_event event)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
	/* Dispatch to msgq instead of raising directly */
	int ret = k_msgq_put(&data->event_msgq, &event, K_NO_WAIT);

	if (ret != 0) {
		LOG_WRN("Failed to put event in msgq: %d", ret);
	}

	ret = k_work_submit(&data->event_callback_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit work item: %d", ret);
	}
#else
	LOG_WRN_ONCE("Event callback called from ISR context without ISR safe events enabled");
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
static void gpio_stepper_work_event_handler(struct k_work *work)
{
	struct gpio_stepper_common_data *data =
		CONTAINER_OF(work, struct gpio_stepper_common_data, event_callback_work);
	enum stepper_ctrl_event event;
	int ret;

	ret = k_msgq_get(&data->event_msgq, &event, K_NO_WAIT);
	if (ret != 0) {
		return;
	}

	/* Run the callback */
	if (data->callback != NULL) {
		data->callback(data->dev, event, data->event_cb_user_data);
	}

	/* If there are more pending events, resubmit this work item to handle them */
	if (k_msgq_num_used_get(&data->event_msgq) > 0) {
		k_work_submit(work);
	}
}
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */

int gpio_stepper_common_init(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;
	int ret;

	if (config->timing_source->init) {
		ret = config->timing_source->init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize timing source: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
	struct gpio_stepper_common_data *data = dev->data;

	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_ctrl_event),
		    CONFIG_STEPPER_GPIO_STEPPER_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, gpio_stepper_work_event_handler);
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */
	return 0;
}
