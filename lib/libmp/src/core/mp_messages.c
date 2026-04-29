/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>

#include "mp_messages.h"

static uint32_t seq_id = 1;

struct mp_message *mp_message_new(enum mp_message_type type, struct mp_object *src,
				  struct mp_structure *data)
{
	struct mp_message *msg = k_malloc(sizeof(struct mp_message));

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

void mp_message_destroy(struct mp_message *msg)
{
	if (msg == NULL) {
		return;
	}

	if (msg->data != NULL) {
		mp_structure_destroy(msg->data);
	}

	k_free(msg);
}
