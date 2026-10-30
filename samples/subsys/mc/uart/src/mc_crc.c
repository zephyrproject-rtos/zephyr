/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

#include "alg/algorithm.h"

mc_u32 mc_alg_crc16_ccitt(mc_buffer buffer)
{
	if (mc_buffer_is_null(buffer)) {
		return mc_u32(0U, MC_ERR_INVALID_ARGUMENT);
	}

	return mc_u32(crc16_itu_t(0xffffU, (const uint8_t *)buffer.data,
				  buffer.capacity * buffer.data_size), MC_SUCCESS);
}
