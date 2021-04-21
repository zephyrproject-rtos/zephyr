/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_df_init(void);
int lll_df_reset(void);

/* Provides number of available antennae for Direction Finding */
uint8_t lll_df_ant_num_get(void);
