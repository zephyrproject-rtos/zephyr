/* morse_code.h - Public API for the Morse Code driver */
/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 * Copyright (c) 2024 OS Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_MORSE_CODE_H_
#define ZEPHYR_INCLUDE_MORSE_CODE_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Morse Code APIs
 * @defgroup morse_code Morse code APIs
 * @ingroup misc_interfaces
 * @{
 */

/**
 *  @brief Send data using morse code
 *
 * This translates the text in morse code and sends it over a GPIO pin. This
 * function is non-blocking and will return immediately. The manage callback
 * should be used to synchronize between FSM and process.
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param data the ASCII text to send
 *  @param size the length of the text in bytes. The 0 value question the driver
 *  about processing queue.
 *
 * @return positive number to indicate missing bytes to transmit
 * @return 0 when device is free
 * @return -EINVAL when some argument is invalid
 * @return -EBUSY when driver is transmitting
 * @return -ENOENT when symbol to be transmitted is not a valid morse code
 */
int morse_code_send(const struct device *dev, const uint8_t *data,
		    const size_t size);

/**
 * @typedef morse_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param ctx User defined data
 * @param status An errno status
 */
typedef void (morse_callback_handler_t)(void *ctx, int status);

/**
 *  @brief Configure a callback to be invoked when transmission finishes
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param cb The user defined callback to be invoked. When cb is NULL the
 *	      callback will be disabled.
 *  @param ctx A user defined context. This parameter is optional and it is
 *	      not checked.
 *
 * @return 0 on success
 * @return -EINVAL when dev is NULL
 */
int morse_code_manage_callback(const struct device *dev,
			       morse_callback_handler_t *cb, void *ctx);

/**
 *  @brief Configure morse code parameters
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param speed Words Per Minute (wps) transmission rate.
 *		 This is defined using the standard word "PARIS".
 *		 In the default case means that "20 wpm" send the word "PARIS"
 *		 (or precisely "PARIS ") is sent 20 times in a minute.
 *
 * @return 0 on success
 * @return -EINVAL when some argument is invalid
 * @return -EBUSY when driver is transmitting
 * @return -ETIME depending how timer is configured
 */
int morse_code_set_config(const struct device *dev, uint16_t speed);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MORSE_CODE_H_ */
