/* @file
 * @brief Media Control Service internal header file
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MTS_INTERNAL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MTS_INTERNAL_H_

#include <stdbool.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include "uint48_util.h"

#ifdef __cplusplus
extern "C" {
#endif

int bt_mcs_init(struct bt_ots_cb *ots_cbs);

struct ots_svc_inst_t *bt_mcs_get_ots(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_OTS_INTERNAL_H_ */
