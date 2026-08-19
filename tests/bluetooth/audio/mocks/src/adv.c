/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/atomic.h>

#include "bluetooth.h"

int bt_le_ext_adv_get_info(const struct bt_le_ext_adv *adv, struct bt_le_ext_adv_info *info)
{
	static bt_addr_le_t adv_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {1, 2, 3, 4, 5, 6},
	};

	if (adv == NULL) {
		return -EINVAL;
	}

	if (info == NULL) {
		return -EINVAL;
	}

	(void)memset(info, 0, sizeof(*info));

	info->id = 0;
	info->tx_power = 0;
	info->addr = &adv_addr;
	info->ext_adv_state = adv->ext_adv_state;
	info->per_adv_state = adv->per_adv_state;

	return 0;
}

int bt_le_ext_adv_start(struct bt_le_ext_adv *adv, const struct bt_le_ext_adv_start_param *param)
{
	if (adv == NULL) {
		return -EINVAL;
	}

	adv->ext_adv_state = BT_LE_EXT_ADV_STATE_ENABLED;

	return 0;
}
