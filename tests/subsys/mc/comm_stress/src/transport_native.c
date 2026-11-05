/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "transport.h"

#define LINK_CAPACITY 2048U

struct test_link {
	uint8_t data[LINK_CAPACITY];
	size_t length;
	bool full;
};

struct test_endpoint {
	struct test_link *incoming;
	struct test_link *outgoing;
	struct mc_comm *peer;
};

static struct test_link sender_to_receiver;
static struct test_link receiver_to_sender;
static struct test_endpoint sender_endpoint = {
	.incoming = &receiver_to_sender,
	.outgoing = &sender_to_receiver,
};
static struct test_endpoint receiver_endpoint = {
	.incoming = &sender_to_receiver,
	.outgoing = &receiver_to_sender,
};
static uint32_t random_state;
static uint32_t frame_count;
static uint8_t current_loss_rate;
static bool inject_duplicate;
static bool inject_corruption;
static bool inject_short_frame;
static bool inject_disconnection;

static uint32_t next_random(void)
{
	random_state = random_state * 1664525U + 1013904223U;
	return random_state;
}

int stress_transport_init(void)
{
	return 0;
}

void stress_transport_reset(uint8_t loss_rate, bool duplicate, bool corrupt,
			    bool shorten, bool disconnect)
{
	memset(&sender_to_receiver, 0, sizeof(sender_to_receiver));
	memset(&receiver_to_sender, 0, sizeof(receiver_to_sender));
	sender_endpoint.peer = NULL;
	receiver_endpoint.peer = NULL;
	random_state = 1U;
	frame_count = 0U;
	current_loss_rate = loss_rate;
	inject_duplicate = duplicate;
	inject_corruption = corrupt;
	inject_short_frame = shorten;
	inject_disconnection = disconnect;
}

void *stress_transport_context(bool sender)
{
	return sender ? (void *)&sender_endpoint : (void *)&receiver_endpoint;
}

void stress_transport_set_peer(bool sender, struct mc_comm *peer)
{
	if (sender) {
		sender_endpoint.peer = peer;
	} else {
		receiver_endpoint.peer = peer;
	}
}

int stress_transport_receive(void *context, uint8_t *data, size_t capacity)
{
	struct test_endpoint *endpoint = context;
	struct test_link *link = endpoint->incoming;

	if (!link->full) {
		return -EAGAIN;
	}
	if (capacity < link->length) {
		return -ENOSPC;
	}
	memcpy(data, link->data, link->length);
	link->full = false;
	return (int)link->length;
}

static void deliver(struct test_endpoint *endpoint, const uint8_t *data, size_t length)
{
	struct test_link *link = endpoint->outgoing;

	memcpy(link->data, data, length);
	link->length = length;
	link->full = true;
	if (endpoint->peer != NULL) {
		(void)mc_comm_update(endpoint->peer);
	}
}

int stress_transport_transmit(void *context, const uint8_t *data, size_t length)
{
	struct test_endpoint *endpoint = context;
	struct test_link *link = endpoint->outgoing;

	if (length > sizeof(link->data)) {
		return -EMSGSIZE;
	}
	if (link->full) {
		return -EAGAIN;
	}
	frame_count++;
	if ((inject_disconnection && frame_count % 32U < 8U) ||
	    (current_loss_rate > 0U && next_random() % 100U < current_loss_rate)) {
		return (int)length;
	}
	memcpy(link->data, data, length);
	link->length = length;
	if (inject_corruption && frame_count % 11U == 0U) {
		link->data[length - 1U] ^= 1U;
	}
	if (inject_short_frame && frame_count % 13U == 0U && length > 1U) {
		link->length--;
	}
	link->full = true;
	if (endpoint->peer != NULL) {
		(void)mc_comm_update(endpoint->peer);
	}
	if (inject_duplicate && frame_count % 7U == 0U && !link->full) {
		deliver(endpoint, data, length);
	}
	return (int)length;
}
