/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_le_adv_resume(void);

struct bt_le_ext_adv *bt_le_adv_lookup_legacy(void);

void bt_le_adv_delete_legacy(void);
int bt_le_adv_set_enable(struct bt_le_ext_adv *adv, bool enable);

void bt_le_ext_adv_foreach(void (*func)(struct bt_le_ext_adv *adv, void *data),
			   void *data);

int bt_le_adv_set_enable_ext(struct bt_le_ext_adv *adv,
			 bool enable,
			 const struct bt_le_ext_adv_start_param *param);
int bt_le_adv_set_enable_legacy(struct bt_le_ext_adv *adv, bool enable);
int bt_le_lim_adv_cancel_timeout(struct bt_le_ext_adv *adv);
