/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_sched_after_mstr_slot_get(u8_t user_id, u32_t ticks_slot_abs,
				   u32_t *ticks_anchor, u32_t *us_offset);
void ull_sched_mfy_after_mstr_offset_get(void *param);
void ull_sched_mfy_free_win_offset_calc(void *param);
void ull_sched_mfy_win_offset_use(void *param);
void ull_sched_mfy_win_offset_select(void *param);
