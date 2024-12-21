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

spsc_queue_t *spsc32_init(uint32_t address, size_t size)
{
	return spsc_pbuf_init((void *)address, size, 0);
}

bool spsc32_push(spsc_queue_t *queue, uint32_t value)
{
	char *pbuf;
	uint8_t len = sizeof(uint32_t);

	if (spsc_pbuf_alloc(queue, len, &pbuf) != len) {
		LOG_ERR("%s: Failed to allocate buffer", __func__);
		return false;
	}

	memcpy(pbuf, &value, len);
	spsc_pbuf_commit(queue, len);

	return true;
}

bool spsc32_pop(spsc_queue_t *queue, uint32_t *out_value)
{
	char *buf;
	uint16_t plen = spsc_pbuf_claim(queue, &buf);

	if (plen == 0) {
		LOG_ERR("%s: Failed to claim buffer", __func__);
		return false;
	}

	spsc_pbuf_free(queue, plen);

	*out_value = *((uint32_t *)buf);

	return true;
}

bool spsc32_read_head(spsc_queue_t *queue, uint32_t *out_value)
{
	char *buf;
	uint16_t plen = spsc_pbuf_claim(queue, &buf);

	if (plen == 0) {
		LOG_ERR("%s: Failed to claim buffer", __func__);
		return false;
	}

	*out_value = *((uint32_t *)buf);

	return true;
}

bool spsc32_is_empty(spsc_queue_t *queue)
{
	char *buf;

	return spsc_pbuf_claim(queue, &buf) == 0;
}

bool spsc32_is_full(spsc_queue_t *queue)
{
	char *pbuf;
	uint8_t len = sizeof(uint32_t);

	return spsc_pbuf_alloc(queue, len, &pbuf) != len;
}
