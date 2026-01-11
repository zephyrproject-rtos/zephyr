/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX public API
 * @ingroup ft8xx_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for FTDI FT8xx graphic controller.
 * @defgroup ft8xx_interface FTDI FT8xx
 * @ingroup misc_interfaces
 * @{
 */

/**
 * @struct ft8xx_touch_transform
 * @brief Structure holding touchscreen calibration data
 *
 * The content of this structure is filled by ft8xx_calibrate().
 */
struct ft8xx_touch_transform {
	uint32_t a; /*< Coefficient A of the bitmap transform matrix */
	uint32_t b; /*< Coefficient B of the bitmap transform matrix */
	uint32_t c; /*< Coefficient C of the bitmap transform matrix */
	uint32_t d; /*< Coefficient D of the bitmap transform matrix */
	uint32_t e; /*< Coefficient E of the bitmap transform matrix */
	uint32_t f; /*< Coefficient F of the bitmap transform matrix */
};

/**
 * @typedef ft8xx_int_callback
 * @brief Callback API to inform API user that FT8xx triggered interrupt
 *
 * This callback is called from IRQ context.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param user_data Pointer to user data provided during callback registration
 */
typedef void (*ft8xx_int_callback)(const struct device *dev, void *user_data);

/**
 * @brief Calibrate touchscreen
 *
 * Run touchscreen calibration procedure that collects three touches from touch
 * screen. Computed calibration result is automatically applied to the
 * touchscreen processing and returned with @p data.
 *
 * The content of @p data may be stored and used after reset in
 * ft8xx_touch_transform_set() to avoid calibrating touchscreen after each
 * device reset.
 *
 * @param dev  Pointer to the device structure for the driver instance
 * @param data Pointer to touchscreen transform structure to populate
 */
void ft8xx_calibrate(const struct device *dev,
		     struct ft8xx_touch_transform *data);

/**
 * @brief Set touchscreen calibration data
 *
 * Apply given touchscreen transform data to the touchscreen processing.
 * Data is to be obtained from calibration procedure started with
 * ft8xx_calibrate().
 *
 * @param dev  Pointer to the device structure for the driver instance
 * @param data Pointer to touchscreen transform structure to apply
 */
void ft8xx_touch_transform_set(const struct device *dev,
			       const struct ft8xx_touch_transform *data);

/**
 * @brief Get tag of recently touched element
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return Tag value 0-255 of recently touched element
 */
int ft8xx_get_touch_tag(const struct device *dev);

/**
 * @brief Set callback executed when FT8xx triggers interrupt
 *
 * This function configures FT8xx to trigger interrupt when touch event changes
 * tag value.
 *
 * When touch event changes tag value, FT8xx activates INT line. The line is
 * kept active until ft8xx_get_touch_tag() is called. It results in single
 * execution of @p callback until ft8xx_get_touch_tag() is called.
 *
 * @param dev       Pointer to the device structure for the driver instance
 * @param callback  Pointer to function called when FT8xx triggers interrupt
 * @param user_data Pointer to user data to be passed to the @p callback
 */
void ft8xx_register_int(const struct device *dev,
			ft8xx_int_callback callback,
			void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_ */
