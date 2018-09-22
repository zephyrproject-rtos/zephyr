/** @file
 *  @brief Internal API for Generic Attribute Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_GATT_CENTRAL_ADDR_RES_NOT_SUPP	0
#define BT_GATT_CENTRAL_ADDR_RES_SUPP		1

void bt_gatt_init(void);
void bt_gatt_connected(struct bt_conn *conn);
void bt_gatt_disconnected(struct bt_conn *conn);

int bt_gatt_store_ccc(u8_t id, const bt_addr_le_t *addr);
int bt_gatt_clear_ccc(u8_t id, const bt_addr_le_t *addr);

#if defined(CONFIG_BT_GATT_CLIENT)
void bt_gatt_notification(struct bt_conn *conn, u16_t handle,
			  const void *data, u16_t length);
#else
static inline void bt_gatt_notification(struct bt_conn *conn, u16_t handle,
					const void *data, u16_t length)
{
}
#endif /* CONFIG_BT_GATT_CLIENT */
