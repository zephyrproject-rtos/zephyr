/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_df_init(void);
int lll_df_reset(void);

/* Provides number of available antennae for Direction Finding */
uint8_t lll_df_ant_num_get(void);

/* Confirm if there is `count` of free IQ report node rx available and return
 * pointer to first (oldest) one.
 */
void *ull_df_iq_report_alloc_peek(uint8_t count);
/* Dequeue first (oldest) free IQ report node rx. If the node was peeked before
 * returned pointer equals to peeked one.
 */
void *ull_df_iq_report_alloc(void);
