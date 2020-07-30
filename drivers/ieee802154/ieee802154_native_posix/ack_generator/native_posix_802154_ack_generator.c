/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This module implements an acknowledgement generator for 802.15.4
 *   radio driver.
 *
 */

#include "native_posix_802154_ack_generator.h"

#include <assert.h>
#include <stdlib.h>

#include "../native_posix_802154_const.h"
#include "native_posix_802154_enh_ack_generator.h"
#include "native_posix_802154_imm_ack_generator.h"

typedef enum {
	FRAME_VERSION_BELOW_2015,
	FRAME_VERSION_2015_OR_ABOVE,
	FRAME_VERSION_INVALID
} frame_version_t;

static frame_version_t frame_version_is_2015_or_above(const uint8_t *p_frame)
{
	switch (p_frame[FRAME_VERSION_OFFSET] & FRAME_VERSION_MASK) {
	case FRAME_VERSION_0:
	case FRAME_VERSION_1:
		return FRAME_VERSION_BELOW_2015;

	case FRAME_VERSION_2:
		return FRAME_VERSION_2015_OR_ABOVE;

	default:
		return FRAME_VERSION_INVALID;
	}
}

void nrf_802154_ack_generator_init(void)
{
	/** Both generators are initialized to enable sending both Imm-Acks and
     *  Enh-Acks.
     */
	nrf_802154_imm_ack_generator_init();
	nrf_802154_enh_ack_generator_init();
}

const uint8_t *nrf_802154_ack_generator_create(const uint8_t *p_frame)
{
	/* This function should not be called if ACK is not requested. */
	assert(p_frame[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT);

	switch (frame_version_is_2015_or_above(p_frame)) {
	case FRAME_VERSION_BELOW_2015:
		return nrf_802154_imm_ack_generator_create(p_frame);

	case FRAME_VERSION_2015_OR_ABOVE:
		return nrf_802154_enh_ack_generator_create(p_frame);

	default:
		return NULL;
	}
}
