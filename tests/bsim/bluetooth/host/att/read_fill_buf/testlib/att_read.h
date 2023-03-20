/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>

int bt_testlib_att_read_by_type_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				     uint16_t *result_handle, struct bt_conn *conn,
				     enum bt_att_chan_opt bearer, const struct bt_uuid *type,
				     uint16_t start_handle, uint16_t end_handle);

int bt_testlib_att_read_by_handle_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				       struct bt_conn *conn, enum bt_att_chan_opt bearer,
				       uint16_t handle, uint16_t offset);
