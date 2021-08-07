/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include "ticker/ticker.h"

uint32_t ticker_update(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		       uint32_t ticks_drift_plus, uint32_t ticks_drift_minus,
		       uint32_t ticks_slot_plus, uint32_t ticks_slot_minus, uint16_t lazy,
		       uint8_t force, ticker_op_func fp_op_func, void *op_context)
{
	return TICKER_STATUS_SUCCESS;
}

uint32_t ticker_start(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		      uint32_t ticks_anchor, uint32_t ticks_first, uint32_t ticks_periodic,
		      uint32_t remainder_periodic, uint16_t lazy, uint32_t ticks_slot,
		      ticker_timeout_func fp_timeout_func, void *context, ticker_op_func fp_op_func,
		      void *op_context)
{
	return TICKER_STATUS_SUCCESS;
}

uint32_t ticker_stop(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		     ticker_op_func fp_op_func, void *op_context)
{
	return TICKER_STATUS_SUCCESS;
}
