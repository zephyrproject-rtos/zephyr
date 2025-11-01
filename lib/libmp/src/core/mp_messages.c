/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>

#include "mp_messages.h"

static uint32_t seq_id = 1;

MpMessage *mp_message_new(MpMessageType type, MpObject *src, MpStructure *data)
{
	MpMessage *msg = k_malloc(sizeof(MpMessage));
	if (msg == NULL) {
		return NULL;
	}

	msg->type = type;
	msg->src = src;
	msg->timestamp = k_uptime_get_32();
	msg->seq_id = seq_id++;
	msg->data = data;

	return msg;
}

void mp_message_destroy(MpMessage *msg)
{
	if (msg == NULL) {
		return;
	}

	if (msg->data != NULL) {
		mp_structure_destroy(msg->data);
	}

	k_free(msg);
}
