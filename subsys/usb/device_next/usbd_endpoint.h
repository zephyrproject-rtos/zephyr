/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_ENDPOINT_H
#define ZEPHYR_INCLUDE_USBD_ENDPOINT_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Get the endpoint bitmap bit associated with an endpoint address
 *
 * OUT endpoints are mapped to bits 0 to 15, IN endpoints to bits 16 to 31.
 *
 * @param[in] ep  Endpoint address
 *
 * @return Bit position in an endpoint bitmap
 */
static inline unsigned int usbd_ep_bm_bit(const uint8_t ep)
{
	return USB_EP_GET_IDX(ep) + (USB_EP_DIR_IS_IN(ep) ? 16U : 0U);
}

/**
 * @brief Set bit associated with the endpoint
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 */
static inline void usbd_ep_bm_set(uint32_t *const ep_bm, const uint8_t ep)
{
	*ep_bm |= BIT(usbd_ep_bm_bit(ep));
}

/**
 * @brief Clear bit associated with the endpoint
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 */
static inline void usbd_ep_bm_clear(uint32_t *const ep_bm, const uint8_t ep)
{
	*ep_bm &= ~BIT(usbd_ep_bm_bit(ep));
}

/**
 * @brief Check whether bit associated with the endpoint is set
 *
 * @param[in] ep_bm  Pointer to endpoint bitmap
 * @param[in] ep     Endpoint address
 *
 * @return true if bit is set, false otherwise
 */
static inline bool usbd_ep_bm_is_set(const uint32_t *const ep_bm, const uint8_t ep)
{
	return (*ep_bm & BIT(usbd_ep_bm_bit(ep))) ? true : false;
}

/**
 * @brief Get the address of the lowest numbered endpoint in a bitmap
 *
 * @param[in] ep_bm  Endpoint bitmap, must not be zero
 *
 * @return Endpoint address
 */
static inline uint8_t usbd_ep_bm_get_first(const uint32_t ep_bm)
{
	const unsigned int bit = find_lsb_set(ep_bm) - 1U;

	if (bit >= 16U) {
		return USB_EP_DIR_IN | (bit - 16U);
	}

	return bit;
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
