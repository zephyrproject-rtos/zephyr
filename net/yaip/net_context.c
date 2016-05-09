/** @file
 @brief Network context API

 An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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
#include <net/net_context.h>

/* Override this in makefile if needed */
#if defined(CONFIG_NET_MAX_CONTEXTS)
#define NET_MAX_CONTEXT CONFIG_NET_MAX_CONTEXTS
#else
#define NET_MAX_CONTEXT 6
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
				    struct net_addr *local_addr,
				    uint16_t local_port)
{
#ifdef CONFIG_NET_IPV6
	const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
#endif
	int i;
	struct net_context *context = NULL;

	/* User must provide storage for the local address. */
	if (!local_addr) {
		return NULL;
	}

#if defined(CONFIG_NET_IPV6)
	if (memcmp(&local_addr->in6_addr, &in6addr_any,
		   sizeof(in6addr_any)) == 0) {
		/* FIXME */
	}
#endif
#if defined(CONFIG_NET_IPV4)
	if (local_addr->in_addr.s_addr == INADDR_ANY) {
		/* FIXME */
	}
#endif

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

	if (local_port) {
		if (context_port_used(ip_proto, local_port, local_addr) < 0) {
			return NULL;
		}
	} else {
		do {
			local_port = sys_rand32_get() | 0x8000;
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

	return context;
}

void net_context_put(struct net_context *context)
{
	if (!context) {
		return;
	}

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

	if (context->tuple.ip_proto == IPPROTO_UDP) {
		/* FIXME */
	}

	memset(&context->tuple, 0, sizeof(context->tuple));

	context_sem_give(&contexts_lock);
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
