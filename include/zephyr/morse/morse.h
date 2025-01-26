/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Morse subsystem
 */

#ifndef ZEPHYR_INCLUDE_MORSE_MORSE_H_
#define ZEPHYR_INCLUDE_MORSE_MORSE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Morse Code
 * @defgroup subsys_morse Morse Code
 * @since 4.1
 * @ingroup os_services
 * @{
 */

enum morse_rx_state {
	MORSE_RX_STATE_END_LETTER,
	MORSE_RX_STATE_END_WORD,
	MORSE_RX_STATE_END_TRANSMISSION,
};

/**
 * @typedef morse_tx_callback_handler_t
 * @brief Define the tx callback handler function signature
 *
 * @param ctx    User defined data
 * @param status An errno status
 */
typedef void (morse_tx_callback_handler_t)(void *ctx,
					   int status);

/**
 * @typedef morse_rx_callback_handler_t
 * @brief Define the rx callback handler function signature
 *
 * @param ctx   User defined data
 * @param state An errno status
 * @param data  A character received
 */
typedef void (morse_rx_callback_handler_t)(void *ctx,
					   enum morse_rx_state state,
					   uint32_t data);

/**
 *  @brief Send data using morse code
 *
 * This translates the text in morse code and sends it over the medium. This
 * function is non-blocking and will return immediately. The manage callback
 * should be used to synchronize between FSM and process. The data is not
 * copied and should be kept valid until morse_tx_callback_handler_t is
 * received.
 *
 *  @param dev  Pointer to device structure for driver instance.
 *  @param data the ASCII text to send
 *  @param size the length of the text in bytes. The 0 value question the
 *		driver  about processing queue.
 *
 * @return positive number to indicate missing bytes to transmit
 * @return 0 when device is free
 * @return -EINVAL when some argument is invalid
 * @return -EBUSY when driver is transmitting
 * @return -ENOENT when symbol to be transmitted is not a valid morse code
 */
int morse_send(const struct device *dev,
	       const uint8_t *data,
	       const size_t size);

/**
 *  @brief Configure a callback to be invoked when transmission finishes
 *
 *  @param dev      Pointer to device structure for driver instance.
 *  @param tx_cb    The user defined tx callback to be invoked. When tx_cb is
 *		    NULL the callback will be disabled.
 *  @param rx_cb    The user defined rx callback to be invoked. When rx_cb is
 *		    NULL the callback will be disabled and user can not receive
 *		    any data.
 *  @param user_ctx A user defined context. This parameter is optional and it
 *		    is not checked.
 *
 * @return 0 on success
 * @return -EINVAL when dev is NULL
 */
int morse_manage_callbacks(const struct device *dev,
			   morse_tx_callback_handler_t *tx_cb,
			   morse_rx_callback_handler_t *rx_cb,
			   void *user_ctx);

/**
 *  @brief Configure morse code parameters
 *
 *  @param dev   Pointer to device structure for driver instance.
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
int morse_set_config(const struct device *dev,
		     uint16_t speed);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MORSE_MORSE_H_ */
