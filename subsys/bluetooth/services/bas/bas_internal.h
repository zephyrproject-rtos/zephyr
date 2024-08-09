/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_BAS_INTERNAL_H_
#define BT_BAS_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/services/bas_bls.h>

/**
 * @brief   Battery Service data structure.
 */
typedef struct {
	/** The charge level of a battery. */
	uint8_t battery_level;
	/** The battery level status of a battery. */
	bt_bas_bls_t level_status;
} __packed bt_bas_data_t;

/**
 * @brief Initialize the Battery Level Status Module.
 *
 * @param battery_level_status Pointer to the battery level status structure.
 */
void bt_bas_bls_init(bt_bas_bls_t *battery_level_status);

/**
 * @brief Update the battery level status to connected devices.
 */
void update_battery_level_status(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_BAS_INTERNAL_H_ */
