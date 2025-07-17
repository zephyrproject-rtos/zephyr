/*
 * Copyright (c) 2025, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KPP Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KPP_H_
#define ZEPHYR_INCLUDE_DRIVERS_KPP_H_

/**
 * @brief KPP Interface
 * @defgroup kpp_interface KPP Interface
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <fsl_kpp.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INPUT_KPP_COLUMNNUM_MAX  KPP_KEYPAD_COLUMNNUM_MAX
#define INPUT_KPP_ROWNUM_MAX     KPP_KEYPAD_ROWNUM_MAX

/**
 * @typedef kpp_callback
 * @brief Callback function for KPP events.
 *
 *  If enabled, callback function will be invoked at kpp event occurred.
 *
 * @param dev  Pointer to the KPP device calling the callback.
 */
typedef void (*kpp_callback)(const struct device *dev);

/**
 * @brief KPP Event
 */
enum kpp_event {
	/** Keypad key dpress event */
	KPP_EVENT_KEY_DPRESS,
	/** Keypad key release event */
	KPP_EVENT_KEY_RELEASE,
};

/**
 * @brief Keypad press scanning.
 *
 * This function will scanning all columns and rows. so
 * all scanning data will be stored in the data pointer.
 *
 * @param dev  KPP device instance.
 * @param active_row  The row number: bit 7 ~ 0 represents the row 7 ~ 0.
 * @param active_column  The column number: bit 7 ~ 0 represents the column 7 ~ 0.
 * @param interrupt_event  KPP interrupt source. A logical OR of "kpp_interrupt_enable_t".
 */
void input_kpp_config(const struct device *dev, uint8_t active_row,
	uint8_t active_column,  enum kpp_event interrupt_event);

/**
 * @brief Keypad press scanning.
 *
 * This function will scanning all columns and rows. so
 * all scanning data will be stored in the data pointer.
 *
 * @param dev  KPP device instance.
 * @param data  KPP key press scanning data. The data buffer should be prepared with
 * length at least equal to KPP_KEYPAD_COLUMNNUM_MAX * KPP_KEYPAD_ROWNUM_MAX.
 * the data pointer is recommended to be a array like uint8_t data[KPP_KEYPAD_COLUMNNUM_MAX].
 * for example the data[2] = 4, that means in column 1 row 2 has a key press event.
 */
void input_kpp_scan(const struct device *dev, uint8_t *data);

/**
 * @brief KPP callback function setting.
 *
 * This function will set the callback function for KPP events.
 *
 * @param dev  KPP device instance.
 * @param cb   KPP callback function.
 */
void input_kpp_set_callback(const struct device *dev, kpp_callback cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_KPP_H_ */
