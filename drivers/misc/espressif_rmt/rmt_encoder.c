/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rmt_private.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_encoder, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

#include <zephyr/drivers/misc/espressif_rmt/rmt_encoder.h>

int rmt_del_encoder(rmt_encoder_handle_t encoder)
{
	if (!encoder) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return encoder->del(encoder);
}

int rmt_encoder_reset(rmt_encoder_handle_t encoder)
{
	if (!encoder) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return encoder->reset(encoder);
}

void *rmt_alloc_encoder_mem(size_t size)
{
	return heap_caps_calloc(1, size, RMT_MEM_ALLOC_CAPS);
}
