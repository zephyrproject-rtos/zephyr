/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private input driver APIs
 */

#ifndef ZEPHYR_DRIVERS_INPUT_INPUT_INTERNAL_H_
#define ZEPHYR_DRIVERS_INPUT_INPUT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/drivers/input.h>
#include <zephyr/sys/ring_buffer.h>

#define INPUT_ASSERT(cond, fmt, ...) { \
	if (!cond) { \
		printk("ASSERT FAIL [%s] @ %s:%d\n", #cond, __FILE__, __LINE__); \
		printk("\t" fmt "\n", ##__VA_ARGS__); \
		k_oops(); \
	} \
}

/**
 * @brief Input device spec
 *
 */
struct input_dev {
	struct ring_buf *buf;
#ifdef CONFIG_ENABLE_INPUT_ISR_LOCK
	struct k_spinlock lock;
#else
	struct k_mutex mutex;
#endif
	struct k_sem sem;
	k_timeout_t readtimeo;
};

/**
 * @brief Setup a input internal device instance.
 *
 * @param dev Pointer to the input device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_internal_setup(struct input_dev *dev);

/**
 * @brief Release a input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_internal_release(struct input_dev *dev);

/**
 * @brief Get attribute from the input internal device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param type The type of attribute which get from the driver instance.
 * @param attr Pointer to the attribute structure which get from the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_internal_attr_get(struct input_dev *dev,
			enum input_attr_type type, union input_attr_data *data);

/**
 * @brief Set attribute from the input internal device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param type The type of attribute which set to the driver instance.
 * @param attr Pointer to the attribute structure which set to the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_internal_attr_set(struct input_dev *dev,
			enum input_attr_type type, union input_attr_data *data);

/**
 * @brief Read event from the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param event Pointer to the event structure which read from the input internal device instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_internal_event_read(struct input_dev *dev, struct input_event *event);

/**
 * @brief Report event to the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param event Pointer to the event structure which report to the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
int input_event(struct input_dev *dev, uint16_t type, uint16_t code, int32_t value);

/**
 * @brief Write event from the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param event Pointer to the event structure which write to the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int input_internal_event_write(struct input_dev *dev, struct input_event *event)
{
	return input_event(dev, event->type, event->code, event->value);
}

/**
 * @brief Report EV_SYN event to the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int input_sync(struct input_dev *dev)
{
	return input_event(dev, EV_SYN, SYN_REPORT, 0);
}

/**
 * @brief Report EV_KEY event to the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param code Key code which defined in input_key_code.h
 * @param value Key event (PRESS, RELEASE, LONGPRESS, HOLDPRESS, LONGRELEASE, etc)
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int input_report_key(struct input_dev *dev, uint16_t code, int32_t value)
{
	return input_event(dev, EV_KEY, code, value);
}

/**
 * @brief Report EV_REL event to the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param code Relative axes (REL_X, REL_Y, REL_Z, etc)
 * @param value Relative Position value
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int input_report_rel(struct input_dev *dev, unsigned int code, int value)
{
	return input_event(dev, EV_REL, code, value);
}

/**
 * @brief Report EV_ABS event to the input internal device instance.
 *
 * @param dev Pointer to the input internal device structure for the driver instance.
 * @param code Absolute axes (ABS_X, ABS_Y, ABS_Z, etc)
 * @param value Relative Position value
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int input_report_abs(struct input_dev *dev, unsigned int code, int value)
{
	return input_event(dev, EV_ABS, code, value);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INPUT_INPUT_INTERNAL_H_ */
