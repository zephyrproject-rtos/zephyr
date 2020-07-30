/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * This file implements procedures to set pending bit and 802.15.4-2015
 * information elements in nRF 802.15.4 radio driver.
 *
 */

#include "native_posix_802154_ack_data.h"

#include <assert.h>
#include <string.h>

#include "../native_posix_802154_frame_parser.h"
#include "../native_posix_802154_config.h"
#include "../native_posix_802154_const.h"

/** Maximum number of Short Addresses of nodes for which there is ACK
 * data to set.
 */
#define NUM_SHORT_ADDRESSES NRF_802154_PENDING_SHORT_ADDRESSES
/** Maximum number of Extended Addresses of nodes for which there is ACK
 * data to set.
 */
#define NUM_EXTENDED_ADDRESSES NRF_802154_PENDING_EXTENDED_ADDRESSES

/* Structure representing pending bit setting variables. */
typedef struct {
	/** If setting pending bit is enabled. */
	bool enabled;

	/** Array of short addresses of nodes for which there is pending
	 * data in the buffer.
	 */
	uint8_t short_addr[NUM_SHORT_ADDRESSES][SHORT_ADDRESS_SIZE];

	/** Array of extended addresses of nodes for which there is pending
	 * data in the buffer.
	 */
	uint8_t extended_addr[NUM_EXTENDED_ADDRESSES][EXTENDED_ADDRESS_SIZE];

	/** Current number of short addresses of nodes for which there is
	 * pending data in the buffer.
	 */
	uint32_t num_of_short_addr;

	/** Current number of extended addresses of nodes for which there
	 * is pending data in the buffer.
	 */
	uint32_t num_of_ext_addr;
} pending_bit_arrays_t;

/* Structure representing a single IE record. */
typedef struct {
	/** Pointer to IE data buffer. */
	uint8_t p_data[NRF_802154_MAX_ACK_IE_SIZE];

	/** Length of the buffer. */
	uint8_t len;
} ie_data_t;

/* Structure representing IE records sent in an ACK message to a given
 * short address.
 */
typedef struct {
	/** Short address of peer node. */
	uint8_t addr[SHORT_ADDRESS_SIZE];

	/** Pointer to IE records. */
	ie_data_t ie_data;
} ack_short_ie_data_t;

/* Structure representing IE records sent in an ACK message to a given
 * extended address.
 */
typedef struct {
	/* Extended address of peer node. */
	uint8_t addr[EXTENDED_ADDRESS_SIZE];
	/* Pointer to IE records. */
	ie_data_t ie_data;
} ack_ext_ie_data_t;

/* Structure representing IE data setting variables. */
typedef struct {
	/** Array of short addresses and IE records sent to these addresses. */
	ack_short_ie_data_t short_data[NUM_SHORT_ADDRESSES];

	/** Array of extended addresses and IE records sent to these
	 * addresses.
	 */
	ack_ext_ie_data_t ext_data[NUM_EXTENDED_ADDRESSES];

	/** Current number of short addresses stored in @p short_data. */
	uint32_t num_of_short_data;

	/** Current number of extended addresses stored in @p ext_data. */
	uint32_t num_of_ext_data;
} ie_arrays_t;

/** TODO: Combine below arrays to perform binary search only once per Ack
 * generation.
 */
static pending_bit_arrays_t m_pending_bit;
static ie_arrays_t m_ie;
static nrf_802154_src_addr_match_t m_src_matching_method;

/******************************************************************************
 * @section Array handling helper functions
 *****************************************************************************/

/**
 * @brief Compare two extended addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be
 *                              compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be
 *                              compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t extended_addr_compare(const uint8_t *p_first_addr,
				    const uint8_t *p_second_addr)
{
	uint32_t first_addr;
	uint32_t second_addr;

	/** Compare extended address in two steps to prevent unaligned
	 * access error.
	 */
	for (uint32_t i = 0; i < EXTENDED_ADDRESS_SIZE / sizeof(uint32_t);
	     i++) {
		first_addr =
			*(uint32_t *)(p_first_addr + (i * sizeof(uint32_t)));
		second_addr =
			*(uint32_t *)(p_second_addr + (i * sizeof(uint32_t)));

		if (first_addr < second_addr) {
			return -1;
		} else if (first_addr > second_addr) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Compare two short addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be
 *                              compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be
 *                              compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t short_addr_compare(const uint8_t *p_first_addr,
				 const uint8_t *p_second_addr)
{
	uint16_t first_addr = *(uint16_t *)(p_first_addr);
	uint16_t second_addr = *(uint16_t *)(p_second_addr);

	if (first_addr < second_addr) {
		return -1;
	} else if (first_addr > second_addr) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Compare two addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be
 *                              compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be
 *                              compared.
 * @param[in]  extended         Indication if @p p_first_addr and @p
 *                              p_second_addr are extended or short addresses.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t addr_compare(const uint8_t *p_first_addr,
			   const uint8_t *p_second_addr, bool extended)
{
	if (extended) {
		return extended_addr_compare(p_first_addr, p_second_addr);
	} else {
		return short_addr_compare(p_first_addr, p_second_addr);
	}
}

/**
 * @brief Perform a binary search for an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[in]  p_addr_array     Pointer to a list of addresses to be searched.
 * @param[out] p_location       If the address @p p_addr appears in the list,
 *                              this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr
 *                              would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended or a
 *                              short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_binary_search(const uint8_t *p_addr,
			       const uint8_t *p_addr_array,
			       uint32_t *p_location, uint8_t data_type,
			       bool extended)
{
	uint32_t addr_array_len = 0;
	uint8_t entry_size = 0;

	switch (data_type) {
	case NRF_802154_ACK_DATA_PENDING_BIT:
		entry_size =
			extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE;
		addr_array_len = extended ? m_pending_bit.num_of_ext_addr :
					    m_pending_bit.num_of_short_addr;
		break;

	case NRF_802154_ACK_DATA_IE:
		entry_size = extended ? sizeof(ack_ext_ie_data_t) :
					sizeof(ack_short_ie_data_t);
		addr_array_len = extended ? m_ie.num_of_ext_data :
					    m_ie.num_of_short_data;
		break;

	default:
		assert(false);
		break;
	}

	/* The actual algorithm */
	int32_t low = 0;
	uint32_t midpoint = 0;
	int32_t high = addr_array_len;

	while (high >= low) {
		midpoint = (uint32_t)(low + (high - low) / 2);

		if (midpoint >= addr_array_len) {
			break;
		}

		switch (addr_compare(p_addr,
				     p_addr_array + entry_size * midpoint,
				     extended)) {
		case -1:
			high = (int32_t)(midpoint - 1);
			break;

		case 0:
			*p_location = midpoint;
			return true;

		case 1:
			low = (int32_t)(midpoint + 1);
			break;

		default:
			break;
		}
	}

	/* If in the last iteration of the loop the last case was utilized, it
	 * means that the midpoint found by the algorithm is less than the address
	 * to be added. The midpoint should be therefore shifted to the next
	 * position. As a simplified example, a { 1, 3, 4 } array can be
	 * considered.
	 * Suppose that a number equal to 2 is about to be added to the array.
	 * At the beginning of the last iteration, midpoint is equal to 1 and low
	 * and high are equal to 0. Midpoint is then set to 0 and with last case
	 * being utilized, low is set to 1. However, midpoint equal to 0 is
	 * incorrect, as in the last iteration first element of the array proves
	 * to be less than the element to be added to the array. With the below
	 * code, midpoint is then shifted to 1.
	 */
	if ((uint32_t)low == midpoint + 1) {
		midpoint++;
	}

	*p_location = midpoint;
	return false;
}

/**
 * @brief Find an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[out] p_location       If the address @p p_addr appears in the list,
 *                              this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr
 *                              would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended
 *                              or a short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_index_find(const uint8_t *p_addr, uint32_t *p_location,
			    uint8_t data_type, bool extended)
{
	uint8_t *p_addr_array;
	bool valid_data_type = true;

	switch (data_type) {
	case NRF_802154_ACK_DATA_PENDING_BIT:
		p_addr_array = extended ?
				       (uint8_t *)m_pending_bit.extended_addr :
				       (uint8_t *)m_pending_bit.short_addr;
		break;

	case NRF_802154_ACK_DATA_IE:
		p_addr_array = extended ? (uint8_t *)m_ie.ext_data :
					  (uint8_t *)m_ie.short_data;
		break;

	default:
		valid_data_type = false;
		assert(false);
		break;
	}

	if (!valid_data_type) {
		return false;
	}

	return addr_binary_search(p_addr, p_addr_array, p_location, data_type,
				  extended);
}

/**
 * @brief Thread implementation of the address matching algorithm.
 *
 * @param[in]  p_frame  Pointer to the frame for which the ACK frame is being
 *                      prepared.
 *
 * @retval true   Pending bit is to be set.
 * @retval false  Pending bit is to be cleared.
 */
static bool addr_match_thread(const uint8_t *p_frame)
{
	bool extended;
	uint32_t location;
	const uint8_t *p_src_addr =
		nrf_802154_frame_parser_src_addr_get(p_frame, &extended);

	/* The pending bit is set by default. */
	if (!m_pending_bit.enabled || (NULL == p_src_addr)) {
		return true;
	}

	return addr_index_find(p_src_addr, &location,
			       NRF_802154_ACK_DATA_PENDING_BIT, extended);
}

/**
 * @brief Zigbee implementation of the address matching algorithm.
 *
 * @param[in]  p_frame  Pointer to the frame for which the ACK frame is being
 *                      prepared.
 *
 * @retval true   Pending bit is to be set.
 * @retval false  Pending bit is to be cleared.
 */
static bool addr_match_zigbee(const uint8_t *p_frame)
{
	uint8_t frame_type;
	nrf_802154_frame_parser_mhr_data_t mhr_fields;
	uint32_t location;
	const uint8_t *p_cmd = p_frame;
	bool ret = false;

	/**
	 * If ack data generator module is disabled do not perform check, return
	 * true by default.
	 */
	if (!m_pending_bit.enabled) {
		return true;
	}

	/* Check the frame type. */
	frame_type = (p_frame[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

	/* Parse the MAC header and retrieve the command type. */
	if (nrf_802154_frame_parser_mhr_parse(p_frame, &mhr_fields)) {
		/** Note: Security header is not included in the offset.
		 * If security is to be used at any point, additional
		 * calculation in nrf_802154_frame_parser_mhr_parse needs
		 * to be implemented.
		 */
		p_cmd += mhr_fields.addressing_end_offset;
	} else {
		/** If invalid source or destination addressing mode is
		 * detected, assume unknown device. Command type cannot be
		 * checked, as addressing_end_offset value will be invalid.
		 */
		return true;
	}

	/* Check frame type and command type. */
	if ((frame_type == FRAME_TYPE_COMMAND) &&
	    (*p_cmd == MAC_CMD_DATA_REQ)) {
		/** Check addressing type-in long case address, pb should
		 * always be 1.
		 */
		if (mhr_fields.src_addr_size == SHORT_ADDRESS_SIZE) {
			/** Return true if address is not found on the
			* m_pending_bits list.
			*/
			ret = !addr_index_find(mhr_fields.p_src_addr, &location,
					       NRF_802154_ACK_DATA_PENDING_BIT,
					       false);
		} else {
			ret = true;
		}
	}

	return ret;
}

/**
 * @brief Standard-compliant implementation of the address matching algorithm.
 *
 * Function always returns true. It is IEEE 802.15.4 compliant, as per 6.7.3.
 * Higher layer should ensure empty data frame with no AR is sent afterwards.
 *
 * @param[in]  p_frame  Pointer to the frame for which the ACK frame is being
 *                      prepared.
 *
 * @retval true   Pending bit is to be set.
 */
static bool addr_match_standard_compliant(const uint8_t *p_frame)
{
	(void)p_frame;
	return true;
}

/**
 * @brief Add an address to the address list in ascending order.
 *
 * @param[in]  p_addr           Pointer to the address to be added.
 * @param[in]  location         Index of the location where @p p_addr should
 *                              be added.
 * @param[in]  extended         Indication if @p p_addr is an extended or
 *                              a short addresses.
 *
 * @retval true   Address @p p_addr has been added to the list successfully.
 * @retval false  Address @p p_addr could not be added to the list.
 */
static bool addr_add(const uint8_t *p_addr, uint32_t location,
		     uint8_t data_type, bool extended)
{
	uint32_t *p_addr_array_len;
	uint32_t max_addr_array_len;
	uint8_t *p_addr_array;
	uint8_t entry_size;
	bool valid_data_type = true;

	switch (data_type) {
	case NRF_802154_ACK_DATA_PENDING_BIT:
		if (extended) {
			p_addr_array = (uint8_t *)m_pending_bit.extended_addr;
			max_addr_array_len = NUM_EXTENDED_ADDRESSES;
			p_addr_array_len = &m_pending_bit.num_of_ext_addr;
			entry_size = EXTENDED_ADDRESS_SIZE;
		} else {
			p_addr_array = (uint8_t *)m_pending_bit.short_addr;
			max_addr_array_len = NUM_SHORT_ADDRESSES;
			p_addr_array_len = &m_pending_bit.num_of_short_addr;
			entry_size = SHORT_ADDRESS_SIZE;
		}
		break;

	case NRF_802154_ACK_DATA_IE:
		if (extended) {
			p_addr_array = (uint8_t *)m_ie.ext_data;
			max_addr_array_len = NUM_EXTENDED_ADDRESSES;
			p_addr_array_len = &m_ie.num_of_ext_data;
			entry_size = sizeof(ack_ext_ie_data_t);
		} else {
			p_addr_array = (uint8_t *)m_ie.short_data;
			max_addr_array_len = NUM_SHORT_ADDRESSES;
			p_addr_array_len = &m_ie.num_of_short_data;
			entry_size = sizeof(ack_short_ie_data_t);
		}
		break;

	default:
		valid_data_type = false;
		assert(false);
		break;
	}

	if (!valid_data_type || (*p_addr_array_len == max_addr_array_len)) {
		return false;
	}

	memmove(p_addr_array + entry_size * (location + 1),
		p_addr_array + entry_size * (location),
		(*p_addr_array_len - location) * entry_size);

	memcpy(p_addr_array + entry_size * location, p_addr,
	       extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE);

	(*p_addr_array_len)++;

	return true;
}

/**
 * @brief Remove an address from the address list keeping it in ascending
 *        order.
 *
 * @param[in]  location     Index of the element to be removed from the list.
 * @param[in]  extended     Indication if address to remove is an extended or
 *                          a short address.
 *
 * @retval true  Address @p p_addr has been removed from the list successfully.
 * @retval false Address @p p_addr could not removed from the list.
 */
static bool addr_remove(uint32_t location, uint8_t data_type, bool extended)
{
	uint32_t *p_addr_array_len;
	uint8_t *p_addr_array;
	uint8_t entry_size;
	bool valid_data_type = true;

	switch (data_type) {
	case NRF_802154_ACK_DATA_PENDING_BIT:
		if (extended) {
			p_addr_array = (uint8_t *)m_pending_bit.extended_addr;
			p_addr_array_len = &m_pending_bit.num_of_ext_addr;
			entry_size = EXTENDED_ADDRESS_SIZE;
		} else {
			p_addr_array = (uint8_t *)m_pending_bit.short_addr;
			p_addr_array_len = &m_pending_bit.num_of_short_addr;
			entry_size = SHORT_ADDRESS_SIZE;
		}
		break;

	case NRF_802154_ACK_DATA_IE:
		if (extended) {
			p_addr_array = (uint8_t *)m_ie.ext_data;
			p_addr_array_len = &m_ie.num_of_ext_data;
			entry_size = sizeof(ack_ext_ie_data_t);
		} else {
			p_addr_array = (uint8_t *)m_ie.short_data;
			p_addr_array_len = &m_ie.num_of_short_data;
			entry_size = sizeof(ack_short_ie_data_t);
		}
		break;

	default:
		valid_data_type = false;
		assert(false);
		break;
	}

	if (!valid_data_type || (*p_addr_array_len == 0)) {
		return false;
	}

	memmove(p_addr_array + entry_size * location,
		p_addr_array + entry_size * (location + 1),
		(*p_addr_array_len - location - 1) * entry_size);

	(*p_addr_array_len)--;

	return true;
}

static void ie_data_add(uint32_t location, bool extended, const uint8_t *p_data,
			uint8_t data_len)
{
	if (extended) {
		memcpy(m_ie.ext_data[location].ie_data.p_data, p_data,
		       data_len);
		m_ie.ext_data[location].ie_data.len = data_len;
	} else {
		memcpy(m_ie.short_data[location].ie_data.p_data, p_data,
		       data_len);
		m_ie.short_data[location].ie_data.len = data_len;
	}
}

/*****************************************************************************
 * @section Public API
 *****************************************************************************/

void nrf_802154_ack_data_init(void)
{
	memset(&m_pending_bit, 0, sizeof(m_pending_bit));
	memset(&m_ie, 0, sizeof(m_ie));

	m_pending_bit.enabled = true;
	m_src_matching_method = NRF_802154_SRC_ADDR_MATCH_THREAD;
}

void nrf_802154_ack_data_enable(bool enabled)
{
	m_pending_bit.enabled = enabled;
}

bool nrf_802154_ack_data_for_addr_set(const uint8_t *p_addr, bool extended,
				      uint8_t data_type, const void *p_data,
				      uint8_t data_len)
{
	uint32_t location = 0;

	if (addr_index_find(p_addr, &location, data_type, extended) ||
	    addr_add(p_addr, location, data_type, extended)) {
		if (data_type == NRF_802154_ACK_DATA_IE) {
			ie_data_add(location, extended, p_data, data_len);
		}

		return true;
	} else {
		return false;
	}
}

bool nrf_802154_ack_data_for_addr_clear(const uint8_t *p_addr, bool extended,
					uint8_t data_type)
{
	uint32_t location = 0;

	if (addr_index_find(p_addr, &location, data_type, extended)) {
		return addr_remove(location, data_type, extended);
	} else {
		return false;
	}
}

void nrf_802154_ack_data_reset(bool extended, uint8_t data_type)
{
	switch (data_type) {
	case NRF_802154_ACK_DATA_PENDING_BIT:
		if (extended) {
			m_pending_bit.num_of_ext_addr = 0;
		} else {
			m_pending_bit.num_of_short_addr = 0;
		}
		break;

	case NRF_802154_ACK_DATA_IE:
		if (extended) {
			m_ie.num_of_ext_data = 0;
		} else {
			m_ie.num_of_short_data = 0;
		}
		break;

	default:
		break;
	}
}

void nrf_802154_ack_data_src_addr_matching_method_set(
	nrf_802154_src_addr_match_t match_method)
{
	switch (match_method) {
	case NRF_802154_SRC_ADDR_MATCH_THREAD:
	case NRF_802154_SRC_ADDR_MATCH_ZIGBEE:
	case NRF_802154_SRC_ADDR_MATCH_ALWAYS_1:
		m_src_matching_method = match_method;
		break;

	default:
		assert(false);
	}
}

bool nrf_802154_ack_data_pending_bit_should_be_set(const uint8_t *p_frame)
{
	bool ret;

	switch (m_src_matching_method) {
	case NRF_802154_SRC_ADDR_MATCH_THREAD:
		ret = addr_match_thread(p_frame);
		break;

	case NRF_802154_SRC_ADDR_MATCH_ZIGBEE:
		ret = addr_match_zigbee(p_frame);
		break;

	case NRF_802154_SRC_ADDR_MATCH_ALWAYS_1:
		ret = addr_match_standard_compliant(p_frame);
		break;

	default:
		assert(false);
	}

	return ret;
}

const uint8_t *nrf_802154_ack_data_ie_get(const uint8_t *p_src_addr,
					  bool src_addr_extended,
					  uint8_t *p_ie_length)
{
	uint32_t location;

	if (NULL == p_src_addr) {
		return NULL;
	}

	if (addr_index_find(p_src_addr, &location, NRF_802154_ACK_DATA_IE,
			    src_addr_extended)) {
		if (src_addr_extended) {
			*p_ie_length = m_ie.ext_data[location].ie_data.len;
			return m_ie.ext_data[location].ie_data.p_data;
		} else {
			*p_ie_length = m_ie.short_data[location].ie_data.len;
			return m_ie.short_data[location].ie_data.p_data;
		}
	} else {
		*p_ie_length = 0;
		return NULL;
	}
}
