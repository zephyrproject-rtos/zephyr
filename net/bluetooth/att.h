/* att.h - Attribute protocol handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define BT_ATT_DEFAULT_LE_MTU	23

struct bt_att_hdr {
	uint8_t  code;
} __packed;

/* Error codes for Error response PDU */
#define BT_ATT_ERR_INVALID_HANDLE		0x01
#define BT_ATT_ERR_READ_NOT_PERMITTED		0x02
#define BT_ATT_ERR_WRITE_NOT_PERMITTED		0x03
#define BT_ATT_ERR_INVALID_PDU			0x04
#define BT_ATT_ERR_AUTHENTICATION		0x05
#define BT_ATT_ERR_NOT_SUPPORTED		0x06
#define BT_ATT_ERR_INVALID_OFFSET		0x07
#define BT_ATT_ERR_AUTHORIZATION		0x08
#define BT_ATT_ERR_PREPARE_QUEUE_FULL		0x09
#define BT_ATT_ERR_ATTRIBUTE_NOT_FOUND		0x0a
#define BT_ATT_ERR_ATTRIBUTE_NOT_LONG		0x0b
#define BT_ATT_ERR_ENCRYPTION_KEY_SIZE		0x0c
#define BT_ATT_ERR_INVALID_ATTRIBUTE_LEN	0x0d
#define BT_ATT_ERR_UNLIKELY			0x0e
#define BT_ATT_ERR_INSUFFICIENT_ENCRYPTION	0x0f
#define BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE	0x10
#define BT_ATT_ERR_INSUFFICIENT_RESOURCES	0x11

#define BT_ATT_OP_ERROR_RSP			0x01
struct bt_att_error_rsp {
	uint8_t  request;
	uint16_t handle;
	uint8_t  error;
} __packed;

#define BT_ATT_OP_MTU_REQ			0x02
struct bt_att_exchange_mtu_req {
	uint16_t mtu;
} __packed;

#define BT_ATT_OP_MTU_RSP			0x03
struct bt_att_exchange_mtu_rsp {
	uint16_t mtu;
} __packed;

/* Find Information Request */
#define BT_ATT_OP_FIND_INFO_REQ			0x04
struct bt_att_find_info_req {
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;

/* Format field values for BT_ATT_OP_FIND_INFO_RSP */
#define BT_ATT_INFO_16				0x01
#define BT_ATT_INFO_128				0x02

struct bt_att_info_16 {
	uint16_t handle;
	uint16_t uuid;
} __packed;

struct bt_att_info_128 {
	uint16_t handle;
	uint8_t  uuid[16];
} __packed;

/* Find Information Response */
#define BT_ATT_OP_FIND_INFO_RSP			0x05
struct bt_att_find_info_rsp {
	uint8_t  format;
	uint8_t  info[0];
} __packed;

/* Find By Type Value Request */
#define BT_ATT_OP_FIND_TYPE_REQ			0x06
struct bt_att_find_type_req {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t type;
	uint8_t  value[0];
} __packed;

struct bt_att_handle_group {
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;

/* Find By Type Value Response */
#define BT_ATT_OP_FIND_TYPE_RSP			0x07
struct bt_att_find_type_rsp {
	struct bt_att_handle_group list[0];
} __packed;

/* Read By Type Request */
#define BT_ATT_OP_READ_TYPE_REQ			0x08
struct bt_att_read_type_req {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t  uuid[0];
} __packed;

struct bt_att_data {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

/* Read By Type Response */
#define BT_ATT_OP_READ_TYPE_RSP			0x09
struct bt_att_read_type_rsp {
	uint8_t  len;
	struct bt_att_data data[0];
} __packed;

/* Read Request */
#define BT_ATT_OP_READ_REQ			0x0a
struct bt_att_read_req {
	uint16_t handle;
} __packed;

/* Read Response */
#define BT_ATT_OP_READ_RSP			0x0b
struct bt_att_read_rsp {
	uint8_t  value[0];
} __packed;

/* Read Blob Request */
#define BT_ATT_OP_READ_BLOB_REQ			0x0c
struct bt_att_read_blob_req {
	uint16_t handle;
	uint16_t offset;
} __packed;

/* Read Blob Response */
#define BT_ATT_OP_READ_BLOB_RSP			0x0d
struct bt_att_read_blob_rsp {
	uint8_t  value[0];
} __packed;

/* Read Multiple Request */
#define BT_ATT_READ_MULT_MIN_LEN_REQ		0x04

#define BT_ATT_OP_READ_MULT_REQ			0x0e
struct bt_att_read_mult_req {
	uint16_t handles[0];
} __packed;

/* Read Multiple Respose */
#define BT_ATT_OP_READ_MULT_RSP			0x0f
struct bt_att_read_mult_rsp {
	uint8_t  value[0];
} __packed;

/* Read by Group Type Request */
#define BT_ATT_OP_READ_GROUP_REQ		0x10
struct bt_att_read_group_req {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t  uuid[0];
} __packed;

struct bt_att_group_data {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t  value[0];
} __packed;

/* Read by Group Type Response */
#define BT_ATT_OP_READ_GROUP_RSP		0x11
struct bt_att_read_group_rsp {
	uint8_t  len;
	struct bt_att_group_data data[0];
} __packed;

/* Write Request */
#define BT_ATT_OP_WRITE_REQ			0x12
struct bt_att_write_req {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

/* Write Response */
#define BT_ATT_OP_WRITE_RSP			0x13

/* Prepare Write Request */
#define BT_ATT_OP_PREPARE_WRITE_REQ		0x16
struct bt_att_prepare_write_req {
	uint16_t handle;
	uint16_t offset;
	uint8_t  value[0];
} __packed;

/* Prepare Write Respond */
#define BT_ATT_OP_PREPARE_WRITE_RSP		0x17
struct bt_att_prepare_write_rsp {
	uint16_t handle;
	uint16_t offset;
	uint8_t  value[0];
} __packed;

/* Execute Write Request */
#define BT_ATT_FLAG_CANCEL			0x00
#define BT_ATT_FLAG_EXEC			0x01

#define BT_ATT_OP_EXEC_WRITE_REQ		0x18
struct bt_att_exec_write_req {
	uint8_t  flags;
} __packed;

/* Execute Write Response */
#define BT_ATT_OP_EXEC_WRITE_RSP		0x19

/* Handle Value Notification */
#define BT_ATT_OP_NOTIFY			0x1b
struct bt_att_notify {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

/* Handle Value Indication */
#define BT_ATT_OP_INDICATE			0x1d
struct bt_att_indicate {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

/* Handle Value Confirm */
#define BT_ATT_OP_CONFIRM			0x1e

struct bt_att_signature {
	uint8_t  value[12];
} __packed;

/* Write Command */
#define BT_ATT_OP_WRITE_CMD			0x52
struct bt_att_write_cmd {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

/* Signed Write Command */
#define BT_ATT_OP_SIGNED_WRITE_CMD		0xd2
struct bt_att_signed_write_cmd {
	uint16_t handle;
	uint8_t  value[0];
} __packed;

void bt_att_init(void);
uint16_t bt_att_get_mtu(struct bt_conn *conn);
struct bt_buf *bt_att_create_pdu(struct bt_conn *conn, uint8_t op, size_t len);

typedef void (*bt_att_func_t)(struct bt_conn *conn, uint8_t err,
			      const void *pdu, uint16_t length,
			      void *user_data);
typedef void (*bt_att_destroy_t)(void *user_data);

/* Send ATT PDU over a connection */
int bt_att_send(struct bt_conn *conn, struct bt_buf *buf, bt_att_func_t func,
		void *user_data, bt_att_destroy_t destroy);

/* Cancel ATT request */
void bt_att_cancel(struct bt_conn *conn);
