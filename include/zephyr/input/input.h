/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_H_
#define ZEPHYR_INCLUDE_INPUT_H_

/**
 * @brief Input Interface
 * @defgroup input_interface Input Interface
 * @since 3.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Input event structure.
 *
 * This structure represents a single input event, for example a key or button
 * press for a single button, or an absolute or relative coordinate for a
 * single axis.
 */
struct input_event {
	/** Device generating the event or NULL. */
	const struct device *dev;
	/** Sync flag. */
	uint8_t sync;
	/** Event type (see @ref INPUT_EV_CODES). */
	uint8_t type;
	/**
	 * Event code (see @ref INPUT_KEY_CODES, @ref INPUT_BTN_CODES,
	 * @ref INPUT_ABS_CODES, @ref INPUT_REL_CODES, @ref INPUT_MSC_CODES).
	 */
	uint16_t code;
	/** Event value. */
	int32_t value;
};

/**
 * @brief Report a new input event.
 *
 * This causes all the callbacks for the specified device to be executed,
 * either synchronously or through the input thread if utilized.
 *
 * @param dev Device generating the event or NULL.
 * @param type Event type (see @ref INPUT_EV_CODES).
 * @param code Event code (see @ref INPUT_KEY_CODES, @ref INPUT_BTN_CODES,
 *        @ref INPUT_ABS_CODES, @ref INPUT_REL_CODES, @ref INPUT_MSC_CODES).
 * @param value Event value.
 * @param sync Set the synchronization bit for the event.
 * @param timeout Timeout for reporting the event, ignored if
 *                @kconfig{CONFIG_INPUT_MODE_SYNCHRONOUS} is used.
 * @retval 0 if the message has been processed.
 * @retval negative if @kconfig{CONFIG_INPUT_MODE_THREAD} is enabled and the
 *         message failed to be enqueued.
 */
int input_report(const struct device *dev,
		 uint8_t type, uint16_t code, int32_t value, bool sync,
		 k_timeout_t timeout);

/**
 * @brief Report a new @ref INPUT_EV_KEY input event, note that value is
 * converted to either 0 or 1.
 *
 * @see input_report() for more details.
 */
static inline int input_report_key(const struct device *dev,
				   uint16_t code, int32_t value, bool sync,
				   k_timeout_t timeout)
{
	return input_report(dev, INPUT_EV_KEY, code, !!value, sync, timeout);
}

/**
 * @brief Report a new @ref INPUT_EV_REL input event.
 *
 * @see input_report() for more details.
 */
static inline int input_report_rel(const struct device *dev,
				   uint16_t code, int32_t value, bool sync,
				   k_timeout_t timeout)
{
	return input_report(dev, INPUT_EV_REL, code, value, sync, timeout);
}

/**
 * @brief Report a new @ref INPUT_EV_ABS input event.
 *
 * @see input_report() for more details.
 */
static inline int input_report_abs(const struct device *dev,
				   uint16_t code, int32_t value, bool sync,
				   k_timeout_t timeout)
{
	return input_report(dev, INPUT_EV_ABS, code, value, sync, timeout);
}

/**
 * @brief Returns true if the input queue is empty.
 *
 * This can be used to batch input event processing until the whole queue has
 * been emptied. Always returns true if @kconfig{CONFIG_INPUT_MODE_SYNCHRONOUS}
 * is enabled.
 */
bool input_queue_empty(void);

/**
 * @brief Input callback structure.
 */
struct input_callback {
	/** @ref device pointer or NULL. */
	const struct device *dev;
	/** The callback function. */
	void (*callback)(struct input_event *evt, void *user_data);
	/** User data pointer. */
	void *user_data;
};

/**
 * @brief Register a callback structure for input events.
 *
 * The @p _dev field can be used to only invoke callback for events generated
 * by a specific device. Setting dev to NULL causes callback to be invoked for
 * every event.
 *
 * @param _dev @ref device pointer or NULL.
 * @param _callback The callback function.
 * @param _user_data Pointer to user specified data.
 */
#define INPUT_CALLBACK_DEFINE(_dev, _callback, _user_data)                     \
	static const STRUCT_SECTION_ITERABLE(input_callback,                   \
					     _input_callback__##_callback) = { \
		.dev = _dev,                                                   \
		.callback = _callback,                                         \
		.user_data = _user_data,                                       \
	}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_H_ */
