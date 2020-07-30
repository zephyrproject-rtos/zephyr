/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements an immediate acknowledgement (Imm-Ack) generator for
 *   802.15.4 radio driver.
 *
 */

#include "native_posix_802154_imm_ack_generator.h"

#include <assert.h>
#include <string.h>

#include "native_posix_802154_ack_data.h"
#include "../native_posix_802154_const.h"

#define IMM_ACK_INITIALIZER                                                    \
	{                                                                      \
		0x05, ACK_HEADER_WITH_PENDING, 0x00, 0x00, 0x00, 0x00          \
	}

static uint8_t m_ack_data[IMM_ACK_LENGTH + PHR_SIZE];

void nrf_802154_imm_ack_generator_init(void)
{
	const uint8_t ack_data[] = IMM_ACK_INITIALIZER;

	memcpy(m_ack_data, ack_data, sizeof(ack_data));
}

const uint8_t *nrf_802154_imm_ack_generator_create(const uint8_t *p_frame)
{
	/* Set valid sequence number in ACK frame. */
	m_ack_data[DSN_OFFSET] = p_frame[DSN_OFFSET];

	/* Set pending bit in ACK frame. */
	if (nrf_802154_ack_data_pending_bit_should_be_set(p_frame)) {
		m_ack_data[FRAME_PENDING_OFFSET] = ACK_HEADER_WITH_PENDING;
	} else {
		m_ack_data[FRAME_PENDING_OFFSET] = ACK_HEADER_WITHOUT_PENDING;
	}

	return m_ack_data;
}
