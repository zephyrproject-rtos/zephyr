/*! @file
 @brief Network context API

 An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_ip.h>
#include <net/net_socket.h>

#include "ip/simple-udp.h"

#include "contiki/os/lib/random.h"

struct net_context {
	/* Connection tuple identifies the connection */
	struct net_tuple tuple;

	/* Application receives data via this fifo */
	struct nano_fifo rx_queue;

	/* Application connection data */
	union {
		struct simple_udp_connection udp;
	};

	bool receiver_registered;
};

/* Override this in makefile if needed */
#ifndef NET_MAX_CONTEXT
#define NET_MAX_CONTEXT 5
#endif

static struct net_context contexts[NET_MAX_CONTEXT];
static struct nano_sem contexts_lock;

static void context_sem_give(struct nano_sem *chan)
{
	switch (context_type_get()) {
	case NANO_CTX_FIBER:
		nano_fiber_sem_give(chan);
		break;
	case NANO_CTX_TASK:
		nano_task_sem_give(chan);
		break;
	case NANO_CTX_ISR:
	default:
		/* Invalid context type */
		break;
	}
}

static int context_port_used(enum ip_protocol ip_proto, uint16_t local_port)
{
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (contexts[i].tuple.ip_proto == ip_proto &&
			contexts[i].tuple.local_port == local_port)
			return -EEXIST;
	}

	return 0;
}

struct net_context *net_context_get(enum ip_protocol ip_proto,
					const struct net_addr *remote_addr,
					uint16_t remote_port,
					const struct net_addr *local_addr,
					uint16_t local_port)
{
	int i;
	struct net_context *context = NULL;

	nano_sem_take_wait(&contexts_lock);

	if (local_port) {
		if (context_port_used(ip_proto, local_port) < 0)
			return NULL;
	} else {
		do {
			local_port = random_rand() | 0x8000;
		} while (context_port_used(ip_proto, local_port) == -EEXIST);
	}

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!contexts[i].tuple.remote_port) {
			contexts[i].tuple.ip_proto = ip_proto;
			contexts[i].tuple.remote_addr = (struct net_addr *)remote_addr;
			contexts[i].tuple.remote_port = remote_port;
			contexts[i].tuple.local_addr = (struct net_addr *)local_addr;
			contexts[i].tuple.local_port = local_port;
			context = &contexts[i];
			break;
		}
	}

	context_sem_give(&contexts_lock);

	return context;
}

void net_context_put(struct net_context *context)
{
	int i;

	nano_sem_take_wait(&contexts_lock);

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (contexts[i].tuple.remote_port) {
			memset(&contexts[i].tuple, 0,
						sizeof(contexts[i].tuple));
			break;
		}
	}

	context_sem_give(&contexts_lock);
}

struct net_tuple *net_context_get_tuple(struct net_context *context)
{
	if (!context) {
		return NULL;
	}

	return &context->tuple;
}

struct nano_fifo *net_context_get_queue(struct net_context *context)
{
	if (!context)
		return NULL;

	return &context->rx_queue;
}

struct simple_udp_connection *
net_context_get_udp_connection(struct net_context *context)
{
	if (!context) {
		return NULL;
	}

	return &context->udp;
}

void net_context_init(void)
{
	int i;

	nano_sem_init(&contexts_lock);

	memset(contexts, 0, sizeof(contexts));

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		nano_fifo_init(&contexts[i].rx_queue);
	}

	context_sem_give(&contexts_lock);
}

int net_context_get_receiver_registered(struct net_context *context)
{
	if (!context) {
		return -ENOENT;
	}

	if (context->receiver_registered) {
		return true;
	}

	return false;
}

void net_context_set_receiver_registered(struct net_context *context)
{
	if (!context) {
		return;
	}

	context->receiver_registered = true;
}
