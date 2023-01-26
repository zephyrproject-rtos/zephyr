/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_ENDPOINT_H
#define ZEPHYR_INCLUDE_USBD_ENDPOINT_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Set bit associated with the endpoint
 *
 * The IN endpoints are mapped in the upper nibble.
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 */
static inline void usbd_ep_bm_set(uint32_t *const ep_bm, const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		*ep_bm |= BIT(USB_EP_GET_IDX(ep) + 16U);
	} else {
		*ep_bm |= BIT(USB_EP_GET_IDX(ep));
	}
}

/**
 * @brief Clear bit associated with the endpoint
 *
 * The IN endpoints are mapped in the upper nibble.
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 */
static inline void usbd_ep_bm_clear(uint32_t *const ep_bm, const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		*ep_bm &= ~BIT(USB_EP_GET_IDX(ep) + 16U);
	} else {
		*ep_bm &= ~BIT(USB_EP_GET_IDX(ep));
	}
}

/**
 * @brief Check whether bit associated with the endpoint is set
 *
 * The IN endpoints are mapped in the upper nibble.
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 *
 * @return true if bit is set, false otherwise
 */
static inline bool usbd_ep_bm_is_set(const uint32_t *const ep_bm, const uint8_t ep)
{
	unsigned int bit;

	if (USB_EP_DIR_IS_IN(ep)) {
		bit = USB_EP_GET_IDX(ep) + 16U;
	} else {
		bit = USB_EP_GET_IDX(ep);
	}

	return (*ep_bm & BIT(bit)) ? true : false;
}

/**
 * @brief Enable endpoint
 *
 * This function enables endpoint and sets corresponding bit.
 *
 * @param[in] dev    Pointer to UDC device
 * @param[in] ed     Pointer to endpoint descriptor
 * @param[in] ep_bm  Pointer to endpoint bitmap
 *
 * @return 0 on success, other values on fail.
 */
int usbd_ep_enable(const struct device *dev,
		   const struct usb_ep_descriptor *const ed,
		   uint32_t *const ep_bm);

/**
 * @brief Disable endpoint
 *
 * This function disables endpoint and clears corresponding bit.
 *
 * @param[in] dev    Pointer to UDC device
 * @param[in] ep     Endpoint address
 * @param[in] ep_bm  Pointer to endpoint bitmap
 *
 * @return 0 on success, other values on fail.
 */
int usbd_ep_disable(const struct device *dev,
		    const uint8_t ep,
		    uint32_t *const ep_bm);

#endif /* ZEPHYR_INCLUDE_USBD_ENDPOINT_H */
