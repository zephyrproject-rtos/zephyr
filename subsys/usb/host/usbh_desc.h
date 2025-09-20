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
 * @brief Search the first descriptor matching the selected type(s).
 *
 * @param[in] start_addr Pointer to the beginning of the descriptor array; to search
 * @param[in] end_addr Pointer after the end of the descriptor array to search
 * @param[in] type_mask Bitmask of bDescriptorType values to match
 *
 * @return A pointer to the first descriptor matching
 */
struct usb_desc_header *usbh_desc_get_by_type(const uint8_t *const start_addr,
					      const uint8_t *const end_addr,
					      uint32_t type_mask);

#endif /* ZEPHYR_INCLUDE_USBH_DESC_H */
