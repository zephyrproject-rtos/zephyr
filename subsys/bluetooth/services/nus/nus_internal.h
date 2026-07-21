/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BLUETOOTH_SERVICES_NUS_INTERNAL_H_
#define ZEPHYR_BLUETOOTH_SERVICES_NUS_INTERNAL_H_

#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/bluetooth/services/nus/inst.h>

struct bt_nus_inst *bt_nus_inst_default(void);
struct bt_nus_inst *bt_nus_inst_get_from_attr(const struct bt_gatt_attr *attr);

#endif /* ZEPHYR_BLUETOOTH_SERVICES_NUS_INTERNAL_H_ */
