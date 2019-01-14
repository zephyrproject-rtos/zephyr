/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/ticker_vendor_hal.h"

u8_t hal_ticker_instance0_caller_id_get(u8_t user_id);
void hal_ticker_instance0_sched(u8_t caller_id, u8_t callee_id, u8_t chain,
				void *instance);
void hal_ticker_instance0_trigger_set(u32_t value);
