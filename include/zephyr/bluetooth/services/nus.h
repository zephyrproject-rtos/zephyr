/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_H_

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/nus/inst.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUIDs of Nordic UART GATT Service.
 * Service: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
 * RX Char: 6e400002-b5a3-f393-e0a9-e50e24dcca9e
 * TX Char: 6e400003-b5a3-f393-e0a9-e50e24dcca9e
 */
#define BT_UUID_NUS_SRV_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS_RX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS_TX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

/** @brief Macro to define instance of NUS Service. It allows users to
 * define multiple NUS instances, analogous to Serial endpoints, and use each
 * one for different purposes. A default NUS instance may be defined through
 * Kconfig.
 */
#define BT_NUS_INST_DEFINE(_name) \
	Z_INTERNAL_BT_NUS_INST_DEFINE(_name)

/** @brief Callbacks for getting notified on NUS Service occurrences */
struct bt_nus_cb {
	/** @brief Notifications subscription changed
	 *
	 * @param enabled Flag that is true if notifications were enabled, false
	 *                if they were disabled.
	 * @param ctx User context provided in the callback structure.
	 */
	void (*notif_enabled)(bool enabled, void *ctx);

	/** @brief Received Data
	 *
	 * @param conn Peer Connection object.
	 * @param data Pointer to buffer with data received.
	 * @param len Size in bytes of data received.
	 * @param ctx User context provided in the callback structure.
	 */
	void (*received)(struct bt_conn *conn, const void *data, uint16_t len, void *ctx);

	/** Internal member. Provided as a callback argument for user context */
	void *ctx;

	/** Internal member to form a list of callbacks */
	sys_snode_t _node;
};

/** @brief NUS server Instance callback register
 *
 * This function registers callbacks that will be called in
 * certain events related to NUS.
 *
 * @param inst Pointer to instance of NUS service. NULL if using default instance.
 * @param cb Pointer to callbacks structure. Must be valid throughout the
 *           lifetime of the application.
 * @param ctx User context to be provided through the callback.
 *
 * @return 0 on success
 * @return -EINVAL in case @p cb is NULL
 */
int bt_nus_inst_cb_register(struct bt_nus_inst *inst, struct bt_nus_cb *cb, void *ctx);

/** @brief Send Data to NUS Instance
 *
 * @note This API sends the data to the specified peer.
 *
 * @param conn Connection object to send data to. NULL if notifying all peers.
 * @param inst Pointer to instance of NUS service. NULL if using default instance.
 * @param data Pointer to buffer with bytes to send.
 * @param len Length in bytes of data to send.
 *
 * @return 0 on success, negative error code if failed.
 * @return -EAGAIN when Bluetooth stack has not been enabled.
 * @return -ENOTCONN when either no connection has been established or no peers
 *         have subscribed.
 */
int bt_nus_inst_send(struct bt_conn *conn,
		     struct bt_nus_inst *inst,
		     const void *data,
		     uint16_t len);

/** @brief NUS server callback register
 *
 * @param cb Pointer to callbacks structure. Must be valid throughout the
 *           lifetime of the application.
 * @param ctx User context to be provided through the callback.
 *
 * @return 0 on success, negative error code if failed.
 * @return -EINVAL in case @p cb is NULL
 */
static inline int bt_nus_cb_register(struct bt_nus_cb *cb, void *ctx)
{
	return bt_nus_inst_cb_register(NULL, cb, ctx);
}

/** @brief Send Data over NUS
 *
 * @note This API sends the data to the specified peer.
 *
 * @param conn Connection object to send data to. NULL if notifying all peers.
 * @param data Pointer to buffer with bytes to send.
 * @param len Length in bytes of data to send.
 *
 * @return 0 on success, negative error code if failed.
 * @return -EAGAIN when Bluetooth stack has not been enabled.
 * @return -ENOTCONN when either no connection has been established or no peers
 *         have subscribed.
 */
static inline int bt_nus_send(struct bt_conn *conn, const void *data, uint16_t len)
{
	return bt_nus_inst_send(conn, NULL, data, len);
}

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_NUS_H_ */
