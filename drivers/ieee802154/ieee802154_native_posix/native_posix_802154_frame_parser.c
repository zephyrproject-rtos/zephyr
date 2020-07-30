/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements frame parsing utilities for 802.15.4 radio driver.
 *
 * @note This module is based on following assumptions:
 *       a. All received frames contain both source and destination address.
 *       b. All received frames contain destination PAN ID field.
 *       Frames that don't meet these assumptions are dropped.
 */

#include "native_posix_802154_frame_parser.h"

#include <stdlib.h>

#include "native_posix_802154_const.h"

/******************************************************************************
 * @section Helper functions
 *****************************************************************************/

/* Version */
static uint8_t frame_version_get(const uint8_t *p_frame)
{
	return p_frame[FRAME_VERSION_OFFSET] & FRAME_VERSION_MASK;
}

/* Addressing */

static uint8_t addressing_offset_get(const uint8_t *p_frame)
{
	if ((frame_version_get(p_frame) >= FRAME_VERSION_2) &&
	    nrf_802154_frame_parser_dsn_suppress_bit_is_set(p_frame)) {
		return PHR_SIZE + FCF_SIZE;
	} else {
		return PHR_SIZE + FCF_SIZE + DSN_SIZE;
	}
}

static bool src_addr_is_present(const uint8_t *p_frame)
{
	return (p_frame[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK) !=
	       SRC_ADDR_TYPE_NONE;
}

static bool dst_addr_is_present(const uint8_t *p_frame)
{
	return (p_frame[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) !=
	       DEST_ADDR_TYPE_NONE;
}

static uint8_t src_addr_size_get(const uint8_t *p_frame)
{
	switch (p_frame[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK) {
	case SRC_ADDR_TYPE_NONE:
		return 0;

	case SRC_ADDR_TYPE_SHORT:
		return SHORT_ADDRESS_SIZE;

	case SRC_ADDR_TYPE_EXTENDED:
		return EXTENDED_ADDRESS_SIZE;

	default:
		return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
	}
}

static uint8_t dst_addr_size_get(const uint8_t *p_frame)
{
	switch (p_frame[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) {
	case DEST_ADDR_TYPE_NONE:
		return 0;

	case DEST_ADDR_TYPE_SHORT:
		return SHORT_ADDRESS_SIZE;

	case DEST_ADDR_TYPE_EXTENDED:
		return EXTENDED_ADDRESS_SIZE;

	default:
		return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
	}
}

/* PAN ID */
static bool dst_panid_is_present(const uint8_t *p_frame)
{
	bool panid_compression =
		(p_frame[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK) ? true :
								     false;

	switch (frame_version_get(p_frame)) {
	case FRAME_VERSION_0:
	case FRAME_VERSION_1:
		if (!dst_addr_is_present(p_frame)) {
			return false;
		}

		return true;

	case FRAME_VERSION_2:
	default:
		if (nrf_802154_frame_parser_dst_addr_is_extended(p_frame) &&
		    nrf_802154_frame_parser_src_addr_is_extended(p_frame)) {
			return panid_compression ? false : true;
		}

		if (src_addr_is_present(p_frame) &&
		    dst_addr_is_present(p_frame)) {
			return true;
		}

		if (src_addr_is_present(p_frame)) {
			return false;
		}

		if (dst_addr_is_present(p_frame)) {
			return panid_compression ? false : true;
		}

		return panid_compression ? true : false;
	}
}

static bool src_panid_is_present(const uint8_t *p_frame)
{
	bool panid_compression =
		(p_frame[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK) ? true :
								     false;

	switch (frame_version_get(p_frame)) {
	case FRAME_VERSION_0:
	case FRAME_VERSION_1:
		if (!src_addr_is_present(p_frame)) {
			return false;
		}

		return panid_compression ? false : true;

	case FRAME_VERSION_2:
	default:
		if (nrf_802154_frame_parser_dst_addr_is_extended(p_frame) &&
		    nrf_802154_frame_parser_src_addr_is_extended(p_frame)) {
			return false;
		}

		if (src_addr_is_present(p_frame) &&
		    dst_addr_is_present(p_frame)) {
			return panid_compression ? false : true;
		}

		if (src_addr_is_present(p_frame)) {
			return panid_compression ? false : true;
		}

		if (dst_addr_is_present(p_frame)) {
			return false;
		}

		return false;
	}
}

static bool src_panid_is_compressed(const uint8_t *p_frame)
{
	return dst_panid_is_present(p_frame) && !src_panid_is_present(p_frame);
}

/* Security */
static bool security_is_enabled(const uint8_t *p_frame)
{
	return p_frame[SECURITY_ENABLED_OFFSET] & SECURITY_ENABLED_BIT ? true :
									 false;
}

static uint8_t security_offset_get(const uint8_t *p_frame)
{
	uint8_t dst_addr_offset =
		nrf_802154_frame_parser_dst_addr_offset_get(p_frame);
	uint8_t dst_panid_offset =
		nrf_802154_frame_parser_dst_panid_offset_get(p_frame);
	uint8_t dst_addr_size = dst_addr_size_get(p_frame);
	uint8_t src_addr_offset =
		nrf_802154_frame_parser_src_addr_offset_get(p_frame);
	uint8_t src_panid_offset =
		nrf_802154_frame_parser_src_panid_offset_get(p_frame);
	uint8_t src_addr_size = src_addr_size_get(p_frame);

	if (src_addr_is_present(p_frame)) {
		if ((src_addr_size == NRF_802154_FRAME_PARSER_INVALID_OFFSET) ||
		    (src_addr_offset ==
		     NRF_802154_FRAME_PARSER_INVALID_OFFSET)) {
			return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
		}

		return src_addr_offset + src_addr_size;
	} else if (src_panid_is_present(p_frame)) {
		if (src_panid_offset ==
		    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
		}

		return src_panid_offset + PAN_ID_SIZE;
	} else if (dst_addr_offset) {
		if (dst_addr_size == NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
		}

		return dst_addr_offset + dst_addr_size;
	} else if (dst_panid_offset) {
		return dst_panid_offset + PAN_ID_SIZE;
	} else {
		return addressing_offset_get(p_frame);
	}
}

static uint8_t key_id_size_get(const uint8_t *p_frame)
{
	switch (*nrf_802154_frame_parser_sec_ctrl_get(p_frame) &
		KEY_ID_MODE_MASK) {
	case KEY_ID_MODE_1:
		return KEY_ID_MODE_1_SIZE;

	case KEY_ID_MODE_2:
		return KEY_ID_MODE_2_SIZE;

	case KEY_ID_MODE_3:
		return KEY_ID_MODE_3_SIZE;

	default:
		return 0;
	}
}

/* IEs */

static uint8_t ie_offset_get(const uint8_t *p_frame)
{
	uint8_t security_offset = security_offset_get(p_frame);
	uint8_t key_id_offset =
		nrf_802154_frame_parser_key_id_offset_get(p_frame);

	if (!security_is_enabled(p_frame)) {
		if (security_offset == NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
		}

		return security_offset;
	} else {
		if (key_id_offset == NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
		}

		return key_id_offset + key_id_size_get(p_frame);
	}
}

/******************************************************************************
 * @section Frame format functions
 *****************************************************************************/

bool nrf_802154_frame_parser_dst_addr_is_extended(const uint8_t *p_frame)
{
	return (p_frame[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) ==
	       DEST_ADDR_TYPE_EXTENDED;
}

bool nrf_802154_frame_parser_src_addr_is_extended(const uint8_t *p_frame)
{
	return (p_frame[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK) ==
	       SRC_ADDR_TYPE_EXTENDED;
}

bool nrf_802154_frame_parser_src_addr_is_short(const uint8_t *p_frame)
{
	return (p_frame[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK) ==
	       SRC_ADDR_TYPE_SHORT;
}

bool nrf_802154_frame_parser_dsn_suppress_bit_is_set(const uint8_t *p_frame)
{
	return (p_frame[DSN_SUPPRESS_OFFSET] & DSN_SUPPRESS_BIT) ? true : false;
}

bool nrf_802154_frame_parser_ie_present_bit_is_set(const uint8_t *p_frame)
{
	return (p_frame[IE_PRESENT_OFFSET] & IE_PRESENT_BIT) ? true : false;
}

bool nrf_802154_frame_parser_ar_bit_is_set(const uint8_t *p_frame)
{
	return (p_frame[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT) ? true : false;
}

/******************************************************************************
 * @section Offset functions
 *****************************************************************************/

uint8_t nrf_802154_frame_parser_dst_panid_offset_get(const uint8_t *p_frame)
{
	if (dst_panid_is_present(p_frame)) {
		return addressing_offset_get(p_frame);
	} else {
		return 0;
	}
}

uint8_t nrf_802154_frame_parser_dst_addr_offset_get(const uint8_t *p_frame)
{
	uint8_t dst_panid_offset =
		nrf_802154_frame_parser_dst_panid_offset_get(p_frame);

	if (dst_addr_is_present(p_frame)) {
		return 0 == dst_panid_offset ? addressing_offset_get(p_frame) :
					       dst_panid_offset + PAN_ID_SIZE;
	} else {
		return 0;
	}
}

uint8_t nrf_802154_frame_parser_dst_addr_end_offset_get(const uint8_t *p_frame)
{
	uint8_t offset = addressing_offset_get(p_frame);
	uint8_t dst_addr_size = dst_addr_size_get(p_frame);

	if (dst_addr_size == NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
		return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
	}

	if (dst_panid_is_present(p_frame)) {
		offset += PAN_ID_SIZE;
	}

	offset += dst_addr_size;

	return offset;
}

uint8_t nrf_802154_frame_parser_src_panid_offset_get(const uint8_t *p_frame)
{
	uint8_t dst_addr_offset =
		nrf_802154_frame_parser_dst_addr_offset_get(p_frame);
	uint8_t dst_panid_offset =
		nrf_802154_frame_parser_dst_panid_offset_get(p_frame);
	uint8_t dst_addr_size = dst_addr_size_get(p_frame);

	if (src_panid_is_present(p_frame)) {
		if (dst_addr_offset) {
			if (dst_addr_size ==
			    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
				return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
			}

			return dst_addr_offset + dst_addr_size;
		} else if (dst_panid_offset) {
			return dst_panid_offset + PAN_ID_SIZE;
		} else {
			return addressing_offset_get(p_frame);
		}
	} else if (src_panid_is_compressed(p_frame)) {
		return nrf_802154_frame_parser_dst_panid_offset_get(p_frame);
	} else {
		return 0;
	}
}

uint8_t nrf_802154_frame_parser_src_addr_offset_get(const uint8_t *p_frame)
{
	uint8_t dst_addr_offset =
		nrf_802154_frame_parser_dst_addr_offset_get(p_frame);
	uint8_t dst_panid_offset =
		nrf_802154_frame_parser_dst_panid_offset_get(p_frame);
	uint8_t dst_addr_size = dst_addr_size_get(p_frame);
	uint8_t src_panid_offset =
		nrf_802154_frame_parser_src_panid_offset_get(p_frame);

	if (src_addr_is_present(p_frame)) {
		if (src_panid_is_present(p_frame)) {
			if (src_panid_offset ==
			    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
				return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
			}

			return src_panid_offset + PAN_ID_SIZE;
		} else if (dst_addr_offset) {
			if (dst_addr_size ==
			    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
				return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
			}

			return dst_addr_offset + dst_addr_size;
		} else if (dst_panid_offset) {
			return dst_panid_offset + PAN_ID_SIZE;
		} else {
			return addressing_offset_get(p_frame);
		}
	} else {
		return 0;
	}
}

uint8_t
nrf_802154_frame_parser_addressing_end_offset_get(const uint8_t *p_frame)
{
	return security_offset_get(p_frame);
}

uint8_t nrf_802154_frame_parser_sec_ctrl_offset_get(const uint8_t *p_frame)
{
	if (!security_is_enabled(p_frame)) {
		return 0;
	} else {
		return security_offset_get(p_frame);
	}
}

uint8_t nrf_802154_frame_parser_key_id_offset_get(const uint8_t *p_frame)
{
	uint8_t sec_ctrl_offset =
		nrf_802154_frame_parser_sec_ctrl_offset_get(p_frame);

	if (0 == sec_ctrl_offset) {
		return 0;
	}

	if (NRF_802154_FRAME_PARSER_INVALID_OFFSET == sec_ctrl_offset) {
		return NRF_802154_FRAME_PARSER_INVALID_OFFSET;
	}

	if (p_frame[sec_ctrl_offset] & FRAME_COUNTER_SUPPRESS_BIT) {
		return sec_ctrl_offset + SECURITY_CONTROL_SIZE;
	} else {
		return sec_ctrl_offset + SECURITY_CONTROL_SIZE +
		       FRAME_COUNTER_SIZE;
	}
}

uint8_t nrf_802154_frame_parser_ie_header_offset_get(const uint8_t *p_frame)
{
	if (!nrf_802154_frame_parser_ie_present_bit_is_set(p_frame)) {
		return 0;
	} else {
		return ie_offset_get(p_frame);
	}
}

/******************************************************************************
 * @section Get functions
 *****************************************************************************/

int nrf_802154_frame_parser_frame_type_get(const uint8_t *p_frame)
{
	uint8_t frame_type = (p_frame[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);
	return frame_type;
}

const uint8_t *nrf_802154_frame_parser_dst_addr_get(const uint8_t *p_frame,
						    bool *p_dst_addr_extended)
{
	uint8_t dst_addr_offset =
		nrf_802154_frame_parser_dst_addr_offset_get(p_frame);

	if ((0 == dst_addr_offset) ||
	    (NRF_802154_FRAME_PARSER_INVALID_OFFSET == dst_addr_offset)) {
		*p_dst_addr_extended = false;
		return NULL;
	} else {
		*p_dst_addr_extended =
			nrf_802154_frame_parser_dst_addr_is_extended(p_frame);
		return &p_frame[dst_addr_offset];
	}
}

const uint8_t *nrf_802154_frame_parser_dst_panid_get(const uint8_t *p_frame)
{
	uint8_t dst_panid_offset =
		nrf_802154_frame_parser_dst_panid_offset_get(p_frame);

	return ((0 == dst_panid_offset) ||
		(NRF_802154_FRAME_PARSER_INVALID_OFFSET == dst_panid_offset)) ?
		       NULL :
		       &p_frame[dst_panid_offset];
}

const uint8_t *nrf_802154_frame_parser_src_panid_get(const uint8_t *p_frame)
{
	uint8_t src_panid_offset =
		nrf_802154_frame_parser_src_panid_offset_get(p_frame);

	return ((0 == src_panid_offset) ||
		(NRF_802154_FRAME_PARSER_INVALID_OFFSET == src_panid_offset)) ?
		       NULL :
		       &p_frame[src_panid_offset];
}

const uint8_t *nrf_802154_frame_parser_src_addr_get(const uint8_t *p_frame,
						    bool *p_src_addr_extended)
{
	uint8_t src_addr_offset =
		nrf_802154_frame_parser_src_addr_offset_get(p_frame);

	if ((0 == src_addr_offset) ||
	    (NRF_802154_FRAME_PARSER_INVALID_OFFSET == src_addr_offset)) {
		*p_src_addr_extended = false;
		return NULL;
	} else {
		*p_src_addr_extended =
			nrf_802154_frame_parser_src_addr_is_extended(p_frame);
		return &p_frame[src_addr_offset];
	}
}

bool nrf_802154_frame_parser_mhr_parse(
	const uint8_t *p_frame, nrf_802154_frame_parser_mhr_data_t *p_fields)
{
	uint8_t offset = addressing_offset_get(p_frame);
	bool is_dst_panid_present = dst_panid_is_present(p_frame);
	bool is_src_panid_present = src_panid_is_present(p_frame);

	if (is_dst_panid_present) {
		p_fields->p_dst_panid = &p_frame[offset];
		offset += PAN_ID_SIZE;
	} else {
		p_fields->p_dst_panid = NULL;
	}

	if (dst_addr_is_present(p_frame)) {
		p_fields->p_dst_addr = &p_frame[offset];
		p_fields->dst_addr_size = dst_addr_size_get(p_frame);
		offset += (p_fields->dst_addr_size);

		if (p_fields->dst_addr_size ==
		    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return false;
		}
	} else {
		p_fields->p_dst_addr = NULL;
		p_fields->dst_addr_size = 0;
	}

	if (is_src_panid_present) {
		p_fields->p_src_panid = &p_frame[offset];
		offset += PAN_ID_SIZE;
	} else if (is_dst_panid_present) {
		p_fields->p_src_panid = p_fields->p_dst_panid;
	} else {
		p_fields->p_src_panid = NULL;
	}

	if (src_addr_is_present(p_frame)) {
		p_fields->p_src_addr = &p_frame[offset];
		p_fields->src_addr_size = src_addr_size_get(p_frame);
		offset += (p_fields->src_addr_size);

		if (p_fields->src_addr_size ==
		    NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			return false;
		}
	} else {
		p_fields->p_src_addr = NULL;
		p_fields->src_addr_size = 0;
	}

	p_fields->addressing_end_offset = offset;

	if (security_is_enabled(p_frame)) {
		p_fields->p_sec_ctrl = &p_frame[offset];
		/** TODO increment offset... */
	} else {
		p_fields->p_sec_ctrl = NULL;
	}

	return true;
}

const uint8_t *nrf_802154_frame_parser_sec_ctrl_get(const uint8_t *p_frame)
{
	uint8_t sec_ctrl_offset =
		nrf_802154_frame_parser_sec_ctrl_offset_get(p_frame);

	return ((0 == sec_ctrl_offset) ||
		(NRF_802154_FRAME_PARSER_INVALID_OFFSET == sec_ctrl_offset)) ?
		       NULL :
		       &p_frame[sec_ctrl_offset];
}

const uint8_t *nrf_802154_frame_parser_key_id_get(const uint8_t *p_frame)
{
	uint8_t key_id_offset =
		nrf_802154_frame_parser_key_id_offset_get(p_frame);

	return ((0 == key_id_offset) ||
		(NRF_802154_FRAME_PARSER_INVALID_OFFSET == key_id_offset)) ?
		       NULL :
		       &p_frame[key_id_offset];
}

const uint8_t *nrf_802154_frame_parser_ie_header_get(const uint8_t *p_frame)
{
	uint8_t ie_header_offset =
		nrf_802154_frame_parser_ie_header_offset_get(p_frame);

	if ((0 == ie_header_offset) ||
	    (NRF_802154_FRAME_PARSER_INVALID_OFFSET == ie_header_offset)) {
		return NULL;
	}

	return &p_frame[ie_header_offset];
}
