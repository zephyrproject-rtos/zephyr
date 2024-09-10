/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_BT_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_BT_H_

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/types.h>
struct bt_conn;

#ifdef __cplusplus
extern "C" {
#endif

/** SMP service UUID value. */
#define SMP_BT_SVC_UUID_VAL \
	BT_UUID_128_ENCODE(0x8d53dc1d, 0x1db7, 0x4cd3, 0x868b, 0x8a527460aa84)

/** SMP service UUID. */
#define SMP_BT_SVC_UUID \
	BT_UUID_DECLARE_128(SMP_BT_SVC_UUID_VAL)

/** SMP characteristic UUID value. */
#define SMP_BT_CHR_UUID_VAL \
	BT_UUID_128_ENCODE(0xda2e7828, 0xfbce, 0x4e01, 0xae9e, 0x261174997c48)

/** SMP characteristic UUID
 *  Used for both requests and responses.
 */
#define SMP_BT_CHR_UUID \
	BT_UUID_DECLARE_128(SMP_BT_CHR_UUID_VAL)

/**
 * @brief	Registers the SMP Bluetooth service. Should only be called if the Bluetooth
 *		transport has been unregistered by calling smp_bt_unregister().
 *
 * @return	0 on success; negative error code on failure.
 */
int smp_bt_register(void);

/**
 * @brief	Unregisters the SMP Bluetooth service.
 *
 * @return	0 on success; negative error code on failure.
 */
int smp_bt_unregister(void);

/**
 * @brief	Transmits an SMP command/response over the specified Bluetooth connection as a
 *		notification.
 *
 * @param conn	Connection object.
 * @param data	Pointer to SMP message.
 * @param len	data length.
 *
 * @return	0 in case of success or negative value in case of error.
 */
int smp_bt_notify(struct bt_conn *conn, const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
