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

int bt_mcs_init(struct bt_ots_cb *ots_cbs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_AUDIO_MCS_INTERNAL_H_ */
