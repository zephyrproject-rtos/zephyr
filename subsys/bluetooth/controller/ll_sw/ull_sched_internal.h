/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_sched_adv_aux_sync_free_anchor_get(uint32_t ticks_slot_abs,
					   uint32_t *ticks_anchor);
int ull_sched_conn_iso_free_offset_get(uint32_t ticks_slot_abs,
				       uint32_t *ticks_to_expire);
int ull_sched_after_cen_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				 uint32_t *ticks_anchor, uint32_t *us_offset);
void ull_sched_mfy_win_offset_use(void *param);
void ull_sched_mfy_free_win_offset_calc(void *param);
void ull_sched_mfy_win_offset_select(void *param);
