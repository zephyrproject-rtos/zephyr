/** @file
 *  @brief Internal API for Generic Attribute Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_gatt_init(void);
void bt_gatt_connected(struct bt_conn *conn);
void bt_gatt_disconnected(struct bt_conn *conn);

#if defined(CONFIG_BT_GATT_CLIENT)
void bt_gatt_notification(struct bt_conn *conn, u16_t handle,
			  const void *data, u16_t length);
#else
static inline void bt_gatt_notification(struct bt_conn *conn, u16_t handle,
					const void *data, u16_t length)
{
}
#endif /* CONFIG_BT_GATT_CLIENT */
