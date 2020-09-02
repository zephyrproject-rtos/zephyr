/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include "ticker/ticker.h"


uint32_t ticker_update(uint8_t instance_index, uint8_t user_id,
		       uint8_t ticker_id, uint32_t ticks_drift_plus,
		       uint32_t ticks_drift_minus,
		       uint32_t ticks_slot_plus, uint32_t ticks_slot_minus,
		       uint16_t lazy, uint8_t force, ticker_op_func fp_op_func,
		       void *op_context)
{
	return 0;
}

uint32_t ticker_stop(uint8_t instance_index, uint8_t user_id,
		     uint8_t ticker_id, ticker_op_func fp_op_func,
		     void *op_context)
{
	return 0;
}
