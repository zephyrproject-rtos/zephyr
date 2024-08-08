/*
 * Copyright (c) 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Openthread HDLC RCP communication Interface API
 *
 * An API for applications do HDLC requests
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
typedef void (*hdlc_rx_callback_t)(uint8_t *data, uint16_t len, void *param);


/** HDLC interface configuration data. */
struct hdlc_api {
	/**
	 * @brief network interface API
	 *
	 * @note Network devices must extend the network interface API. It is
	 * therefore mandatory to place it at the top of the interface API struct so
	 * that it can be cast to a network interface.
	 */
	struct net_if_api iface_api;

	/**
	 * @brief Register the HDLC RX callback.
	 *
	 * @details Upper layers will assume the interface is "UP" if this
	 * operation returns with zero.
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. MAY be
	 * called in any interface state once the interface is fully initialized
	 * ("ready").
	 *
	 * @param hdlc_rx_callback pointer to the spinel HDLC RX callback
	 * @param param pointer to the spinel HDLC interface
	 *
	 * @retval 0 The interface was successfully opened.
	 * @retval -EIO The interface could not be opened.
	 */
	int (*register_rx_cb)(hdlc_rx_callback_t hdlc_rx_callback, void *param);

	/**
	 * @brief Transmit a HDLC packet as a single frame
	 *
	 * @details It is the responsibility of L2 implementations to prepare
	 * the spinel HDLC frame according to the offloading capabilities
	 * announced by the interface.
	 *
	 * All the spinel HDLC frames originating from L2 SHALL have all
	 * required IEs pre-allocated and pre-filled such that the interface
	 * does not have to parse and manipulate IEs at all.
	 *
	 * @warning The interface SHALL NOT take ownership of the given spinel
	 * HDLC frame buffer. Any data required by the interface including the
	 * actual frame content must be read synchronously.
	 * The frame MAY be re-used or released by upper layers immediately
	 * after the function returns.
	 *
	 * @param aFrame pointer to the spinel HDLC frame to be transmitted.
	 *
	 * @retval 0 The frame was successfully sent.
	 * @retval -EIO The frame could not be sent due to some unspecified
	 * interface error (e.g. the interface being busy).
	 */
	int (*send)(const uint8_t *aFrame, uint16_t aLength);

	/**
	 * @brief Deinitialize the device.
	 *
	 * @details Upper layers will assume the interface is "DOWN" if this
	 * operation returns with zero. The interface switches off
	 * the receiver before returning if it was previously on. The interface
	 * enters the lowest possible power mode after this operation is called.
	 * This MAY happen asynchronously (i.e. after the operation already
	 * returned control).
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. MAY be
	 * called in any interface state once the interface is fully initialized
	 * ("ready").
	 *
	 * @param none
	 *
	 * @retval 0 The interface was successfully stopped.
	 * @retval -EIO The interface could not be stopped.
	 */
	int (*deinit)(void);
};

/* Make sure that the network interface API is properly setup inside
 * IEEE 802.15.4 interface API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct hdlc_api, iface_api) == 0);

#ifdef __cplusplus
}
#endif
