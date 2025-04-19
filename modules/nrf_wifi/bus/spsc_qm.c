/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the
 * SPSC queue management layer of the nRF71 Wi-Fi driver.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#include "spsc_qm.h"

SPSC_QUEUE_T *SPSC32_init(uint32_t address, size_t size)
{
	return spsc_pbuf_init((void *)address, size, 0);
}

bool SPSC32_push(SPSC_QUEUE_T *pb, uint32_t value)
{
	char *pbuf;
	uint8_t len = sizeof(uint32_t);

	if (spsc_pbuf_alloc(pb, len, &pbuf) != len) {
		LOG_ERR("%s: Failed to allocate buffer", __func__);
		return false;
	}

	memcpy(pbuf, &value, len);
	spsc_pbuf_commit(pb, len);

	return true;
}

bool SPSC32_pop(SPSC_QUEUE_T *pb, uint32_t *outValue)
{
	char *buf;
	uint16_t plen = spsc_pbuf_claim(pb, &buf);

	if (plen == 0) {
		LOG_ERR("%s: Failed to claim buffer", __func__);
		return false;
	}

	spsc_pbuf_free(pb, plen);

	*outValue = *((uint32_t *)buf);

	return true;
}

bool SPSC32_readHead(SPSC_QUEUE_T *pb, uint32_t *outValue)
{
	char *buf;
	uint16_t plen = spsc_pbuf_claim(pb, &buf);

	if (plen == 0) {
		LOG_ERR("%s: Failed to claim buffer", __func__);
		return false;
	}

	*outValue = *((uint32_t *)buf);

	return true;
}

bool SPSC32_isEmpty(SPSC_QUEUE_T *pb)
{
	char *buf;

	return spsc_pbuf_claim(pb, &buf) == 0;
}

bool SPSC32_isFull(SPSC_QUEUE_T *pb)
{
	char *pbuf;
	uint8_t len = sizeof(uint32_t);

	return spsc_pbuf_alloc(pb, len, &pbuf) != len;
}
