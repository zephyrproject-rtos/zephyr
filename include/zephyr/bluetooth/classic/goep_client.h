/** @file
 *  @brief GOEP Client Protocol handling.
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GOEP_CLIENT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GOEP_CLIENT_H_

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	GOEP_SEND_HEADER_CONNECTING_ID = (0x1 << 0),
	GOEP_SEND_HEADER_NAME = (0x1 << 1),
	GOEP_SEND_HEADER_TYPE = (0x1 << 2),
	GOEP_SEND_HEADER_APP_PARAM = (0x1 << 3),
	GOEP_SEND_HEADER_FLAGS = (0x1 << 4),
};

enum {
	GOEP_RECV_HEADER_APP_PARAM = (0x1 << 0),
	GOEP_RECV_HEADER_BODY = (0x1 << 1),
	GOEP_RECV_HEADER_END_OF_BODY = (0x1 << 2),
	GOEP_RECV_CONTINUE_WITHOUT_BODY = (0x1 << 3),
};

struct bt_goep_client;

struct bt_goep_header_param {
	uint16_t flag;
	uint16_t len;
	void *value;
};

struct bt_goep_client_cb {
	void (*connected)(struct bt_goep_client *client);
	void (*aborted)(struct bt_goep_client *client);
	void (*disconnected)(struct bt_goep_client *client);
	void (*recv)(struct bt_goep_client *client, uint16_t flag, uint8_t *data, uint16_t len);
	void (*setpath)(struct bt_goep_client *client);
};

struct bt_goep_client {
	struct bt_conn *conn;
	struct bt_rfcomm_server rfcomm_server;
	struct bt_rfcomm_dlc rfcomm_dlc;

	uint8_t *target_uuid;
	uint8_t uuid_len;
	uint8_t state;
	uint8_t server_channel;
	uint32_t connection_id;
	struct bt_goep_client_cb *cb;
};

/** @brief GOEP Client Connect
 *
 *  @param conn Connection object.
 *  @param path map message path.
 *  @param cb bt_map_client_cb.
 *
 *  @return pointer to struct bt_map_client in case of success or NULL in case
 *  of error.
 */
struct bt_goep_client *bt_goep_client_connect(struct bt_conn *conn, uint8_t *target_uuid,
					      uint8_t uuid_len, uint8_t server_channel,
					      struct bt_goep_client_cb *cb);

/** @brief MAP Client disconnect
 *
 *  @param client The map client instance.
 *  @param flag The map client header.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_goep_client_disconnect(struct bt_goep_client *client, uint16_t flag);

/** @brief MAP Client get object
 *
 *  @param client The map client instance.
 *  @param flag The map client header.
 *  @param param The map client param.
 *  @param size The map param size.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_goep_client_get(struct bt_goep_client *client, uint16_t flag,
		       struct bt_goep_header_param *param, uint8_t size);

/** @brief MAP Client abort
 *
 *  @param client The map client instance.
 *  @param flag The map client header.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_goep_client_abort(struct bt_goep_client *client, uint16_t flag);

/** @brief MAP Client set path
 *
 *  @param client The map client instance.
 *  @param flag The map client header.
 *  @param param The map client param.
 *  @param size The map param size.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_goep_client_setpath(struct bt_goep_client *client, uint16_t flag,
			   struct bt_goep_header_param *param, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GOEP_CLIENT_H_ */
