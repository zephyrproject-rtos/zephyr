/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Coryright 2025 - 2026 NXP
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
			  struct usbh_class_filter *const filter, uint8_t *const iface_num);

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
 * @brief Checks that the pointed descriptor is an string descriptor.
 *
 * @param[in] desc The descriptor to validate
 *
 * @return true if the descriptor size and type are correct
 * @return false if the descriptor size or type is wrong
 */
bool usbh_desc_is_valid_string(const void *const desc);

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

/**
 * @brief Get the supported LANGIDs from the device's string descriptor zero.
 *
 * Retrieves the list of language IDs supported by the USB device by reading
 * string descriptor index 0.
 *
 * @param[in] udev Pointer to the USB device.
 * @param[out] lang_ids Array to store the supported LANGIDs.
 * @param[in] max_langs Maximum number of LANGIDs that can be stored in the array.
 *
 * @return Number of supported LANGIDs on success, or negative error code on failure:
 *         -ENOMEM if memory allocation failed
 *         -EBADMSG if the descriptor is invalid
 *         -ENOTSUP if the number of languages exceeds INT_MAX
 */
int usbh_desc_get_supported_langs(struct usb_device *const udev, uint16_t *const lang_ids,
				  size_t max_langs);

/**
 * @brief Convert UTF-16LE USB string descriptor to ASCII string.
 *
 * Converts the UTF-16LE encoded string from a USB string descriptor to an ASCII
 * string. Non-ASCII characters (code points > 0x7F) are replaced with '?'.
 * The output string is always null-terminated.
 *
 * @param[in] desc_buf Pointer to the buffer containing the string descriptor.
 * @param[out] str Buffer to store the converted ASCII string.
 * @param[in] len Maximum length of the output buffer (including null terminator).
 *
 * @return Length of the converted ASCII string (excluding null terminator) on success,
 *         or negative error code on failure:
 *         -EINVAL if parameters are invalid or descriptor is malformed
 *         -ENOTSUP if the string contains non-ASCII characters (conversion still performed)
 */
int usbh_desc_str_utfle16_to_ascii(const struct net_buf *const desc_buf, char *const str,
				   size_t len);

#endif /* ZEPHYR_INCLUDE_USBH_DESC_H */
