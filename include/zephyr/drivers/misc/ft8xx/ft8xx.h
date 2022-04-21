/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX public API
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx driver public APIs
 * @defgroup ft8xx_interface FT8xx driver APIs
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
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
};

/**
 * @typedef ft8xx_int_callback
 * @brief Callback API to inform API user that FT8xx triggered interrupt
 *
 * This callback is called from IRQ context.
 */
typedef void (*ft8xx_int_callback)(void);

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
 * @param data Pointer to touchscreen transform structure to populate
 */
void ft8xx_calibrate(struct ft8xx_touch_transform *data);

/**
 * @brief Set touchscreen calibration data
 *
 * Apply given touchscreen transform data to the touchscreen processing.
 * Data is to be obtained from calibration procedure started with
 * ft8xx_calibrate().
 *
 * @param data Pointer to touchscreen transform structure to apply
 */
void ft8xx_touch_transform_set(const struct ft8xx_touch_transform *data);

/**
 * @brief Get tag of recently touched element
 *
 * @return Tag value 0-255 of recently touched element
 */
int ft8xx_get_touch_tag(void);

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
 * @param callback Pointer to function called when FT8xx triggers interrupt
 */
void ft8xx_register_int(ft8xx_int_callback callback);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_ */
