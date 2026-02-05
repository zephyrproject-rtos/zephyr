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
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search
 *
 * @return A pointer to the first descriptor matching
 */
const void *usbh_desc_get_next(const void *const desc_beg, const void *const desc_end);

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
 * @param[in] iface The interface number to search
 *
 * @return A pointer to the first descriptor matching
 */
const void *usbh_desc_get_by_iface(const void *const desc_beg, const void *const desc_end,
				   const uint8_t iface);

/**
 * @brief Get the start of a device's configuration descriptor.
 *
 * @param[in] udev The device holding the configuration descriptor
 *
 * @return A pointer to the beginning of the descriptor
 */
const void *usbh_desc_get_cfg_beg(const struct usb_device *const udev);

/**
 * @brief Get the end of a device's configuration descriptor.
 *
 * @param[in] udev The device holding the configuration descriptor
 *
 * @return A pointer to the beginning of the descriptor
 */
const void *usbh_desc_get_cfg_end(const struct usb_device *const udev);

/**
 * @brief Extract information from an interface or interface association descriptors
 *
 * @param[in] desc The descriptor to use
 * @param[out] iface_code Device information filled by this function
 * @param[out] iface_num Pointer filled with the interface number, or NULL
 *
 * @return 0 on success or negative error code on failure.
 */
int usbh_desc_get_iface_info(const struct usb_desc_header *desc,
			     struct usbh_class_filter *const iface_code,
			     uint8_t *const iface_num);

/**
 * @brief Checks that the pointed descriptor is not truncated.
 *
 * @param[in] desc The descriptor to validate
 * @param[in] desc_end Pointer after the end of the descriptor array to search.
 * @param[in] expected_size The size of the descriptor.
 *
 * @return true if the descriptor size is valid
 */
const bool usbh_desc_is_valid(const void *const desc, const void *const desc_end,
			      size_t expected_size);

/**
 * @brief Checks that the pointed descriptor is a valid interface descriptor.
 *
 * @param[in] desc The descriptor to validate
 * @param[in] desc_end Pointer after the end of the descriptor array to search.
 *
 * @return true if the descriptor size is the expected type and size
 */
bool usbh_desc_is_valid_interface(const void *const desc, const void *const desc_end);

/**
 * @brief Checks that the pointed descriptor is a valid interface descriptor.
 *
 * @param[in] desc The descriptor to validate
 * @param[in] desc_end Pointer after the end of the descriptor array to search.
 *
 * @return true if the descriptor size is the expected type and size
 */
bool usbh_desc_is_valid_association(const void *const desc, const void *const desc_end);

/**
 * @brief Get the next function in the descriptor list.
 *
 * This returns the interface or interface association of the next USB function, if found.
 * This can be used to walk through the list of USB functions to associate a class to each.
 *
 * If @p desc_beg is an interface association descriptor, it will return a pointer to the interface
 * after all those related to the association descriptor.
 *
 * If @p desc_beg is an interface descriptor, it will skip all the interface alternate settings
 * and return a pointer to the next interface with bAlternateSetting of 0.
 *
 * If @p desc_beg is another type of descriptors, it will seek to a matching descriptor type and
 * return it, without skipping to the next one after it.
 *
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search.
 *
 * @return Pointer to the next matching descriptor or NULL.
 */
const void *usbh_desc_get_next_function(const void *const desc_beg, const void *const desc_end);

/**
 * @brief Get the next alternate setting in the current interface.
 *
 * The @p desc_beg descriptor is expected to point at an interface descriptor, and the descriptor
 * returned  will be different, with bAlternateSetting > 0 and same bInterfaceNumber, or NULL if
 * none was found or on invalid descriptor.
 *
 * @param[in] desc_beg Pointer to the beginning of the descriptor array; to search. May be NULL.
 * @param[in] desc_end Pointer after the end of the descriptor array to search.
 *
 * @return Pointer to the next matching descriptor or NULL.
 */
const void *usbh_desc_get_next_alt_setting(const void *const desc_beg, const void *const desc_end);

#endif /* ZEPHYR_INCLUDE_USBH_DESC_H */
