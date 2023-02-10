/* @file
 * @brief Media Control Service internal header file
 *
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_AUDIO_MCS_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_AUDIO_MCS_INTERNAL_H_

#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This differs from BT_OTS_VALID_OBJ_ID as MCS does not use the directory list object */
#define BT_MCS_VALID_OBJ_ID(id) (IN_RANGE((id), BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX))

int bt_mcs_init(struct bt_ots_cb *ots_cbs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_AUDIO_MCS_INTERNAL_H_ */
