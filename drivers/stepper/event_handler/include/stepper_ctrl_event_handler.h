/*
 * Copyright 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright 2026 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_CTRL_EVENT_HANDLER_H_
#define ZEPHYR_DRIVER_STEPPER_CTRL_EVENT_HANDLER_H_

/**
 * @brief Stepper Motion Controller Event Handler APIs
 * @ingroup stepper_ctrl_interface
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>

/**
 * @brief Stepper controller event common data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct stepper_ctrl_event_handler_data {
	const struct device *dev;
	stepper_ctrl_event_callback_t callback;
	void *event_cb_user_data;

#ifdef CONFIG_STEPPER_CTRL_ISR_SAFE_EVENTS
	struct k_work event_callback_work;
	struct k_msgq event_msgq;
	uint8_t event_msgq_buffer[CONFIG_STEPPER_CTRL_EVENT_QUEUE_LEN *
				  sizeof(enum stepper_ctrl_event)];
#endif /* CONFIG_STEPPER_CTRL_ISR_SAFE_EVENTS */
};

/**
 * @brief Initialize stepper controller event common data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEPPER_CTRL_EVENT_HANDLER_DT_DATA_INIT(node_id)                                           \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	}

/**
 * @brief Initialize common tepper controller event handler data from devicetree instance.
 * @param inst Instance.
 */
#define STEPPER_CTRL_EVENT_HANDLER_DT_INST_DATA_INIT(inst)                                         \
	STEPPER_CTRL_EVENT_HANDLER_DT_DATA_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structure.
 *
 * @param data Name of the data structure.
 */
#define STEPPER_CONTROLLER_EVENT_COMMON_STRUCT_CHECK(data)                                         \
	BUILD_ASSERT(offsetof(data, event_common) == 0,                                            \
		     "struct stepper_controller_event_common_data must be placed first");

/**
 * @brief Common function to initialize the event handling at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev stepper_ctrl device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int stepper_ctrl_event_handler_init(const struct device *dev);

/**
 * @brief Manage triggering the callback function for stepper_ctrl events.
 * @param dev Pointer to the device structure.
 * @param event The stepper_event to trigger the callback for.
 */
void stepper_ctrl_event_handler_process_cb(const struct device *dev,
						 enum stepper_ctrl_event event);

/**
 * @brief Set a callback function for stepper_ctrl events.
 *
 * This function sets a user-defined callback that will be invoked when a stepper_ctrl_event
 * occurs.
 *
 * @param dev Pointer to the device structure.
 * @param callback The callback function to set.
 * @param user_data Pointer to user-defined data that will be passed to the callback.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int stepper_ctrl_event_handler_set_event_cb(const struct device *dev,
							  stepper_ctrl_event_callback_t callback,
							  void *user_data)
{
	struct stepper_ctrl_event_handler_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	return 0;
}

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_CTRL_EVENT_HANDLER_H_ */
