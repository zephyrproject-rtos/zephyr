/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>

/** Perform a single ATT_READ_BY_TYPE_REQ. */
int bt_testlib_att_read_by_type_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				     uint16_t *result_handle, struct bt_conn *conn,
				     enum bt_att_chan_opt bearer, const struct bt_uuid *type,
				     uint16_t start_handle, uint16_t end_handle);

/** If offset == 0, perform a single ATT_READ_REQ.
 * If offset > 0, perform a signle ATT_READ_BLOB_REQ.
 */
int bt_testlib_att_read_by_handle_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				       struct bt_conn *conn, enum bt_att_chan_opt bearer,
				       uint16_t handle, uint16_t offset);

int bt_testlib_gatt_discover_primary(uint16_t *result_handle, uint16_t *result_end_handle,
				     struct bt_conn *conn, const struct bt_uuid *uuid,
				     uint16_t start_handle, uint16_t end_handle);

int bt_testlib_gatt_discover_characteristic(uint16_t *const result_value_handle,
					    uint16_t *const result_end_handle,
					    uint16_t *const result_def_handle, struct bt_conn *conn,
					    const struct bt_uuid *uuid, uint16_t start_handle,
					    uint16_t svc_end_handle);
