/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/ticker_vendor_hal.h"

uint8_t hal_ticker_instance0_caller_id_get(uint8_t user_id);
void hal_ticker_instance0_sched(uint8_t caller_id, uint8_t callee_id, uint8_t chain,
				void *instance);
void hal_ticker_instance0_trigger_set(uint32_t value);
