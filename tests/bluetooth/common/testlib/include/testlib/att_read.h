/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_READ_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_READ_H_

#include <stdint.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>

/** Perform a single ATT_READ_BY_TYPE_REQ. */
int bt_testlib_att_read_by_type_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				     uint16_t *result_handle, uint16_t *result_att_mtu,
				     struct bt_conn *conn, enum bt_att_chan_opt bearer,
				     const struct bt_uuid *type, uint16_t start_handle,
				     uint16_t end_handle);

/** If offset == 0, perform a single ATT_READ_REQ.
 * If offset > 0, perform a signle ATT_READ_BLOB_REQ.
 */
int bt_testlib_att_read_by_handle_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				       uint16_t *result_att_mtu, struct bt_conn *conn,
				       enum bt_att_chan_opt bearer, uint16_t handle,
				       uint16_t offset);

int bt_testlib_gatt_long_read(struct net_buf_simple *result_data, uint16_t *result_size,
			      uint16_t *result_att_mtu, struct bt_conn *conn,
			      enum bt_att_chan_opt bearer, uint16_t handle, uint16_t offset);

int bt_testlib_gatt_discover_primary(uint16_t *result_handle, uint16_t *result_end_handle,
				     struct bt_conn *conn, const struct bt_uuid *uuid,
				     uint16_t start_handle, uint16_t end_handle);

/* Note: svc_end_handle must be the service end handle. (The discovery
 * algorithm requires it to recognize the last characteristic in a
 * service and deduce its end handle.)
 */
int bt_testlib_gatt_discover_characteristic(uint16_t *const result_value_handle,
					    uint16_t *const result_end_handle,
					    uint16_t *const result_def_handle, struct bt_conn *conn,
					    const struct bt_uuid *uuid, uint16_t start_handle,
					    uint16_t svc_end_handle);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_READ_H_ */
