/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <toolchain.h>

#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_scan.h"

#include "ull_scan_types.h"

void ull_sched_after_mstr_slot_get(u8_t user_id, u32_t ticks_slot_abs,
				   u32_t *ticks_anchor, u32_t *us_offset)
{
	/* TODO: */
}

void ull_sched_mfy_after_mstr_offset_get(void *param)
{
	struct ll_scan_set *scan = param;

	/* TODO: */
	scan->lll.conn_win_offset_us = 0;
}

void ull_sched_mfy_free_win_offset_calc(void *param)
{
	/* TODO: */
}

void ull_sched_mfy_win_offset_use(void *param)
{
	/* TODO: */
}

void ull_sched_mfy_win_offset_select(void *param)
{
	/* TODO: */
}
