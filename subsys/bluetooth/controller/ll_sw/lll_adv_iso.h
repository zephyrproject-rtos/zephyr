/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_iso_init(void);
int lll_adv_iso_reset(void);
void lll_adv_iso_create_prepare(void *param);
void lll_adv_iso_prepare(void *param);
void lll_adv_iso_resume(void *param);

extern struct lll_adv_iso_stream *ull_adv_iso_lll_stream_get(uint16_t handle);
extern struct lll_adv_iso *ull_adv_iso_lll_is_valid_get(struct lll_adv_iso *lll);

extern void ull_adv_iso_lll_biginfo_fill(struct pdu_adv *pdu, struct lll_adv_sync *lll_sync);
