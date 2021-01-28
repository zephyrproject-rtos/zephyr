/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern struct bt_le_ext_adv *g_adv;
extern struct bt_le_adv_param g_param;

void common_setup(void);
void common_create_adv_set(void);
void common_delete_adv_set(void);
