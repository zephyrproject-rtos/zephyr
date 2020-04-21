/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_aux_init(void);
int lll_adv_aux_reset(void);
void lll_adv_aux_prepare(void *param);
void lll_adv_aux_offset_fill(u32_t ticks_offset, u32_t start_us,
			     struct pdu_adv *pdu);


extern u8_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll);
