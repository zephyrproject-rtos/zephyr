/** @file
 *  @brief Bluetooth PBAP Client handling
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PBAP_CLIENT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PBAP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/classic/parse_vcard.h>

#define PBAP_GET_VALUE_16(addr)        sys_be16_to_cpu(UNALIGNED_GET((uint16_t *)(addr)))
#define PBAP_PUT_VALUE_16(value, addr) UNALIGNED_PUT(sys_cpu_to_be16(value), (uint16_t *)addr)
#define PBAP_GET_VALUE_32(addr)        sys_be32_to_cpu(UNALIGNED_GET((uint32_t *)(addr)))
#define PBAP_PUT_VALUE_32(value, addr) UNALIGNED_PUT(sys_cpu_to_be32(value), (uint32_t *)addr)

struct bt_pbap_client;

enum {
	PBAP_PARAM_ID_ORDER = 0x01,
	PBAP_PARAM_ID_SEARCH_VALUE = 0x02,
	PBAP_PARAM_ID_SEARCH_ATTR = 0x03,
	PBAP_PARAM_ID_MAX_LIST_COUNT = 0x04,
	PBAP_PARAM_ID_FILTER = 0x06,
	PBAP_PARAM_ID_FORMAT = 0x07,
	PBAP_PARAM_ID_PHONEBOOK_SIZE = 0x08,
};

enum {
	PBAP_VCARD_VERSION_2_1 = 0x00, /* Version 2.1 */
};

enum {
	PBAP_FILTER_VCARD_VERSION = (0x1 << 0),
	PBAP_FILTER_FORMAT_NAME = (0x1 << 1),
	PBAP_FILTER_SP_NAME = (0x1 << 2),
	PBAP_FILTER_AI_PHOTO = (0x1 << 3),
	PBAP_FILTER_BIRTHDAY = (0x1 << 4),
	PBAP_FILTER_DELIVERY_ADDRESS = (0x1 << 5),
	PBAP_FILTER_DELIVERY = (0x1 << 6),
	PBAP_FILTER_TELEPHONE_NUMBER = (0x1 << 7),
	PBAP_FILTER_EMAIL_ADDRESS = (0x1 << 8),
	PBAP_FILTER_EMAIL = (0x1 << 9),
	PBAP_FILTER_TIME_ZONE = (0x1 << 10),
	PBAP_FILTER_GEOGRAPHIC_POSITION = (0x1 << 11),
	PBAP_FILTER_JOB = (0x1 << 12),
	PBAP_FILTER_ROLE_WITH_ORG = (0x1 << 13),
	PBAP_FILTER_ORG_LOGO = (0x1 << 14),
	PBAP_FILTER_VCARD_OF_PERSON = (0x1 << 15),
	PBAP_FILTER_NAME_OF_ORG = (0x1 << 16),
	PBAP_FILTER_COMMENTS = (0x1 << 17),
	PBAP_FILTER_REVISION = (0x1 << 18),
	PBAP_FILTER_PRON_OF_NAME = (0x1 << 19),
	PBAP_FILTER_UNIFORM_LOCATOR = (0x1 << 20),
	PBAP_FILTER_UNIQUE_ID = (0x1 << 21),
	PBAP_FILTER_PUBLIC_ENCRY_KEY = (0x1 << 22),
	PBAP_FILTER_NICKNAME = (0x1 << 23),
	PBAP_FILTER_CATEGORIES = (0x1 << 24),
	PBAP_FILTER_PRODUCT_ID = (0x1 << 25),
	PBAP_FILTER_CLASS_INFO = (0x1 << 26),
	PBAP_FILTER_STRING_FOR_OPT = (0x1 << 27),
	PBAP_FILTER_TIMESTAMP = (0x1 << 28),
};

enum {
	PBAP_CLIENT_CB_EVENT_MAX_SIZE,
	PBAP_CLIENT_CB_EVENT_VCARD,
	PBAP_CLIENT_CB_EVENT_SETPATH_FINISH,
	PBAP_CLIENT_CB_EVENT_SEARCH_RESULT,
	PBAP_CLIENT_CB_EVENT_GET_VCARD_FINISH,
	PBAP_CLIENT_CB_EVENT_END_OF_BODY,
	PBAP_CLIENT_CB_EVENT_ABORT_FINISH,
};

enum {
	PBAP_CLIENT_OP_CMD_GET_SIZE,
	PBAP_CLIENT_OP_CMD_GET_PB,
	PBAP_CLIENT_OP_CMD_SET_PATH,
	PBAP_CLIENT_OP_CMD_GET_VCARD,
	PBAP_CLIENT_OP_CMD_GET_CONTINUE,
	PBAP_CLIENT_OP_CMD_LISTING,
	PBAP_CLIENT_OP_CMD_ABORT,
};

struct pbap_common_param {
	uint8_t id;
	uint8_t len;
	uint8_t data[0];
};

struct pbap_client_param {
	uint8_t op_cmd;
	uint8_t user_id;
	uint8_t order: 4;
	uint8_t search_attr: 4;
	uint32_t filter;
	union {
		uint8_t search_len;
		uint8_t vcard_name_len;
	};
	union {
		uint8_t *search_value;
		uint8_t *vcard_name;
		char *path;
	};
};

struct bt_pbap_result {
	uint8_t event;
	union {
		uint16_t max_size;
		struct {
			uint8_t array_size;
			struct parse_vcard_result *vcard_result;
		};
	};
};

struct bt_pbap_client_cb {
	void (*connected)(struct bt_pbap_client *client);
	void (*disconnected)(struct bt_pbap_client *client);
	void (*recv)(struct bt_pbap_client *client, struct bt_pbap_result *result);
};

struct bt_pbap_client {
	struct bt_conn *conn;
	char *path;
	void *goep_client;
	uint16_t max_list_cnt;
	uint8_t user_id;
	uint8_t state;
	uint8_t folder_level: 4;
	uint8_t path_valid: 1;
	uint8_t op_busy: 1;
	uint32_t op_time;
	struct bt_pbap_client_cb *cb;
	struct parse_cached_data vcached;
};

/** @brief PBAP Client Connect
 *
 *  @param conn Connection object.
 *  @param cb bt_map_client_cb.
 *
 *  @return pointer to struct bt_pbap_client in case of success or NULL in case
 *  of error.
 */
struct bt_pbap_client *bt_pbap_client_connect(struct bt_conn *conn, struct bt_pbap_client_cb *cb);

/** @brief PBAP Client disconnect
 *
 *  @param client The pbap client instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_pbap_client_disconnect(struct bt_pbap_client *client);

/** @brief PBAP Client Request Operation
 *
 *  @param client The pbap client instance.
 *  @param param The pbap client param.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_pbap_client_request(struct bt_pbap_client *client, struct pbap_client_param *param);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PBAP_CLIENT_H_ */
