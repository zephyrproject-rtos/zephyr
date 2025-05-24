/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEPPER_EVENT_HANDLER_H_
#define ZEPHYR_DRIVER_STEPPER_STEPPER_EVENT_HANDLER_H_

#include <zephyr/drivers/stepper.h>

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

#endif /* ZEPHYR_DRIVER_STEPPER_STEPPER_EVENT_HANDLER_H_ */
