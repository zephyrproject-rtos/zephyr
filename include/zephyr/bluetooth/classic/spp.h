/** @file
 *  @brief Bluetooth SPP Protocol handling.
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_

/**
 * @brief SPP
 * @defgroup SPP
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/bluetooth/classic/rfcomm.h>

struct bt_spp {
	uint8_t connected: 1;
	uint8_t id;
	uint8_t port;
	struct bt_uuid *uuid;
	struct bt_conn *conn;
	struct bt_rfcomm_server rfcomm_server;
	struct bt_rfcomm_dlc rfcomm_dlc;
};

/** @brief spp app call back structure. */
struct bt_spp_cb {
	/** @brief SPP Connected.
	 *
	 *  SPP is connected
	 *
	 *  @param spp The SPP instance.
	 *  @param port The SPP port id.
	 *
	 */
	void (*connected)(struct bt_spp *spp, uint8_t port);

	/** @brief SPP Disconnected.
	 *
	 *  SPP is disconnected
	 *
	 *  @param spp The SPP instance.
	 *  @param port The SPP port id.
	 *
	 */
	void (*disconnected)(struct bt_spp *spp, uint8_t port);

	/** @brief SPP Recv Data.
	 *
	 *  SPP recv data from remote
	 *
	 *  @param spp The SPP instance.
	 *  @param port The SPP port id.
	 *  @param data The data buffer.
	 *  @param len The data buffer len.
	 *
	 */
	void (*recv)(struct bt_spp *spp, uint8_t port, uint8_t *data, uint16_t len);
};

/** @brief Register SPP Callback.
 *
 *  Register SPP Callback
 *
 *  @param cb The bt_spp_cb attribute.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_spp_register_cb(struct bt_spp_cb *cb);

/** @brief Register SPP Server.
 *
 *  Register SPP Server
 *
 *  @param port The spp server port
 *  @param uuid The spp server uuid.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_spp_register_srv(uint8_t port, const struct bt_uuid *uuid);

/** @brief SPP Connect
 *
 * This function establish spp connection.
 *
 *  @param conn The conn instance.
 *  @param uuid SPP uuid.
 *
 *  @return SPP instance and NULL in case of error.
 */
struct bt_spp *bt_spp_connect(struct bt_conn *conn, const struct bt_uuid *uuid);

/** @brief SPP Send
 *
 * This function send spp buffer.
 *
 *  @param spp The SPP instance.
 *  @param data Send buffer.
 *  @param len  Buffer size.
 *
 *  @return size in case of success and error code in case of error.
 */
int bt_spp_send(struct bt_spp *spp, uint8_t *data, uint16_t len);

/** @brief SPP Disconnect
 *
 * This function disconnect spp connection.
 *
 *  @param spp The SPP instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_spp_disconnect(struct bt_spp *spp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_ */
