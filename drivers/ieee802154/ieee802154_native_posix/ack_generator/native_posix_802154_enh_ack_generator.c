/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements an enhanced acknowledgement (Enh-Ack) generator for
 *   802.15.4 radio driver.
 *
 */

#include "native_posix_802154_enh_ack_generator.h"

#include <assert.h>
#include <string.h>

#include "../native_posix_802154_frame_parser.h"
#include "native_posix_802154_ack_data.h"
#include "../native_posix_802154_const.h"
#include "../native_posix_802154_pib.h"

#define ENH_ACK_MAX_SIZE MAX_PACKET_SIZE

static uint8_t m_ack_data[ENH_ACK_MAX_SIZE + PHR_SIZE];

static void ack_buffer_clear(void)
{
	memset(m_ack_data, 0, FCF_SIZE + PHR_SIZE);
}

static void sequence_number_set(const uint8_t *p_frame)
{
	if (!nrf_802154_frame_parser_dsn_suppress_bit_is_set(p_frame)) {
		m_ack_data[DSN_OFFSET] = p_frame[DSN_OFFSET];
	}
}

/******************************************************************************
 * @section Frame control field functions
 *****************************************************************************/

static void fcf_frame_type_set(void)
{
	m_ack_data[FRAME_TYPE_OFFSET] |= FRAME_TYPE_ACK;
}

static void fcf_security_enabled_set(const uint8_t *p_frame)
{
	m_ack_data[SECURITY_ENABLED_OFFSET] |=
		(p_frame[SECURITY_ENABLED_OFFSET] & SECURITY_ENABLED_BIT);
}

static void fcf_frame_pending_set(const uint8_t *p_frame)
{
	if (nrf_802154_ack_data_pending_bit_should_be_set(p_frame)) {
		m_ack_data[FRAME_PENDING_OFFSET] |= FRAME_PENDING_BIT;
	}
}

static void fcf_panid_compression_set(const uint8_t *p_frame)
{
	if (p_frame[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK) {
		m_ack_data[PAN_ID_COMPR_OFFSET] |= PAN_ID_COMPR_MASK;
	}
}

static void fcf_sequence_number_suppression_set(const uint8_t *p_frame)
{
	if (nrf_802154_frame_parser_dsn_suppress_bit_is_set(p_frame)) {
		m_ack_data[DSN_SUPPRESS_OFFSET] |= DSN_SUPPRESS_BIT;
	}
}

static void fcf_ie_present_set(const uint8_t *p_frame,
			       const uint8_t *p_ie_data)
{
	if (p_ie_data != NULL) {
		m_ack_data[IE_PRESENT_OFFSET] |= IE_PRESENT_BIT;
	}
}

static void fcf_dst_addressing_mode_set(const uint8_t *p_frame)
{
	if (nrf_802154_frame_parser_src_addr_is_extended(p_frame)) {
		m_ack_data[DEST_ADDR_TYPE_OFFSET] |= DEST_ADDR_TYPE_EXTENDED;
	} else if (nrf_802154_frame_parser_src_addr_is_short(p_frame)) {
		m_ack_data[DEST_ADDR_TYPE_OFFSET] |= DEST_ADDR_TYPE_SHORT;
	} else {
		m_ack_data[DEST_ADDR_TYPE_OFFSET] |= DEST_ADDR_TYPE_NONE;
	}
}

static void fcf_src_addressing_mode_set(const uint8_t *p_frame)
{
	m_ack_data[SRC_ADDR_TYPE_OFFSET] |= SRC_ADDR_TYPE_NONE;
}

static void fcf_frame_version_set(void)
{
	m_ack_data[FRAME_VERSION_OFFSET] |= FRAME_VERSION_2;
}

static void frame_control_set(const uint8_t *p_frame, const uint8_t *p_ie_data,
			      nrf_802154_frame_parser_mhr_data_t *p_ack_offsets)
{
	bool parse_results;

	fcf_frame_type_set();
	fcf_security_enabled_set(p_frame);
	fcf_frame_pending_set(p_frame);
	fcf_panid_compression_set(p_frame);
	fcf_sequence_number_suppression_set(p_frame);
	fcf_ie_present_set(p_frame, p_ie_data);
	fcf_dst_addressing_mode_set(p_frame);
	fcf_frame_version_set();
	fcf_src_addressing_mode_set(p_frame);

	parse_results =
		nrf_802154_frame_parser_mhr_parse(m_ack_data, p_ack_offsets);
	assert(parse_results);
	(void)parse_results;

	m_ack_data[PHR_OFFSET] =
		p_ack_offsets->addressing_end_offset - PHR_SIZE + FCS_SIZE;
}

/******************************************************************************
 * @section Addressing fields functions
 *****************************************************************************/

static void destination_set(const nrf_802154_frame_parser_mhr_data_t *p_frame,
			    const nrf_802154_frame_parser_mhr_data_t *p_ack)
{
	/* Fill the Ack destination PAN ID field. */
	if (p_ack->p_dst_panid != NULL) {
		const uint8_t *p_dst_panid;

		if (p_frame->p_src_panid != NULL) {
			p_dst_panid = p_frame->p_src_panid;
		} else if (p_frame->p_dst_panid != NULL) {
			p_dst_panid = p_frame->p_dst_panid;
		} else {
			p_dst_panid = nrf_802154_pib_pan_id_get();
		}

		memcpy((uint8_t *)p_ack->p_dst_panid, p_dst_panid, PAN_ID_SIZE);
	}

	/* Fill the Ack destination address field. */
	if (p_frame->p_src_addr != NULL) {
		assert(p_ack->p_dst_addr != NULL);
		assert(p_ack->dst_addr_size == p_frame->src_addr_size);

		memcpy((uint8_t *)p_ack->p_dst_addr, p_frame->p_src_addr,
		       p_frame->src_addr_size);
	}
}

static void source_set(const uint8_t *p_frame)
{
	/* Intentionally empty: source address type is None. */
}

/******************************************************************************
 * @section Auxiliary security header functions
 *****************************************************************************/

static void
security_control_set(const nrf_802154_frame_parser_mhr_data_t *p_frame,
		     const nrf_802154_frame_parser_mhr_data_t *p_ack)
{
	assert(p_frame->p_sec_ctrl != NULL);

	/* All the bits in the security control byte can be copied. */
	*(uint8_t *)p_ack->p_sec_ctrl = *p_frame->p_sec_ctrl;

	m_ack_data[PHR_OFFSET] += SECURITY_CONTROL_SIZE;
}

static void
security_key_id_set(const nrf_802154_frame_parser_mhr_data_t *p_frame,
		    const nrf_802154_frame_parser_mhr_data_t *p_ack,
		    bool fc_suppresed, const uint8_t **p_sec_end)
{
	const uint8_t *p_frame_key_id;
	const uint8_t *p_ack_key_id;
	uint8_t key_id_mode_size = 0;

	p_frame_key_id = p_frame->p_sec_ctrl + SECURITY_CONTROL_SIZE;
	p_ack_key_id = p_ack->p_sec_ctrl + SECURITY_CONTROL_SIZE;

	if (!fc_suppresed) {
		p_frame_key_id += FRAME_COUNTER_SIZE;
		p_ack_key_id += FRAME_COUNTER_SIZE;
	}

	switch ((*p_ack->p_sec_ctrl) & KEY_ID_MODE_MASK) {
	case KEY_ID_MODE_1:
		key_id_mode_size = KEY_ID_MODE_1_SIZE;
		break;

	case KEY_ID_MODE_2:
		key_id_mode_size = KEY_ID_MODE_2_SIZE;
		break;

	case KEY_ID_MODE_3:
		key_id_mode_size = KEY_ID_MODE_3_SIZE;
		break;

	default:
		break;
	}

	if (0 != key_id_mode_size) {
		memcpy((uint8_t *)p_ack_key_id, p_frame_key_id,
		       key_id_mode_size);
		m_ack_data[PHR_OFFSET] += key_id_mode_size;
	}

	switch (*(p_ack->p_sec_ctrl) & SECURITY_LEVEL_MASK) {
	case SECURITY_LEVEL_MIC_32:
	case SECURITY_LEVEL_ENC_MIC_32:
		m_ack_data[PHR_OFFSET] += MIC_32_SIZE;
		break;

	case SECURITY_LEVEL_MIC_64:
	case SECURITY_LEVEL_ENC_MIC_64:
		m_ack_data[PHR_OFFSET] += MIC_64_SIZE;
		break;

	case SECURITY_LEVEL_MIC_128:
	case SECURITY_LEVEL_ENC_MIC_128:
		m_ack_data[PHR_OFFSET] += MIC_128_SIZE;
		break;

	default:
		break;
	}

	*p_sec_end = p_ack_key_id + key_id_mode_size;
}

static void
security_header_set(const nrf_802154_frame_parser_mhr_data_t *p_frame,
		    const nrf_802154_frame_parser_mhr_data_t *p_ack,
		    const uint8_t **p_sec_end)
{
	bool fc_suppressed;

	if (p_ack->p_sec_ctrl == NULL) {
		*p_sec_end = &m_ack_data[p_ack->addressing_end_offset];
		return;
	}

	security_control_set(p_frame, p_ack);

	/* Frame counter is set by MAC layer when the frame is encrypted. */
	fc_suppressed = ((*p_ack->p_sec_ctrl) & FRAME_COUNTER_SUPPRESS_BIT);

	if (!fc_suppressed) {
		m_ack_data[PHR_OFFSET] += FRAME_COUNTER_SIZE;
	}

	security_key_id_set(p_frame, p_ack, fc_suppressed, p_sec_end);
}

/******************************************************************************
 * @section Information Elements
 *****************************************************************************/

static void ie_header_set(const uint8_t *p_ie_data, uint8_t ie_data_len,
			  const uint8_t *p_sec_end)
{
	uint8_t *p_ack_ie = (uint8_t *)p_sec_end;

	if (p_ie_data == NULL) {
		return;
	}

	assert(p_ack_ie != NULL);

	memcpy(p_ack_ie, p_ie_data, ie_data_len);
	m_ack_data[PHR_OFFSET] += ie_data_len;
}

/******************************************************************************
 * @section Public API implementation
 *****************************************************************************/

void nrf_802154_enh_ack_generator_init(void)
{
	/* Intentionally empty. */
}

const uint8_t *nrf_802154_enh_ack_generator_create(const uint8_t *p_frame)
{
	nrf_802154_frame_parser_mhr_data_t frame_offsets;
	nrf_802154_frame_parser_mhr_data_t ack_offsets;
	const uint8_t *p_sec_end = NULL;
	bool parse_result =
		nrf_802154_frame_parser_mhr_parse(p_frame, &frame_offsets);

	if (!parse_result) {
		return NULL;
	}

	uint8_t ie_data_len;
	const uint8_t *p_ie_data = nrf_802154_ack_data_ie_get(
		frame_offsets.p_src_addr,
		frame_offsets.src_addr_size == EXTENDED_ADDRESS_SIZE,
		&ie_data_len);

	/* Clear previously created ACK. */
	ack_buffer_clear();

	/* Set Frame Control field bits. */
	frame_control_set(p_frame, p_ie_data, &ack_offsets);

	/* Set valid sequence number in ACK frame. */
	sequence_number_set(p_frame);

	/* Set destination address and PAN ID. */
	destination_set(&frame_offsets, &ack_offsets);

	/* Set source address and PAN ID. */
	source_set(p_frame);

	/* Set auxiliary security header. */
	security_header_set(&frame_offsets, &ack_offsets, &p_sec_end);

	/* Set IE header. */
	ie_header_set(p_ie_data, ie_data_len, p_sec_end);

	return m_ack_data;
}
