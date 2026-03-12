/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Descriptor matching and searching utilities
 *
 * This file contains utilities to browse USB descriptors returned by a device
 * according to the USB Specification 2.0 Chapter 9.
 */

#ifndef ZEPHYR_INCLUDE_USBH_DESC_H
#define ZEPHYR_INCLUDE_USBH_DESC_H

#include <stdint.h>

#include <zephyr/usb/usbh.h>

/**
 * @brief Get the next descriptor in an array of descriptors.
 *
 * @param[in] desc Pointer to the beginning of the descriptor array to search.
 *
 * @return A pointer to the descriptor
 */
const void *usbh_desc_get_next(const void *const desc);

/**
 * @brief Search an interface descriptor matching the interace number wanted.
 *
 * The descriptors following it can be browsed using @ref usbh_desc_get_next
 *
 * @param[in] udev USB device that contains the list of interfaces
 * @param[in] iface The interface number to search
 *
 * @return A pointer to the descriptor
 */
const void *usbh_desc_get_iface(const struct usb_device *const udev, const uint8_t iface);

/**
 * @brief Search an endpoint descriptor matching the endpoint number wanted.
 *
 * The descriptors following it can be browsed using @ref usbh_desc_get_next
 *
 * @param[in] udev USB device that contains the list of endpoints
 * @param[in] ep The endpoint number to search
 *
 * @return A pointer to the descriptor
 */
const void *usbh_desc_get_endpoint(const struct usb_device *const udev, const uint8_t ep);

/**
 * @brief Search an interface association descriptor matching the interace number wanted.
 *
 * The descriptors are going to be scanned until either an interface association
 * @c bFirstInterface field or an interface @c bInterfaceNumber field match.
 *
 * The descriptors following it can be browsed using @ref usbh_desc_get_next
 *
 * @param[in] udev USB device that contains the list of interface associations
 * @param[in] iface The interface number to search
 *
 * @return A pointer to the descriptor
 */
const void *usbh_desc_get_iad(const struct usb_device *const udev, const uint8_t iface);

/**
 * @brief Extract information from an interface or interface association descriptors
 *
 * @param[in] desc The descriptor to use
 * @param[out] filter Device information filled by this function
 * @param[out] iface_num Pointer filled with the interface number, or NULL
 *
 * @return 0 on success or negative error code on failure.
 */
int usbh_desc_fill_filter(const struct usb_desc_header *desc,
			  struct usbh_class_filter *const filter,
			  uint8_t *const iface_num);

/**
 * @brief Checks that the pointed descriptor is not truncated and of the expected type.
 *
 * @param[in] desc The descriptor to validate
 * @param[in] size The size of the descriptor.
 * @param[in] type The type of the descriptor, 0 for any.
 *
 * @return true if the descriptor size and type are correct
 * @return false if the descriptor size or type is wrong
 */
bool usbh_desc_is_valid(const void *const desc, const size_t size, const uint8_t type);

/**
 * @brief Checks that the pointed descriptor is an interface descriptor.
 *
 * @param[in] desc The descriptor to validate
 *
 * @return true if the descriptor size and type are correct
 * @return false if the descriptor size or type is wrong
 */
bool usbh_desc_is_valid_interface(const void *const desc);

/**
 * @brief Checks that the pointed descriptor is an interface association descriptor.
 *
 * @param[in] desc The descriptor to validate
 *
 * @return true if the descriptor size and type are correct
 * @return false if the descriptor size or type is wrong
 */
bool usbh_desc_is_valid_association(const void *const desc);

/**
 * @brief Checks that the pointed descriptor is an endpoint descriptor.
 *
 * @param[in] desc The descriptor to validate
 *
 * @return true if the descriptor size and type are correct
 * @return false if the descriptor size or type is wrong
 */
bool usbh_desc_is_valid_endpoint(const void *const desc);

/**
 * @brief Get the next function in the descriptor list.
 *
 * This searches the interface or interface association of the next USB
 * function. This can be used to walk through the list of USB functions
 * to associate a class to each.
 *
 * It will seek a pointer to the next interface with bAlternateSetting of 0 or next interface
 * association descriptor (IAD), and return it.
 *
 * @param[in] desc Pointer to the descriptor array to search.
 *
 * @retval Pointer to the next matching descriptor.
 * @retval NULL if no matching descriptor was found
 */
const void *usbh_desc_get_next_function(const void *const desc);

/**
 * @brief Get the next alternate setting in the current interface.
 *
 * The @p desc descriptor is expected to point at an interface descriptor, and
 * the descriptor returned will be different, with bAlternateSetting > 0 and
 * same bInterfaceNumber, or NULL if none was found or on invalid descriptor.
 *
 * @param[in] desc Pointer to the beginning of the descriptor array to search.
 *
 * @return Pointer to the next matching descriptor or NULL.
 */
const void *usbh_desc_get_next_alt_setting(const void *const desc);

#endif /* ZEPHYR_INCLUDE_USBH_DESC_H */
