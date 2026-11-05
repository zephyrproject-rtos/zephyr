/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "canopen_internal.h"

#define CANOPEN_OD_SDO_CLIENT 0x1280U

#define CLIENT_SUB_COB_TX    1U
#define CLIENT_SUB_COB_RX    2U
#define CLIENT_SUB_SERVER_ID 3U
#define CLIENT_SERVER_ID_MAX 127U

#define SDO_CCS_DOWNLOAD_SEGMENT  0x00U
#define SDO_CCS_DOWNLOAD_INITIATE 0x20U
#define SDO_CCS_UPLOAD_INITIATE   0x40U
#define SDO_CCS_UPLOAD_SEGMENT    0x60U
#define SDO_SCS_UPLOAD_SEGMENT    0U
#define SDO_SCS_DOWNLOAD_SEGMENT  1U
#define SDO_SCS_UPLOAD_INITIATE   2U
#define SDO_SCS_DOWNLOAD_INITIATE 3U
#define SDO_ABORT                 0x80U

#define SDO_SIZE_INDICATED BIT(0)
#define SDO_EXPEDITED      BIT(1)
#define SDO_LAST           BIT(0)
#define SDO_TOGGLE         BIT(4)

#define SDO_FRAME_LEN      8U
#define SDO_EXPEDITED_DATA 4U
#define SDO_SEGMENT_DATA   7U

enum client_state {
	CLIENT_IDLE,
	CLIENT_DOWNLOAD_INITIATE,
	CLIENT_DOWNLOAD_SEGMENT,
	CLIENT_UPLOAD_INITIATE,
	CLIENT_UPLOAD_SEGMENT,
};

static const uint8_t client_highest = CLIENT_SUB_SERVER_ID;

static bool client_valid(const struct canopen_sdo_client *client)
{
	return ((client->cob_tx | client->cob_rx) & CANOPEN_COB_ID_INVALID) == 0U;
}

static void client_send(struct canopen_node *node, const uint8_t *data)
{
	struct can_frame frame = {
		.id = node->client.cob_tx & CAN_STD_ID_MASK,
		.dlc = SDO_FRAME_LEN,
	};

	memcpy(frame.data, data, SDO_FRAME_LEN);
	(void)canopen_send(node, &frame);
}

static void client_init_frame(const struct canopen_sdo_client *client, uint8_t *data, uint8_t cs)
{
	memset(data, 0, SDO_FRAME_LEN);
	data[0] = cs;
	sys_put_le16(client->index, &data[1]);
	data[3] = client->sub;
}

static void client_finish(struct canopen_node *node, int result)
{
	node->client.state = CLIENT_IDLE;
	node->client.result = result;
	k_sem_give(&node->client.done);
}

static void client_abort(struct canopen_node *node, uint32_t code, int result)
{
	uint8_t data[SDO_FRAME_LEN];

	client_init_frame(&node->client, data, SDO_ABORT);
	sys_put_le32(code, &data[4]);
	client_send(node, data);
	client_finish(node, result);
}

static void client_arm(struct canopen_sdo_client *client, enum client_state state)
{
	client->state = state;
	client->deadline = sys_timepoint_calc(K_MSEC(CONFIG_CANOPEN_SDO_CLIENT_TIMEOUT_MS));
}

static void client_download_segment(struct canopen_node *node)
{
	struct canopen_sdo_client *client = &node->client;
	size_t len = MIN(client->size - client->pos, SDO_SEGMENT_DATA);
	uint8_t data[SDO_FRAME_LEN] = {0};

	data[0] = SDO_CCS_DOWNLOAD_SEGMENT | client->toggle | ((SDO_SEGMENT_DATA - len) << 1);
	if ((client->pos + len) == client->size) {
		data[0] |= SDO_LAST;
	}

	memcpy(&data[1], &client->src[client->pos], len);
	client->pos += len;
	client_send(node, data);
	client_arm(client, CLIENT_DOWNLOAD_SEGMENT);
}

static void client_upload_request(struct canopen_node *node)
{
	struct canopen_sdo_client *client = &node->client;
	uint8_t data[SDO_FRAME_LEN] = {SDO_CCS_UPLOAD_SEGMENT | client->toggle};

	client_send(node, data);
	client_arm(client, CLIENT_UPLOAD_SEGMENT);
}

static bool client_mux_match(const struct canopen_sdo_client *client, const uint8_t *resp)
{
	return (sys_get_le16(&resp[1]) == client->index) && (resp[3] == client->sub);
}

static void client_upload_initiate(struct canopen_node *node, const uint8_t *resp)
{
	struct canopen_sdo_client *client = &node->client;
	size_t len;

	if ((resp[0] & SDO_EXPEDITED) != 0U) {
		len = ((resp[0] & SDO_SIZE_INDICATED) != 0U)
			      ? (SDO_EXPEDITED_DATA - ((resp[0] >> 2) & 0x3U))
			      : SDO_EXPEDITED_DATA;
		if (len > client->max) {
			client_abort(node, CANOPEN_SDO_ABORT_OUT_OF_MEMORY, -ENOSPC);
			return;
		}

		memcpy(client->dst, &resp[4], len);
		client->pos = len;
		client_finish(node, 0);
		return;
	}

	client->size = ((resp[0] & SDO_SIZE_INDICATED) != 0U) ? sys_get_le32(&resp[4]) : SIZE_MAX;
	if ((client->size != SIZE_MAX) && (client->size > client->max)) {
		client_abort(node, CANOPEN_SDO_ABORT_OUT_OF_MEMORY, -ENOSPC);
		return;
	}

	client_upload_request(node);
}

static void client_upload_segment(struct canopen_node *node, const uint8_t *resp)
{
	struct canopen_sdo_client *client = &node->client;
	size_t len = SDO_SEGMENT_DATA - ((resp[0] >> 1) & 0x7U);

	if ((client->pos + len) > client->max) {
		client_abort(node, CANOPEN_SDO_ABORT_OUT_OF_MEMORY, -ENOSPC);
		return;
	}

	memcpy(&client->dst[client->pos], &resp[1], len);
	client->pos += len;

	if ((resp[0] & SDO_LAST) == 0U) {
		client->toggle ^= SDO_TOGGLE;
		client_upload_request(node);
	} else if ((client->size != SIZE_MAX) && (client->pos != client->size)) {
		client_abort(node, CANOPEN_SDO_ABORT_LENGTH_MISMATCH, -EPROTO);
	} else {
		client_finish(node, 0);
	}
}

bool canopen_sdo_client_rx(struct canopen_node *node, const struct can_frame *frame)
{
	struct canopen_sdo_client *client = &node->client;
	const uint8_t *resp = frame->data;
	uint8_t scs = resp[0] >> 5;

	if ((client->state == CLIENT_IDLE) || !client_valid(client) ||
	    (frame->id != (client->cob_rx & CAN_STD_ID_MASK))) {
		return false;
	}

	if (frame->dlc != SDO_FRAME_LEN) {
		return true;
	}

	if (resp[0] == SDO_ABORT) {
		client->abort = sys_get_le32(&resp[4]);
		client_finish(node, -ECANCELED);
		return true;
	}

	switch (client->state) {
	case CLIENT_DOWNLOAD_INITIATE:
		if ((scs != SDO_SCS_DOWNLOAD_INITIATE) || !client_mux_match(client, resp)) {
			break;
		}
		if (client->pos == client->size) {
			client_finish(node, 0);
		} else {
			client_download_segment(node);
		}
		return true;
	case CLIENT_DOWNLOAD_SEGMENT:
		if ((scs != SDO_SCS_DOWNLOAD_SEGMENT) ||
		    ((resp[0] & SDO_TOGGLE) != client->toggle)) {
			break;
		}
		if (client->pos == client->size) {
			client_finish(node, 0);
		} else {
			client->toggle ^= SDO_TOGGLE;
			client_download_segment(node);
		}
		return true;
	case CLIENT_UPLOAD_INITIATE:
		if ((scs != SDO_SCS_UPLOAD_INITIATE) || !client_mux_match(client, resp)) {
			break;
		}
		client_upload_initiate(node, resp);
		return true;
	default:
		if ((scs != SDO_SCS_UPLOAD_SEGMENT) || ((resp[0] & SDO_TOGGLE) != client->toggle)) {
			break;
		}
		client_upload_segment(node, resp);
		return true;
	}

	client_abort(node, CANOPEN_SDO_ABORT_COMMAND, -EPROTO);

	return true;
}

void canopen_sdo_client_process(struct canopen_node *node)
{
	if ((node->client.state != CLIENT_IDLE) && sys_timepoint_expired(node->client.deadline)) {
		client_abort(node, CANOPEN_SDO_ABORT_TIMEOUT, -ETIMEDOUT);
	}
}

k_timepoint_t canopen_sdo_client_deadline(struct canopen_node *node)
{
	return (node->client.state == CLIENT_IDLE) ? sys_timepoint_calc(K_FOREVER)
						   : node->client.deadline;
}

static void client_filter(struct canopen_node *node)
{
	struct canopen_sdo_client *client = &node->client;
	uint32_t cob_id = client_valid(client) ? client->cob_rx : CANOPEN_COB_ID_INVALID;

	if (canopen_filter_set(node, &client->filter, cob_id) != 0) {
		client->cob_rx |= CANOPEN_COB_ID_INVALID;
	}
}

void canopen_sdo_client_init(struct canopen_node *node)
{
	node->client.filter = -1;
	node->client.state = CLIENT_IDLE;
	k_sem_init(&node->client.done, 0, 1);
}

void canopen_sdo_client_reset(struct canopen_node *node)
{
	struct canopen_sdo_client *client = &node->client;

	if (client->state != CLIENT_IDLE) {
		client_finish(node, -ECONNRESET);
	}

	client->cob_tx = CANOPEN_COB_ID_INVALID;
	client->cob_rx = CANOPEN_COB_ID_INVALID;
	client->server_id = 1U;
	client_filter(node);
}

uint32_t canopen_sdo_client_find(struct canopen_node *node, uint8_t sub,
				 struct canopen_od_ref *ref)
{
	struct canopen_sdo_client *client = &node->client;
	const uint16_t index = CANOPEN_OD_SDO_CLIENT;

	switch (sub) {
	case 0U:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &client_highest, NULL, sizeof(client_highest));
		break;
	case CLIENT_SUB_COB_TX:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &client->cob_tx, &client->cob_tx, sizeof(client->cob_tx));
		break;
	case CLIENT_SUB_COB_RX:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &client->cob_rx, &client->cob_rx, sizeof(client->cob_rx));
		break;
	case CLIENT_SUB_SERVER_ID:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW,
				    &client->server_id, &client->server_id,
				    sizeof(client->server_id));
		break;
	default:
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_sdo_client_write(struct canopen_node *node, uint8_t sub, uint32_t value)
{
	struct canopen_sdo_client *client = &node->client;
	uint32_t *cob = (sub == CLIENT_SUB_COB_TX) ? &client->cob_tx : &client->cob_rx;

	if (client->state != CLIENT_IDLE) {
		return CANOPEN_SDO_ABORT_DEVICE_STATE;
	}

	if (sub == CLIENT_SUB_SERVER_ID) {
		if ((value == 0U) || (value > CLIENT_SERVER_ID_MAX)) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		client->server_id = (uint8_t)value;
		return CANOPEN_SDO_ABORT_NONE;
	}

	if (((value & (CANOPEN_COB_ID_FRAME | CANOPEN_COB_ID_RESERVED)) != 0U) ||
	    (client_valid(client) && ((value & CANOPEN_COB_ID_INVALID) == 0U) &&
	     ((value & CAN_STD_ID_MASK) != (*cob & CAN_STD_ID_MASK)))) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	*cob = value;
	client_filter(node);

	return CANOPEN_SDO_ABORT_NONE;
}

static int client_start(struct canopen_node *node, uint8_t server_id, uint16_t index,
			uint8_t sub, bool upload)
{
	struct canopen_sdo_client *client = &node->client;

	if (node->lock.lock_count > 1U) {
		return -EDEADLK;
	}

	if (server_id > CLIENT_SERVER_ID_MAX) {
		return -EINVAL;
	}

	if (client->state != CLIENT_IDLE) {
		return -EBUSY;
	}

	if (server_id != 0U) {
		client->cob_tx = CANOPEN_COB_SDO_RX + server_id;
		client->cob_rx = CANOPEN_COB_SDO_TX + server_id;
		client->server_id = server_id;
		client_filter(node);
	}

	if (!client_valid(client)) {
		return -ENOTCONN;
	}

	client->index = index;
	client->sub = sub;
	client->pos = 0U;
	client->toggle = 0U;
	client->abort = CANOPEN_SDO_ABORT_NONE;
	k_sem_reset(&client->done);
	client_arm(client, upload ? CLIENT_UPLOAD_INITIATE : CLIENT_DOWNLOAD_INITIATE);

	return 0;
}

static int client_wait(struct canopen_node *node, uint32_t *abort)
{
	struct canopen_sdo_client *client = &node->client;

	k_poll_signal_raise(&node->signal, 0);
	(void)k_sem_take(&client->done, K_FOREVER);

	if (abort != NULL) {
		*abort = client->abort;
	}

	return client->result;
}

int canopen_sdo_upload(struct canopen_node *node, uint8_t server_id, uint16_t index, uint8_t sub,
		       void *buf, size_t *len, uint32_t *abort)
{
	struct canopen_sdo_client *client = &node->client;
	uint8_t data[SDO_FRAME_LEN];
	int err;

	if ((buf == NULL) || (len == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	err = client_start(node, server_id, index, sub, true);
	if (err == 0) {
		client->dst = buf;
		client->max = *len;
		client->size = 0U;
		client_init_frame(client, data, SDO_CCS_UPLOAD_INITIATE);
		client_send(node, data);
	}
	k_mutex_unlock(&node->lock);

	if (err != 0) {
		return err;
	}

	err = client_wait(node, abort);
	*len = client->pos;

	return err;
}

int canopen_sdo_download(struct canopen_node *node, uint8_t server_id, uint16_t index,
			 uint8_t sub, const void *buf, size_t len, uint32_t *abort)
{
	struct canopen_sdo_client *client = &node->client;
	uint8_t data[SDO_FRAME_LEN];
	int err;

	if ((buf == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	err = client_start(node, server_id, index, sub, false);
	if (err == 0) {
		client->src = buf;
		client->size = len;
		if (len <= SDO_EXPEDITED_DATA) {
			client_init_frame(client, data,
					  SDO_CCS_DOWNLOAD_INITIATE |
						  ((SDO_EXPEDITED_DATA - len) << 2) |
						  SDO_EXPEDITED | SDO_SIZE_INDICATED);
			memcpy(&data[4], buf, len);
			client->pos = len;
		} else {
			client_init_frame(client, data,
					  SDO_CCS_DOWNLOAD_INITIATE | SDO_SIZE_INDICATED);
			sys_put_le32(len, &data[4]);
		}
		client_send(node, data);
	}
	k_mutex_unlock(&node->lock);

	if (err != 0) {
		return err;
	}

	return client_wait(node, abort);
}
