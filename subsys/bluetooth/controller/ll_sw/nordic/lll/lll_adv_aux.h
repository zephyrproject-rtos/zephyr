/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_aux_init(void);
int lll_adv_aux_reset(void);
void lll_adv_aux_prepare(void *param);
void lll_adv_aux_offset_fill(uint32_t ticks_offset, uint32_t start_us,
			     struct pdu_adv *pdu);


extern uint16_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll);
