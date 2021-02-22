/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_OTS_DIR_LIST_INTERNAL_H_
#define BT_OTS_DIR_LIST_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>
#include "ots_internal.h"

void bt_ots_dir_list_obj_add(struct bt_ots *ots, struct bt_gatt_ots_object *obj);
void bt_ots_dir_list_obj_remove(struct bt_ots *ots, struct bt_gatt_ots_object *obj);
void bt_ots_dir_list_selected(struct bt_ots *ots);
void bt_ots_dir_list_init(struct bt_ots *ots);
int bt_ots_dir_list_content_get(struct bt_ots *ots, uint8_t **data,
				uint32_t len, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* BT_OTS_DIR_LIST_INTERNAL_H_ */
