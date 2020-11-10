/** @file
 *  @brief DIS user function prototypes.
 */

/*
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_DIS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_DIS_H_

#include <bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t bt_dis_serial_number(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset);

ssize_t bt_dis_fw_rev(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset);

ssize_t bt_dis_hw_rev(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset);

ssize_t bt_dis_sw_rev(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_DIS_H_ */
