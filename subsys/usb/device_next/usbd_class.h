/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_CLASS_H
#define ZEPHYR_INCLUDE_USBD_CLASS_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Handle endpoint transfer result
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] buf     Pointer to UDC request buffer
 * @param[in] err     Transfer status
 *
 * @return usbd_class_request() return value
 */
int usbd_class_handle_xfer(struct usbd_contex *const uds_ctx,
			   struct net_buf *const buf,
			   const int err);

/**
 * @brief Calculate the length of the class function descriptor
 *
 * Calculate the length of the class instance function descriptor.
 * Calculated length does not include any string descriptors that may be
 * used by the class instance.
 *
 * @param[in] node Pointer to a class node
 * @param[in] speed Speed-dependent descriptor selector
 *
 * @return Length of the class descriptor
 */
size_t usbd_class_desc_len(struct usbd_class_node *const c_nd,
			   const enum usbd_speed speed);

/**
 * @brief Get class context by bInterfaceNumber value
 *
 * The function searches the class instance list for the interface number.
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] inum    Interface number
 *
 * @return Class iter pointer or NULL
 */
struct usbd_class_iter *usbd_class_get_by_iface(struct usbd_contex *uds_ctx,
						uint8_t i_n);

/**
 * @brief Get class context by configuration and interface number
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] speed   Speed the configuration number refers to
 * @param[in] cnum    Configuration number
 * @param[in] inum    Interface number
 *
 * @return Class iter pointer or NULL
 */
struct usbd_class_iter *usbd_class_get_by_config(struct usbd_contex *uds_ctx,
						 const enum usbd_speed speed,
						 uint8_t cnum,
						 uint8_t inum);

/**
 * @brief Get class context by endpoint address
 *
 * The function searches the class instance list for the endpoint address.
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] ep      Endpoint address
 *
 * @return Class iter pointer or NULL
 */
struct usbd_class_iter *usbd_class_get_by_ep(struct usbd_contex *uds_ctx,
					     uint8_t ep);

/**
 * @brief Get class context by request (bRequest)
 *
 * The function searches the class instance list and
 * compares the vendor request table with request value.
 * The function is only used if the request type is Vendor and
 * request recipient is Device.
 * Accordingly only the first class instance can be found.
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] request bRequest value
 *
 * @return Class iter pointer or NULL
 */
struct usbd_class_iter *usbd_class_get_by_req(struct usbd_contex *uds_ctx,
					      uint8_t request);

/**
 * @brief Remove all registered class instances from a configuration
 *
 * @param[in] uds_ctx Pointer to device context
 * @param[in] speed   Speed the configuration number applies to
 * @param[in] cfg     Configuration number (bConfigurationValue)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_class_remove_all(struct usbd_contex *const uds_ctx,
			  const enum usbd_speed speed,
			  const uint8_t cfg);

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_H */
