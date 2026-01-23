/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HAL_TICKER_H_MOCK
#define HAL_TICKER_H_MOCK

#include <stdint.h>

/* Mock HAL ticker interface for testing */

static inline uint8_t hal_ticker_instance0_caller_id_get(uint8_t user_id)
{
	/* Return TICKER_CALL_ID_PROGRAM (5) as default caller ID */
	return 5;
}

static inline void hal_ticker_instance0_sched(uint8_t caller_id, uint8_t callee_id,
					      uint8_t chain, void *instance)
{
	/* Mock scheduler - no-op */
}

static inline void hal_ticker_instance0_trigger_set(uint32_t value)
{
	/* Mock trigger set - no-op */
}

#endif /* HAL_TICKER_H_MOCK */
