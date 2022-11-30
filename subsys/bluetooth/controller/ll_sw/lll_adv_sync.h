/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_sync_init(void);
int lll_adv_sync_reset(void);
void lll_adv_sync_prepare(void *param);

extern uint16_t ull_adv_sync_lll_handle_get(struct lll_adv_sync *lll);

extern void ull_adv_sync_lll_syncinfo_fill(struct pdu_adv *pdu, struct lll_adv_aux *lll_aux);
