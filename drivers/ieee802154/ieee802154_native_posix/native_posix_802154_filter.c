/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements incoming frame filtering according to 3 and 4 levels
 *   of filtering.
 *
 * Filtering details are specified in 802.15.4-2015: 6.7.2.
 * 1st and 2nd filtering level is performed by FSM module depending on
 * promiscuous mode, when FCS is received.
 */

#include "native_posix_802154_filter.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "native_posix_802154_const.h"
#include "native_posix_802154_frame_parser.h"
#include "native_posix_802154_pib.h"

#define FCF_CHECK_OFFSET (PHR_SIZE + FCF_SIZE)
#define PANID_CHECK_OFFSET (DEST_ADDR_OFFSET)
#define SHORT_ADDR_CHECK_OFFSET (DEST_ADDR_OFFSET + SHORT_ADDRESS_SIZE)
#define EXTENDED_ADDR_CHECK_OFFSET (DEST_ADDR_OFFSET + EXTENDED_ADDRESS_SIZE)

/**
 * @brief Check if given frame version is allowed for given frame type.
 *
 * @param[in] frame_type     Type of incoming frame.
 * @param[in] frame_version  Version of incoming frame.
 *
 * @retval true   Given frame version is allowed for given frame type.
 * @retval false  Given frame version is not allowed for given frame type.
 */
static bool frame_type_and_version_filter(uint8_t frame_type,
					  uint8_t frame_version)
{
	bool result;

	switch (frame_type) {
	case FRAME_TYPE_BEACON:
	case FRAME_TYPE_DATA:
	case FRAME_TYPE_ACK:
	case FRAME_TYPE_COMMAND:
		result = (frame_version != FRAME_VERSION_3);
		break;

	case FRAME_TYPE_MULTIPURPOSE:
		result = (frame_version == FRAME_VERSION_0);
		break;

	case FRAME_TYPE_FRAGMENT:
	case FRAME_TYPE_EXTENDED:
		result = true;
		break;

	default:
		result = false;
	}

	return result;
}

/**
 * @brief Check if given frame type may include destination address fields.
 *
 * @note Actual presence of destination address fields in the frame is
 * 		 indicated by FCF.
 *
 * @param[in] frame_type  Type of incoming frame.
 *
 * @retval true   Given frame type may include addressing fields.
 * @retval false  Given frame type may not include addressing fields.
 */
static bool dst_addressing_may_be_present(uint8_t frame_type)
{
	bool result;

	switch (frame_type) {
	case FRAME_TYPE_BEACON:
	case FRAME_TYPE_DATA:
	case FRAME_TYPE_ACK:
	case FRAME_TYPE_COMMAND:
	case FRAME_TYPE_MULTIPURPOSE:
		result = true;
		break;

	case FRAME_TYPE_FRAGMENT:
	case FRAME_TYPE_EXTENDED:
		result = false;
		break;

	default:
		result = false;
	}

	return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame assuming its
 * 		  version is 2006.
 *
 * If given frame contains errors that prevent getting offset, this function
 * returns false. If there are no destination address fields in given frame,
 * this function returns true and does not modify @p p_num_bytes. If there is
 * destination address in given frame, this function returns true and inserts
 * offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_data       Pointer to a buffer containing PHR and PSDU of the
 * 							incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were
 * 												  detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but
 * 												  addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given
 * 												  frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t
dst_addressing_end_offset_get_2006(const uint8_t *p_data, uint8_t *p_num_bytes,
				   uint8_t frame_type)
{
	nrf_802154_rx_error_t result;

	switch (p_data[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) {
	case DEST_ADDR_TYPE_SHORT:
		*p_num_bytes = SHORT_ADDR_CHECK_OFFSET;
		result = NRF_802154_RX_ERROR_NONE;
		break;

	case DEST_ADDR_TYPE_EXTENDED:
		*p_num_bytes = EXTENDED_ADDR_CHECK_OFFSET;
		result = NRF_802154_RX_ERROR_NONE;
		break;

	case DEST_ADDR_TYPE_NONE:
		if (nrf_802154_pib_pan_coord_get() ||
		    (frame_type == FRAME_TYPE_BEACON)) {
			switch (p_data[SRC_ADDR_TYPE_OFFSET] &
				SRC_ADDR_TYPE_MASK) {
			case SRC_ADDR_TYPE_SHORT:
			case SRC_ADDR_TYPE_EXTENDED:
				*p_num_bytes = PANID_CHECK_OFFSET;
				result = NRF_802154_RX_ERROR_NONE;
				break;

			default:
				result = NRF_802154_RX_ERROR_INVALID_FRAME;
			}
		} else {
			result = NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
		}

		break;

	default:
		result = NRF_802154_RX_ERROR_INVALID_FRAME;
	}

	return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame assuming its
 * 		  version is 2015.
 *
 * If given frame contains errors that prevent getting offset, this function
 * returns false. If there are no destination address fields in given frame,
 * this function returns true and does not modify @p p_num_bytes. If there is
 * destination address in given frame, this function returns true and inserts
 * offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_data       Pointer to a buffer containing PHR and PSDU of the
 * 							incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were
 * 												  detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but
 * 												  addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given
 * 												  frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t
dst_addressing_end_offset_get_2015(const uint8_t *p_data, uint8_t *p_num_bytes,
				   uint8_t frame_type)
{
	nrf_802154_rx_error_t result;

	switch (frame_type) {
	case FRAME_TYPE_BEACON:
	case FRAME_TYPE_DATA:
	case FRAME_TYPE_ACK:
	case FRAME_TYPE_COMMAND: {
		uint8_t end_offset =
			nrf_802154_frame_parser_dst_addr_end_offset_get(p_data);

		if (end_offset == NRF_802154_FRAME_PARSER_INVALID_OFFSET) {
			result = NRF_802154_RX_ERROR_INVALID_FRAME;
		} else {
			*p_num_bytes = end_offset;
			result = NRF_802154_RX_ERROR_NONE;
		}
	} break;

	case FRAME_TYPE_MULTIPURPOSE:
		/** TODO: Implement dst addressing filtering according to 2015 spec */
		result = NRF_802154_RX_ERROR_INVALID_FRAME;
		break;

	case FRAME_TYPE_FRAGMENT:
	case FRAME_TYPE_EXTENDED:
		/* No addressing data */
		result = NRF_802154_RX_ERROR_NONE;
		break;

	default:
		result = NRF_802154_RX_ERROR_INVALID_FRAME;
	}

	return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame.
 *
 * If given frame contains errors that prevent getting offset, this function
 * returns false. If there are no destination address fields in given frame,
 * this function returns true and does not modify @p p_num_bytes. If there is
 * destination address in given frame, this function returns true and
 * inserts offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_data       Pointer to a buffer containing PHR and PSDU of the
 * 							incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were
 * 												  detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but
 * 												  addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given
 * 												  frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t
dst_addressing_end_offset_get(const uint8_t *p_data, uint8_t *p_num_bytes,
			      uint8_t frame_type, uint8_t frame_version)
{
	nrf_802154_rx_error_t result;

	switch (frame_version) {
	case FRAME_VERSION_0:
	case FRAME_VERSION_1:
		result = dst_addressing_end_offset_get_2006(p_data, p_num_bytes,
							    frame_type);
		break;

	case FRAME_VERSION_2:
		result = dst_addressing_end_offset_get_2015(p_data, p_num_bytes,
							    frame_type);
		break;

	default:
		result = NRF_802154_RX_ERROR_INVALID_FRAME;
	}

	return result;
}

/**
 * Verify if destination PAN Id of incoming frame allows processing by this
 * node.
 *
 * @param[in] p_panid     Pointer of PAN ID of incoming frame.
 * @param[in] frame_type  Type of the frame being filtered.
 *
 * @retval true   PAN Id of incoming frame allows further processing of the
 * 				  frame.
 * @retval false  PAN Id of incoming frame does not allow further processing.
 */
static bool dst_pan_id_check(const uint8_t *p_panid, uint8_t frame_type)
{
	bool result;

	if ((0 == memcmp(p_panid, nrf_802154_pib_pan_id_get(), PAN_ID_SIZE)) ||
	    (0 == memcmp(p_panid, BROADCAST_ADDRESS, PAN_ID_SIZE))) {
		result = true;
	} else if ((FRAME_TYPE_BEACON == frame_type) &&
		   (0 == memcmp(nrf_802154_pib_pan_id_get(), BROADCAST_ADDRESS,
				PAN_ID_SIZE))) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 * Verify if destination short address of incoming frame allows processing by
 * this node.
 *
 * @param[in] p_dst_addr  Pointer of destination address of incoming frame.
 * @param[in] frame_type  Type of the frame being filtered.
 *
 * @retval true   Destination address of incoming frame allows further
 * 				  processing of the frame.
 * @retval false  Destination address of incoming frame does not allow further
 * 				  processing.
 */
static bool dst_short_addr_check(const uint8_t *p_dst_addr, uint8_t frame_type)
{
	bool result;

	if ((0 == memcmp(p_dst_addr, nrf_802154_pib_short_address_get(),
			 SHORT_ADDRESS_SIZE)) ||
	    (0 == memcmp(p_dst_addr, BROADCAST_ADDRESS, SHORT_ADDRESS_SIZE))) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 * Verify if destination extended address of incoming frame allows processing
 * by this node.
 *
 * @param[in] p_dst_addr  Pointer of destination address of incoming frame.
 * @param[in] frame_type  Type of the frame being filtered.
 *
 * @retval true   Destination address of incoming frame allows further
 * 				  processing of the frame.
 * @retval false  Destination address of incoming frame does not allow further
 * 				  processing.
 */
static bool dst_extended_addr_check(const uint8_t *p_dst_addr,
				    uint8_t frame_type)
{
	bool result;

	if (0 == memcmp(p_dst_addr, nrf_802154_pib_extended_address_get(),
			EXTENDED_ADDRESS_SIZE)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 * Verify if destination addressing of incoming frame allows processing by this
 * node. This function checks addressing according to IEEE 802.15.4-2015.
 *
 * @param[in] p_data  Pointer to a buffer containing PHR and PSDU of the
 * 					  incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               Destination address of
 * 												  incoming frame allows further
 * 												  processing of the frame.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Received frame is invalid.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  Destination address of
 * 												  incoming frame does not allow
 * 												  further processing.
 */
static nrf_802154_rx_error_t dst_addr_check(const uint8_t *p_data,
					    uint8_t frame_type)
{
	bool result;
	nrf_802154_frame_parser_mhr_data_t mhr_data;

	result = nrf_802154_frame_parser_mhr_parse(p_data, &mhr_data);

	if (!result) {
		return NRF_802154_RX_ERROR_INVALID_FRAME;
	}

	if (mhr_data.p_dst_panid != NULL) {
		if (!dst_pan_id_check(mhr_data.p_dst_panid, frame_type)) {
			return NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
		}
	}

	switch (mhr_data.dst_addr_size) {
	case SHORT_ADDRESS_SIZE:
		return dst_short_addr_check(mhr_data.p_dst_addr, frame_type) ?
			       NRF_802154_RX_ERROR_NONE :
			       NRF_802154_RX_ERROR_INVALID_DEST_ADDR;

	case EXTENDED_ADDRESS_SIZE:
		return dst_extended_addr_check(mhr_data.p_dst_addr,
					       frame_type) ?
			       NRF_802154_RX_ERROR_NONE :
			       NRF_802154_RX_ERROR_INVALID_DEST_ADDR;

	case 0:
		/** Allow frames destined to the Pan Coordinator without destination
		 * address or beacon frames without destination address
		 */
		return (nrf_802154_pib_pan_coord_get() ||
			(frame_type == FRAME_TYPE_BEACON)) ?
			       NRF_802154_RX_ERROR_NONE :
			       NRF_802154_RX_ERROR_INVALID_DEST_ADDR;

	default:
		assert(false);
	}

	return NRF_802154_RX_ERROR_INVALID_FRAME;
}

nrf_802154_rx_error_t nrf_802154_filter_frame_part(const uint8_t *p_data,
						   uint8_t *p_num_bytes)
{
	nrf_802154_rx_error_t result = NRF_802154_RX_ERROR_INVALID_FRAME;
	uint8_t frame_type = p_data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK;
	uint8_t frame_version =
		p_data[FRAME_VERSION_OFFSET] & FRAME_VERSION_MASK;

	switch (*p_num_bytes) {
	case FCF_CHECK_OFFSET:
		if (p_data[0] < IMM_ACK_LENGTH || p_data[0] > MAX_PACKET_SIZE) {
			result = NRF_802154_RX_ERROR_INVALID_LENGTH;
			break;
		}

		if (!frame_type_and_version_filter(frame_type, frame_version)) {
			result = NRF_802154_RX_ERROR_INVALID_FRAME;
			break;
		}

		if (!dst_addressing_may_be_present(frame_type)) {
			result = NRF_802154_RX_ERROR_NONE;
			break;
		}

		result = dst_addressing_end_offset_get(
			p_data, p_num_bytes, frame_type, frame_version);
		break;

	default:
		result = dst_addr_check(p_data, frame_type);
		break;
	}

	return result;
}
