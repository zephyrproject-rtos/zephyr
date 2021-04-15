/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern struct bt_le_ext_adv *g_adv;
extern struct bt_le_adv_param g_param;
extern uint8_t g_cte_len;

void common_setup(void);
void common_create_adv_set(void);
void common_delete_adv_set(void);
void common_delete_adv_set(void);
void common_set_cte_params(void);
void common_set_cl_cte_tx_params(void);
void common_set_adv_params(void);
void common_per_adv_enable(void);
void common_per_adv_disable(void);
