/** @file
 *  @brief Bluetooth MAP Client handling
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MAP_CLIENT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MAP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/classic/parse_vcard.h>

#define MAP_GET_VALUE_16(addr)        sys_be16_to_cpu(UNALIGNED_GET((uint16_t *)(addr)))
#define MAP_PUT_VALUE_16(value, addr) UNALIGNED_PUT(sys_cpu_to_be16(value), (uint16_t *)addr)
#define MAP_GET_VALUE_32(addr)        sys_be32_to_cpu(UNALIGNED_GET((uint32_t *)(addr)))
#define MAP_PUT_VALUE_32(value, addr) UNALIGNED_PUT(sys_cpu_to_be32(value), (uint32_t *)addr)

enum {
	MAP_PARAM_ID_MAX_LIST_COUNT = 0x01,
	MAP_PARAM_ID_FILTER_MESSAGE_TYPE = 0x03,
	MAP_PARAM_ID_PARAMETER_MASK = 0x10,
	MAP_PARAM_ID_MESSAGES_LISTING_SIZE = 0x12,
	MAP_PARAM_ID_ATTACHMENT = 0x0A,
	MAP_PARAM_ID_NEW_MESSAGE = 0x0D,
	MAP_PARAM_ID_MSE_TIME = 0x19,
};

enum {
	MAP_PARAMETER_MASK_SUBJECT = (0x1 << 0),
	MAP_PARAMETER_MASK_DATETIME = (0x1 << 1),
	MAP_PARAMETER_MASK_SENDER_NAME = (0x1 << 2),
	MAP_PARAMETER_MASK_SENDER_ADDRESS = (0x1 << 3),
	MAP_PARAMETER_MASK_RECIPIENT_NAME = (0x1 << 4),
	MAP_PARAMETER_MASK_RECIPIENT_ADDRESSING = (0x1 << 5),
	MAP_PARAMETER_MASK_TYPE = (0x1 << 6),
	MAP_PARAMETER_MASK_SIZE = (0x1 << 7),
	MAP_PARAMETER_MASK_RECEPTION_STATUS = (0x1 << 8),
	MAP_PARAMETER_MASK_TEXT = (0x1 << 9),
	MAP_PARAMETER_MASK_ATTACHMENT_SIZE = (0x1 << 10),
	MAP_PARAMETER_MASK_PRIORITY = (0x1 << 11),
	MAP_PARAMETER_MASK_READ = (0x1 << 12),
	MAP_PARAMETER_MASK_SEND = (0x1 << 13),
	MAP_PARAMETER_MASK_PROTECTED = (0x1 << 14),
	MAP_PARAMETER_MASK_REPLYTO_ADDRESSING = (0x1 << 15),
};

enum {
	MAP_MESSAGE_RESULT_TYPE_DATETIME = 0x01,
	MAP_MESSAGE_RESULT_TYPE_BODY = 0x02,
};

struct bt_map_client;

struct bt_map_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

struct bt_map_client_cb {
	void (*connected)(struct bt_map_client *client);
	void (*disconnected)(struct bt_map_client *client);
	void (*set_path_finished)(struct bt_map_client *client);
	void (*recv)(struct bt_map_client *client, struct bt_map_result *result,
		     uint8_t array_size);
};

struct map_common_param {
	uint8_t id;
	uint8_t len;
	uint8_t data[0];
};

struct map_get_messages_listing_param {
	uint16_t max_list_count;
	uint8_t filter_message_type;
	uint8_t reserved;
	uint32_t parametr_mask;
};

struct bt_map_client {
	struct bt_conn *conn;
	char *path;
	void *goep_client;
	uint8_t state;
	int8_t folder_level;
	uint16_t max_list_cnt;
	uint16_t get_list_cnt;
	uint32_t mask;
	uint8_t goep_connect: 1;
	uint8_t path_valid: 1;
	struct bt_map_client_cb *cb;
};

/** @brief MAP Client conect
 *
 *  @param conn Connection object.
 *  @param path map message path.
 *  @param cb bt_map_client_cb.
 *
 *  @return pointer to struct bt_map_client in case of success or NULL in case
 *  of error.
 */
struct bt_map_client *bt_map_client_connect(struct bt_conn *conn, char *path,
					    struct bt_map_client_cb *cb);

/** @brief MAP Client disconnect
 *
 *  @param client The map client instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_disconnect(struct bt_map_client *client);

/** @brief MAP Client get map message
 *
 *  @param client The map client instance.
 *  @param path  map message path..
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_get_message(struct bt_map_client *client, char *path);

/** @brief MAP Client set map path
 *
 *  @param client The map client instance.
 *  @param path  map message path..
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_setpath(struct bt_map_client *client, char *path);

/** @brief MAP Client set map folder
 *
 *  @param client The map client instance.
 *  @param path  map message path.
 *  @param flags  map message flags..
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_set_folder(struct bt_map_client *client, char *path, uint8_t flags);

/** @brief MAP Client get map meaasage list
 *
 *  @param client The map client instance.
 *  @param max_list_count  map message max message count.
 *  @param mask  map message mask..
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_get_messages_listing(struct bt_map_client *client, uint16_t max_list_count,
				       uint32_t mask);

/** @brief MAP Client get map folder list
 *
 *  @param client The map client instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_map_client_get_folder_listing(struct bt_map_client *client);

#ifdef __cplusplus
}
#endif
#endif /* __BT_MAP_CLIENT_H */
