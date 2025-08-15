/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
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
 * This
 * to search either an interface or  interface association descriptor:
 *
 * @code{.c}
 * BIT(USB_DESC_INTERFACE) | BIT(USB_DESC_INTERFACE_ASSOC)
 * @endcode
 *
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search
 * @param[in] type_mask Bitmask of bDescriptorType values to match
 * @return A pointer to the first descriptor matching
 */
const void *usbh_desc_get_next(const void *const desc_beg, const void *const desc_end);

/**
 * @brief Search the first descriptor matching the selected type(s).
 *
 * As an example, the @p type_mask argument can be constructed this way
 * to search either an interface or  interface association descriptor:
 *
 * @code{.c}
 * BIT(USB_DESC_INTERFACE) | BIT(USB_DESC_INTERFACE_ASSOC)
 * @endcode
 *
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search
 * @param[in] type_mask Bitmask of bDescriptorType values to match
 * @return A pointer to the first descriptor matching
 */
const struct usb_desc_header *usbh_desc_get_by_type(const void *const desc_beg,
						    const void *const desc_end,
						    uint32_t type_mask);

/**
 * @brief Search a descriptor matching the interace number wanted.
 *
 * The descriptors are going to be scanned until either an interface association
 * @c bFirstInterface field or an interface @c bInterfaceNumber field match.
 *
 * The descriptors following it can be browsed using @ref usbh_desc_get_next
 *
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search
 * @param[in] ifnum The interface number to search
 * @return A pointer to the first descriptor matching
 */
const void *usbh_desc_get_by_ifnum(const void *const desc_beg, const void *const desc_end,
				   const uint8_t ifnum);

/**
 * @brief Get the start of a device's configuration descriptor.
 *
 * @param[in] udev The device holding the confiugration descriptor
 * @return A pointer to the beginning of the descriptor
 */
const void *usbh_desc_get_cfg_beg(const struct usb_device *udev);

/**
 * @brief Get the end of a device's configuration descriptor.
 *
 * @param[in] udev The device holding the confiugration descriptor
 * @return A pointer to the beginning of the descriptor
 */
const void *usbh_desc_get_cfg_end(const struct usb_device *udev);

/**
 * @brief Get class code and other information from the device descriptors
 *
 * This information is obtained from the device configuration descriptor if
 * @c bDeviceClass is non-zero, otherwise, the specified interface (or interface association
 * descriptor if present) is used as source for the information.
 *
 * @param[out] device_info Device information filled by this function
 * @param[in] udev Device instance whose descriptors are read
 * @param[in] ifnum Interface number or interface association number to use
 */
const int usbh_desc_get_device_info(struct usbh_class_filter *const device_info,
				    const struct usb_device *const udev, const uint8_t ifnum);

/**
 * Checks that the pointed descriptor is not truncated.
 *
 * @param[in] desc The descriptor
 */
const bool usbh_desc_size_is_valid(const void *const desc, size_t expected_size,
				   const void *const desc_end);

#endif /* ZEPHYR_INCLUDE_USBH_DESC_H */
