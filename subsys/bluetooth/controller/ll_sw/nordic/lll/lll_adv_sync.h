/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_sync_init(void);
int lll_adv_sync_reset(void);
void lll_adv_sync_prepare(void *param);
void lll_adv_sync_offset_fill(u32_t ticks_offset, u32_t start_us,
			     struct pdu_adv *pdu);

extern u16_t ull_adv_sync_lll_handle_get(struct lll_adv_sync *lll);
