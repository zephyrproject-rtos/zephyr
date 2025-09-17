/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the CDC ACM class driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_CDC_ACM_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_CDC_ACM_H_

#include <errno.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @typedef cdc_dte_rate_callback_t
 * @brief A function that is called when the USB host changes the baud
 * rate.
 *
 * @param dev Device struct for the CDC ACM device.
 * @param rate New USB baud rate
 */
typedef void (*cdc_dte_rate_callback_t)(const struct device *dev,
					uint32_t rate);

/**
 * @brief Set the callback for dwDTERate SetLineCoding requests.
 *
 * @deprecated Use @ref usbd_api and @ref USBD_MSG_CDC_ACM_LINE_CODING instead.
 *
 * The callback is invoked when the USB host changes the baud rate.
 *
 * @note This function is available only when
 * CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT is enabled.
 *
 * @param dev	    CDC ACM device structure.
 * @param callback  Event handler.
 *
 * @return	    0 on success.
 */
__deprecated int cdc_acm_dte_rate_callback_set(const struct device *dev,
				  cdc_dte_rate_callback_t callback);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_CDC_ACM_H_ */
