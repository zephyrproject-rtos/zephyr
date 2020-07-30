/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements storage of PIB attributes in nRF 802.15.4
 *   radio driver.
 *
 */

#include "native_posix_802154_pib.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "native_posix_802154_config.h"
#include "native_posix_802154_const.h"
#include "native_posix_802154_utils.h"
#include <bs_radio/bs_radio.h>

typedef struct {
	/** Transmit power. */
	int8_t tx_power;

	/** Pan Id of this node. */
	uint8_t pan_id[PAN_ID_SIZE];

	/** Short Address of this node. */
	uint8_t short_addr[SHORT_ADDRESS_SIZE];

	/** Extended Address of this node. */
	uint8_t extended_addr[EXTENDED_ADDRESS_SIZE];

	/** Indicating if radio is in promiscuous mode. */
	bool promiscuous : 1;

	/** Indicating if auto ACK procedure is enabled. */
	bool auto_ack : 1;

	/** Indicating if radio is configured as the PAN coordinator. */
	bool pan_coord : 1;

	/** Channel on which the node receives messages. */
	uint8_t channel : 5;
} nrf_802154_pib_data_t;

/** Buffer containing PIB data. */
static nrf_802154_pib_data_t m_data;

void nrf_802154_pib_init(void)
{
	m_data.promiscuous = false;
	m_data.auto_ack = true;
	m_data.pan_coord = false;
	m_data.channel = 11;

	memset(m_data.pan_id, 0xff, sizeof(m_data.pan_id));
	m_data.short_addr[0] = 0xfe;
	m_data.short_addr[1] = 0xff;
	memset(m_data.extended_addr, 0, sizeof(m_data.extended_addr));
}

bool nrf_802154_pib_promiscuous_get(void)
{
	return m_data.promiscuous;
}

void nrf_802154_pib_promiscuous_set(bool enabled)
{
	m_data.promiscuous = enabled;
}

bool nrf_802154_pib_auto_ack_get(void)
{
	return m_data.auto_ack;
}

void nrf_802154_pib_auto_ack_set(bool enabled)
{
	m_data.auto_ack = enabled;
}

bool nrf_802154_pib_pan_coord_get(void)
{
	return m_data.pan_coord;
}

void nrf_802154_pib_pan_coord_set(bool enabled)
{
	m_data.pan_coord = enabled;
}

uint8_t nrf_802154_pib_channel_get(void)
{
	return m_data.channel;
}

void nrf_802154_pib_channel_set(uint8_t channel)
{
	m_data.channel = channel;
}

void nrf_802154_pib_tx_power_set(int8_t dbm)
{
	m_data.tx_power = dbm;
}

const uint8_t *nrf_802154_pib_pan_id_get(void)
{
	return m_data.pan_id;
}

void nrf_802154_pib_pan_id_set(const uint8_t *p_pan_id)
{
	memcpy(m_data.pan_id, p_pan_id, PAN_ID_SIZE);
}

const uint8_t *nrf_802154_pib_extended_address_get(void)
{
	return m_data.extended_addr;
}

void nrf_802154_pib_extended_address_set(const uint8_t *p_extended_address)
{
	memcpy(m_data.extended_addr, p_extended_address,
	       EXTENDED_ADDRESS_SIZE);
}

const uint8_t *nrf_802154_pib_short_address_get(void)
{
	return m_data.short_addr;
}

void nrf_802154_pib_short_address_set(const uint8_t *p_short_address)
{
	memcpy(m_data.short_addr, p_short_address, SHORT_ADDRESS_SIZE);
}
