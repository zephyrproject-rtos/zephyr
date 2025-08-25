/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_COMMON_UVC_H
#define ZEPHYR_INCLUDE_USB_COMMON_UVC_H

#include <stdint.h>

/**
 * @brief Type of value used by the USB protocol for this control.
 */
enum uvc_control_type {
	/** Signed integer control type */
	UVC_CONTROL_SIGNED,
	/** Unsigned integer control type */
	UVC_CONTROL_UNSIGNED,
};

/**
 * @brief Mapping between UVC controls and Video controls
 */
struct uvc_control_map {
	/* Video CID to use for this control */
	uint32_t cid;
	/* Size to write out */
	uint8_t size;
	/* Bit position in the UVC control */
	uint8_t bit;
	/* UVC selector identifying this control */
	uint8_t selector;
	/* Whether the UVC value is signed, always false for bitmaps and boolean */
	enum uvc_control_type type;
};

/**
 * @brief Mapping between UVC GUIDs and standard FourCC.
 */
struct uvc_guid_quirk {
	/* A Video API format identifier, for which the UVC format GUID is not standard. */
	uint32_t fourcc;
	/* GUIDs are 16-bytes long, with the first four bytes being the Four Character Code of the
	 * format and the rest constant, except for some exceptions listed in this table.
	 */
	uint8_t guid[16];
};

/**
 * @brief Get a conversion table for a given control unit type
 *
 * The mappings contains information about how UVC control structures are related to
 * video control structures.
 *
 * @param subtype The field bDescriptorSubType of a descriptor of type USB_DESC_CS_INTERFACE.
 * @param map Filled with a pointer to the conversion table
 * @param length Filled with the number of elements in that conversion table.
 *
 * @return 0 on success, negative code on error.
 */
int uvc_get_control_map(uint8_t subtype, const struct uvc_control_map **map, size_t *length);

/**
 * @brief Convert a standard FourCC to an equivalent UVC GUID.
 *
 * @param guid Array to a GUID, filled in binary format
 * @param fourcc Four character code
 */
void uvc_fourcc_to_guid(uint8_t guid[16], const uint32_t fourcc);

/**
 * @brief Convert an UVC GUID to a standard FourCC
 *
 * @param guid GUID, to convert
 *
 * @return Four Character Code
 */
uint32_t uvc_guid_to_fourcc(const uint8_t guid[16]);

#endif /* ZEPHYR_INCLUDE_USB_COMMON_UVC_H */
