/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include "util/memq.h"
#include "util/mayfly.h"

void mayfly_init(void)
{
}

void mayfly_enable(uint8_t caller_id, uint8_t callee_id, uint8_t enable)
{
}

uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id)
{
	ARG_UNUSED(caller_id);
	ARG_UNUSED(callee_id);

	return 0;
}

uint32_t mayfly_enqueue(uint8_t caller_id, uint8_t callee_id, uint8_t chain, struct mayfly *m)
{
	return 0;
}

void mayfly_run(uint8_t callee_id)
{
}

uint32_t mayfly_is_running(void)
{
	return 0U;
}
