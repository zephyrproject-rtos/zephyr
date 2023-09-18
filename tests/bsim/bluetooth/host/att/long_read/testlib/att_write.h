/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>

int bt_testlib_att_write(struct bt_conn *conn, enum bt_att_chan_opt bearer, uint16_t handle,
			 const uint8_t *data, uint16_t size);
