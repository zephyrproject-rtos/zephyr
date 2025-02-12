/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_aux_init(void);
int lll_adv_aux_reset(void);
void lll_adv_aux_prepare(void *param);

extern uint8_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll);
extern struct pdu_adv_aux_ptr *
	ull_adv_aux_lll_offset_fill(struct pdu_adv *pdu, uint32_t ticks_offset,
				    uint32_t remainder_us, uint32_t start_us);

extern void ull_adv_aux_lll_auxptr_fill(struct pdu_adv *pdu, struct lll_adv *lll_adv);
