/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_EVENT_HANDLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_EVENT_HANDLER_H_

#include <zephyr/drivers/stepper.h>

#ifdef CONFIG_STEPPER_EVENT_HANDLER

/*
 * @brief post an event to the stepper event handler
 *
 * @param dev Pointer to the stepper device
 * @param cb Callback function to be called when the event is processed
 * @param event The stepper event to post
 * @param user_data User data to be passed to the callback function
 * @return 0 on success, negative error code on failure
 */
int stepper_post_event(const struct device *dev, stepper_event_callback_t cb,
		       enum stepper_event event, void *user_data);

#else

/**
 * @brief Execute the stepper event handler callback directly if the event handler is not enabled
 *
 * @param dev Pointer to the stepper device
 * @param cb Callback function to be called when the event is processed
 * @param event The stepper event to post
 * @param user_data User data to be passed to the callback function
 * @return 0 on success, negative error code on failure
 */
static inline int stepper_post_event(const struct device *dev, stepper_event_callback_t cb,
				     enum stepper_event event, void *user_data)
{
	if (cb == NULL) {
		return -EINVAL;
	}
	cb(dev, event, user_data);
	return 0;
}

#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_EVENT_HANDLER_H_ */
