/*
 * Copyright (c) 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs of HDLC RCP communication Interface
 *
 * This file provide the HDLC APIs to be used by an RCP host
 */

#include <zephyr/sys/util.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OT RCP HDLC RX callback function.
 *
 * @note This function is called in the radio spinel HDLC level
 */
typedef void (*hdlc_rx_callback_t)(const uint8_t *data, uint16_t len, void *param);

/** HDLC interface configuration data. */
struct hdlc_api {
	/**
	 * @brief HDLC interface API
	 */
	struct net_if_api iface_api;

	/**
	 * @brief Register the Spinel HDLC RX callback.
	 *
	 * @param hdlc_rx_callback pointer to the spinel HDLC RX callback
	 * @param param pointer to the spinel HDLC interface
	 *
	 * @retval 0 The callback was successfully registered.
	 * @retval -EIO The callback could not be registered.
	 */
	int (*register_rx_cb)(hdlc_rx_callback_t hdlc_rx_callback, void *param);

	/**
	 * @brief Transmit a HDLC frame
	 *
	 *
	 * @param frame pointer to the HDLC frame to be transmitted.
	 * @param length length of the HDLC frame to be transmitted.

	 * @retval 0 The frame was successfully sent.
	 * @retval -EIO The frame could not be sent due to some unspecified
	 * interface error (e.g. the interface being busy).
	 */
	int (*send)(const uint8_t *frame, uint16_t length);

	/**
	 * @brief Deinitialize the device.
	 *
	 * @param none
	 *
	 * @retval 0 The interface was successfully stopped.
	 * @retval -EIO The interface could not be stopped.
	 */
	int (*deinit)(void);
};

/* Make sure that the interface API is properly setup inside
 * HDLC interface API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct hdlc_api, iface_api) == 0);

#ifdef __cplusplus
}
#endif
