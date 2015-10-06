/** @file
 @brief Network context API

 An API for applications to define a network connection.
 */

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

#include <nanokernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_ip.h>
#include <net/net_socket.h>

#include "ip/simple-udp.h"
#include "contiki/ipv6/uip-ds6.h"

#include "contiki/os/lib/random.h"
#include "contiki/ipv6/uip-ds6.h"

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
	switch (sys_execution_context_type_get()) {
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

static int context_port_used(enum ip_protocol ip_proto, uint16_t local_port,
			     const struct net_addr *local_addr)

{
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (contexts[i].tuple.ip_proto == ip_proto &&
		    contexts[i].tuple.local_port == local_port &&
		    !memcmp(&contexts[i].tuple.local_addr, local_addr,
			   sizeof(struct net_addr))) {
			return -EEXIST;
		}
	}

	return 0;
}

struct net_context *net_context_get(enum ip_protocol ip_proto,
					const struct net_addr *remote_addr,
					uint16_t remote_port,
					const struct net_addr *local_addr,
					uint16_t local_port)
{
#ifdef CONFIG_NETWORKING_WITH_IPV6
	const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	const uip_ds6_addr_t *uip_addr;
	uip_ipaddr_t ipaddr;
#endif
	int i;
	static struct net_addr laddr;
	struct net_context *context = NULL;

#ifdef CONFIG_NETWORKING_WITH_IPV6
	if (!local_addr || memcmp(&local_addr->in6_addr, &in6addr_any,
				  sizeof(in6addr_any)) == 0) {
		uip_addr = uip_ds6_get_global(-1);
		if (!uip_addr) {
			uip_addr = uip_ds6_get_link_local(-1);
		}
		if (!uip_addr) {
			return NULL;
		}

		memcpy(&laddr, local_addr, sizeof(struct net_addr));
		laddr.in6_addr = *(struct in6_addr *)&uip_addr->ipaddr;
		local_addr = (const struct net_addr *)&laddr;
	}
#else
	if (!local_addr || local_addr->in_addr.s_addr == INADDR_ANY) {
		memcpy(&laddr, local_addr, sizeof(struct net_addr));
		uip_gethostaddr((uip_ipaddr_t *)&laddr.in_addr);
		local_addr = (const struct net_addr *)&laddr;
	}
#endif

	nano_sem_take_wait(&contexts_lock);

	if (local_port) {
		if (context_port_used(ip_proto, local_port, local_addr) < 0) {
			return NULL;
		}
	} else {
		do {
			local_port = random_rand() | 0x8000;
		} while (context_port_used(ip_proto, local_port,
					   local_addr) == -EEXIST);
	}

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!contexts[i].tuple.ip_proto) {
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

	/* Set our local address */
#ifdef CONFIG_NETWORKING_WITH_IPV6
	memcpy(&ipaddr.u8, local_addr->in6_addr.s6_addr, sizeof(ipaddr.u8));
	if (uip_is_addr_mcast(&ipaddr)) {
		uip_ds6_maddr_add(&ipaddr);
	} else {
		uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
	}
#endif

	return context;
}

void net_context_put(struct net_context *context)
{
	nano_sem_take_wait(&contexts_lock);

	memset(&context->tuple, 0, sizeof(context->tuple));
	memset(&context->udp, 0, sizeof(context->udp));
	context->receiver_registered = false;

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
