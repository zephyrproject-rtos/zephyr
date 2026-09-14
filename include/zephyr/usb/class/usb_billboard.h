/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Device/host common USB Billboard header file
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_BILLBOARD_H
#define ZEPHYR_INCLUDE_USB_CLASS_USB_BILLBOARD_H

#include <stdint.h>
#include <zephyr/sys/util.h>

/**
 * @brief AUM part of Billboard capability descriptor
 */
struct usb_billboard_aum {
	/** Alternate/USB4 mode's SVID. */
	uint16_t wSVID;
	/** Alternate/USB4 mode index within the SVID. */
	uint8_t bAlternateOrUSB4Mode;
	/** Index of string descriptor describing the alternate/USB4 mode. */
	uint8_t iAlternateOrUSB4ModeString;
} __packed;

/**
 * @brief Billboard capability descriptor with flexible AUM array
 */
struct usb_billboard_capability_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. Must be set to @ref USB_DESC_DEVICE_CAPABILITY. */
	uint8_t bDescriptorType;
	/** Device capability type. Must be @ref USB_BOS_CAPABILITY_BILLBOARD. */
	uint8_t bDevCapabilityType;
	/** Index of string descriptor for the additional info URL. */
	uint8_t iAdditionalInfoURL;
	/** Number of alternate/USB4 modes described by the AUM array. */
	uint8_t bNumberOfAlternateOrUSB4Modes;
	/** Index of the preferred alternate/USB4 mode in the AUM array. */
	uint8_t bPreferredAlternateOrUSB4Mode;
	/** VCONN power information. */
	uint16_t VCONNPower;
	/** Bitmap of the configured state of each alternate/USB4 mode. */
	uint8_t bmConfigured[32];
	/** Billboard capability descriptor version in BCD format. */
	uint16_t bcdVersion;
	/** Additional failure info of the preferred alternate/USB4 mode. */
	uint8_t bAdditionalFailureInfo;
	/** Reserved (must be zero). */
	uint8_t bReserved;
	/** Flexible array of AUM descriptors. */
	FLEXIBLE_ARRAY_DECLARE(struct usb_billboard_aum, aum);
} __packed;

/**
 * @brief AUM capability descriptor
 */
struct usb_aum_capability_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. Must be set to @ref USB_DESC_DEVICE_CAPABILITY. */
	uint8_t bDescriptorType;
	/** Device capability type. Must be @ref USB_BOS_CAPABILITY_BILLBOARD_EX. */
	uint8_t bDevCapabilityType;
	/** Index into the AUM array of the described alternate/USB4 mode. */
	uint8_t bIndex;
	/** Alternate mode VDO of the described mode. */
	uint32_t dwAlternateModeVdo;
} __packed;

/** Billboard device class subclass code. */
#define USB_BILLBOARD_SUBCLASS 0x00
/** Billboard device class protocol code. */
#define USB_BILLBOARD_RUNTIME  0x00

/** @brief VCONN power needed by an alternate/USB4 mode */
enum usb_billboard_vconn_needed {
	USB_BILLBOARD_VCONN_NEEDED_1W = 0,       /**< 1 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_1W5 = 1,      /**< 1.5 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_2W = 2,       /**< 2 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_3W = 3,       /**< 3 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_4W = 4,       /**< 4 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_5W = 5,       /**< 5 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_6W = 6,       /**< 6 W of VCONN power needed. */
	USB_BILLBOARD_VCONN_NEEDED_RESERVED = 7, /**< Reserved value. */
};

/**
 * @name VCONNPower field accessors
 * @{
 */
/**
 * @brief Get the VCONN power needed value from a VCONNPower field
 *
 * @param vconn_field VCONNPower field value
 *
 * @return VCONN power needed, one of @ref usb_billboard_vconn_needed.
 */
#define USB_BILLBOARD_VCONN_GET_NEEDED(vconn_field) ((vconn_field) & 0x7)

/** Bit indicating that no VCONN power is needed. */
#define USB_BILLBOARD_VCONN_NOT_NEEDED_MASK (1U << 15)

/**
 * @brief Check whether a VCONNPower field indicates no VCONN power is needed
 *
 * @param vconn_field VCONNPower field value
 *
 * @return Non-zero if no VCONN power is needed, 0 otherwise.
 */
#define USBBILLBOARD__VCONN_NOT_NEEDED(vconn_field)                                                \
	(!!((vconn_field) & (USB_BILLBOARD_VCONN_NOT_NEEDED_MASK)))
/** @} */

/** @brief Configuration/failure state of an alternate/USB4 mode */
enum usb_billboard_aum_state {
	USB_BILLBOARD_AUM_STATE_ERROR = 0,                /**< Mode failed to configure. */
	USB_BILLBOARD_AUM_STATE_NOT_ATTEMPTED = 1,        /**< Mode was not attempted. */
	USB_BILLBOARD_AUM_STATE_NOT_ATTEMPTED_FAILED = 2, /**< Attempted mode failed. */
	USB_BILLBOARD_AUM_STATE_SUCCESSFUL = 3,           /**< Mode configured successfully. */
};

/**
 * @name bmConfigured field accessors
 * @{
 */
/**
 * @brief Get the bmConfigured byte index for an alternate/USB4 mode
 *
 * @param alt_idx Index of the alternate/USB4 mode in the AUM array.
 *
 * @return Index into @ref usb_billboard_capability_descriptor.bmConfigured.
 */
#define USB_BILLBOARD_GET_AUM_INDEX(alt_idx) ((alt_idx) / 4)

/**
 * @brief Extract an alternate/USB4 mode's state from a bmConfigured byte
 *
 * @param aum_elem bmConfigured byte containing the mode's state.
 * @param alt_idx  Index of the alternate/USB4 mode in the AUM array.
 *
 * @return Mode state, one of @ref usb_billboard_aum_state.
 */
#define USB_BILLBOARD_GET_AUM_BITPOS(aum_elem, alt_idx)                                            \
	(((aum_elem) >> (((alt_idx) % 4) * 2)) & 0x3)

/**
 * @brief Get an alternate/USB4 mode's state from the bmConfigured array
 *
 * @param bmConfigured @ref usb_billboard_capability_descriptor.bmConfigured array.
 * @param alt_idx       Index of the alternate/USB4 mode in the AUM array.
 *
 * @return Mode state, one of @ref usb_billboard_aum_state.
 */
#define USB_BILLBOARD_GET_AUM(bmConfigured, alt_idx)                                               \
	USB_BILLBOARD_GET_AUM_BITPOS((bmConfigured)[USB_BILLBOARD_GET_AUM_INDEX(alt_idx)], alt_idx)
/** @} */

#endif
