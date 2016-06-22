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

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_CONTEXT
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#include <nanokernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_ip.h>
#include <net/net_socket.h>

#include "contiki/ip/simple-udp.h"
#include "contiki/ipv6/uip-ds6.h"

#include "contiki/os/lib/random.h"
#include "contiki/ipv6/uip-ds6.h"

#ifdef CONFIG_NETWORKING_WITH_TCP
#include "contiki/os/sys/process.h"
#include "contiki/ip/psock.h"
#endif

#if !defined(CONFIG_NETWORK_IP_STACK_DEBUG_CONTEXT)
#undef NET_DBG
#define NET_DBG(...)
#endif

int net_context_get_receiver_registered(struct net_context *context);

struct net_context {
	/* Connection tuple identifies the connection */
	struct net_tuple tuple;

	/* Application receives data via this fifo */
	struct nano_fifo rx_queue;

	/* Application connection data */
	union {
		struct simple_udp_connection udp;

#ifdef CONFIG_NETWORKING_WITH_TCP
		struct {
			/* Proto socket that handles one TCP connection. */
			struct psock ps;
			struct process tcp;
			enum net_tcp_type tcp_type;
			int connection_status;
			void *conn;
			struct net_buf *pending;
			uint8_t retry_count;
		};
#endif
	};

	bool receiver_registered;
};

/* Override this in makefile if needed */
#if defined(CONFIG_NET_MAX_CONTEXTS)
#define NET_MAX_CONTEXT CONFIG_NET_MAX_CONTEXTS
#else
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
				    struct net_addr *local_addr,
				    uint16_t local_port)
{
#ifdef CONFIG_NETWORKING_WITH_IPV6
	const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	const uip_ds6_addr_t *uip_addr;
	uip_ipaddr_t ipaddr;
#endif
	int i;
	struct net_context *context = NULL;

	/* User must provide storage for the local address. */
	if (!local_addr) {
		return NULL;
	}

#ifdef CONFIG_NETWORKING_WITH_IPV6
	if (memcmp(&local_addr->in6_addr, &in6addr_any,
				  sizeof(in6addr_any)) == 0) {
		uip_addr = uip_ds6_get_global(-1);
		if (!uip_addr) {
			uip_addr = uip_ds6_get_link_local(-1);
		}
		if (!uip_addr) {
			return NULL;
		}

		memcpy(&local_addr->in6_addr, &uip_addr->ipaddr,
		       sizeof(struct in6_addr));
	}
#else
	if (local_addr->in_addr.s_addr[0] == INADDR_ANY) {
		uip_gethostaddr((uip_ipaddr_t *)&local_addr->in_addr);
	}
#endif

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

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
	if (!context) {
		return;
	}

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

	if (context->tuple.ip_proto == IPPROTO_UDP) {
		if (net_context_get_receiver_registered(context)) {
			struct simple_udp_connection *udp =
				net_context_get_udp_connection(context);
				simple_udp_unregister(udp);
		}
	}

#ifdef CONFIG_NETWORKING_WITH_TCP
	if (context->tcp_type == NET_TCP_TYPE_SERVER) {
		tcp_unlisten(UIP_HTONS(context->tuple.local_port),
			     &context->tcp);
	}
#endif

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

#ifdef CONFIG_NETWORKING_WITH_TCP
static int handle_tcp_connection(struct psock *p, enum tcp_event_type type,
				 struct net_buf *buf)
{
	PSOCK_BEGIN(p);

	if (type == TCP_WRITE_EVENT) {
		NET_DBG("Trying to send %d bytes data\n", uip_appdatalen(buf));
		PSOCK_SEND(p, buf);
	}

	PSOCK_END(p);
}

int net_context_tcp_send(struct net_buf *buf)
{
	bool connected, reset;

	/* Prepare data to be sent */

	process_post_synch(&ip_buf_context(buf)->tcp,
			   tcpip_event,
			   INT_TO_POINTER(TCP_WRITE_EVENT),
			   buf);

	connected = uip_flags(buf) & UIP_CONNECTED;
	reset = uip_flags(buf) & UIP_ABORT;

	/* If the buffer ref is 1, then the buffer was sent and it
	 * is cleared already.
	 */
	if (buf->ref == 1) {
		return 0;
	}

	return ip_buf_sent_status(buf);
}

/* This is called by contiki/ip/tcpip.c:tcpip_uipcall() when packet
 * is processed.
 */
PROCESS_THREAD(tcp, ev, data, buf, user_data)
{
	NET_DBG("tcp %p ev %d data %p buf %p user_data %p next line %d\n",
		process_thread_tcp, ev, data, buf, user_data,
		process_pt->lc);

	PROCESS_BEGIN();

	while(1) {
		PROCESS_YIELD_UNTIL(ev == tcpip_event);

	try_send:
		if (POINTER_TO_INT(data) == TCP_WRITE_EVENT) {
			/* We want to send data to peer. */
			struct net_context *context = user_data;

			if (!context) {
				continue;
			}

			context->connection_status = ip_buf_sent_status(buf);

			do {
				context = user_data;
				if (!context || !buf) {
					break;
				}

				if (!context->ps.net_buf ||
				    context->ps.net_buf != buf) {
					NET_DBG("psock init %p buf %p\n",
						&context->ps, buf);
					PSOCK_INIT(&context->ps, buf);
				}

				handle_tcp_connection(&context->ps,
						      POINTER_TO_INT(data),
						      buf);

				PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

				if (uip_timedout(buf)) {
					break;
				}

				if (POINTER_TO_INT(data) != TCP_WRITE_EVENT) {
					goto read_data;
				}
			} while(!(uip_closed(buf)  ||
				  uip_aborted(buf) ||
				  uip_timedout(buf)));

			context = user_data;

			if (uip_timedout(buf)) {
				ip_buf_sent_status(buf) = -ETIMEDOUT;
				if (context) {
					context->connection_status = -ETIMEDOUT;
				}
				continue;
			}

			if (context &&
			    context->tcp_type == NET_TCP_TYPE_CLIENT) {
				NET_DBG("\nConnection closed.\n");
				ip_buf_sent_status(buf) = -ECONNRESET;
			}

			continue;
		} else {
			if (buf && uip_aborted(buf)) {
				struct net_context *context = user_data;
				NET_DBG("Connection aborted context %p\n",
					user_data);
				context->connection_status = -ECONNRESET;
				continue;
			}

			if (buf && uip_connected(buf)) {
				struct net_context *context = user_data;
				NET_DBG("Connection established context %p\n",
					user_data);
				context->connection_status = -EALREADY;
				data = INT_TO_POINTER(TCP_WRITE_EVENT);
				goto try_send;
			}
		}

	read_data:
		/* We are receiving data from peer. */
		if (buf && uip_newdata(buf)) {
			struct net_buf *clone;

			if (!uip_len(buf)) {
				continue;
			}

			/* Note that uIP stack will reuse the buffer when
			 * sending ACK to peer host. The sending will happen
			 * right after this function returns. Because of this
			 * we cannot use the same buffer to pass data to
			 * application.
			 */
			clone = net_buf_clone(buf);
			if (!clone) {
				NET_ERR("No enough RX buffers, "
					"packet %p discarded\n", buf);
				continue;
			}

			ip_buf_appdata(clone) = uip_buf(clone) +
				(ip_buf_appdata(buf) - (void *)uip_buf(buf));
			ip_buf_appdatalen(clone) = uip_len(buf);
			ip_buf_len(clone) = uip_len(buf) + UIP_IPTCPH_LEN + UIP_LLH_LEN;
			ip_buf_context(clone) = user_data;
			if (!ip_buf_context(buf)) {
				ip_buf_context(buf) = user_data;
			}
			uip_set_conn(clone) = uip_conn(buf);
			uip_flags(clone) = uip_flags(buf);
			uip_flags(clone) |= UIP_CONNECTED;

			NET_DBG("packet received context %p buf %p len %d "
				"appdata %p appdatalen %d\n",
				ip_buf_context(clone),
				clone,
				ip_buf_len(clone),
				ip_buf_appdata(clone),
				ip_buf_appdatalen(clone));

			nano_fifo_put(net_context_get_queue(user_data), clone);

			ip_buf_sent_status(buf) = 1;

			/* We let the application to read the data now */
			fiber_yield();
		}
	}

	PROCESS_END();
}

int net_context_tcp_init(struct net_context *context, struct net_buf *buf,
			 enum net_tcp_type tcp_type)
{
	if (!context || context->tuple.ip_proto != IPPROTO_TCP) {
		return -EINVAL;
	}

	if (context->receiver_registered) {
		return 0;
	}

	context->receiver_registered = true;

	if (context->tcp_type == NET_TCP_TYPE_UNKNOWN) {
		/* This is the first call to this init func.
		 * If we are called by net_receive() first, then
		 * we are working as a server, if net_send() called
		 * us first, then we are the client.
		 */
		context->tcp_type = tcp_type;
	} else if (context->tcp_type != tcp_type) {
		/* This means that we have already selected that we
		 * are either client or server. Use the context
		 * value.
		 */
		return 0;
	}

	context->tcp.thread = process_thread_tcp;

	if (context->tcp_type == NET_TCP_TYPE_SERVER) {
		context->tcp.name = "TCP server";

		NET_DBG("Listen to TCP port %d\n", context->tuple.local_port);
		tcp_listen(UIP_HTONS(context->tuple.local_port),
			   &context->tcp);
#if UIP_ACTIVE_OPEN
	} else {
		context->tcp.name = "TCP client";
		context->connection_status = -EINPROGRESS;

#ifdef CONFIG_NETWORKING_WITH_IPV6
		NET_DBG("Connecting to ");
		PRINT6ADDR((const uip_ipaddr_t *)&context->tuple.remote_addr->in6_addr);
		PRINTF(" port %d\n", context->tuple.remote_port);

		tcp_connect((uip_ipaddr_t *)
			    &context->tuple.remote_addr->in6_addr,
			    UIP_HTONS(context->tuple.remote_port),
			    context, &context->tcp, buf);
#else /* CONFIG_NETWORKING_WITH_IPV6 */
		NET_DBG("Connecting to ");
		PRINT6ADDR((const uip_ipaddr_t *)&context->tuple.remote_addr->in_addr);
		PRINTF(" port %d\n", context->tuple.remote_port);

		tcp_connect((uip_ipaddr_t *)
			    &context->tuple.remote_addr->in_addr,
			    UIP_HTONS(context->tuple.remote_port),
			    context, &context->tcp, buf);
#endif /* CONFIG_NETWORKING_WITH_IPV6 */
#endif /* UIP_ACTIVE_OPEN */
	}

	context->tcp.next = NULL;
	process_start(&context->tcp, NULL, context);

	return 0;
}
#endif /* TCP */

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

void net_context_unset_receiver_registered(struct net_context *context)
{
	if (!context) {
		return;
	}

	context->receiver_registered = false;
}

int net_context_get_connection_status(struct net_context *context)
{
	if (!context) {
		return -ENOENT;
	}

#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return 0;
#else
	if (context->tuple.ip_proto == IPPROTO_TCP) {
		return context->connection_status;
	} else {
		return 0;
	}
#endif
}

void net_context_set_connection_status(struct net_context *context,
				      int status)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return;
#else
	if (!context) {
		return;
	}

	if (context->tuple.ip_proto == IPPROTO_TCP) {
		NET_DBG("context %p status %d\n", context, status);
		context->connection_status = status;
	}
#endif
}

void *net_context_get_internal_connection(struct net_context *context)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return NULL;
#else
	if (!context) {
		return NULL;
	}

	if (context->tuple.ip_proto == IPPROTO_TCP) {
		return context->conn;
	} else {
		return NULL;
	}
#endif
}

void net_context_set_internal_connection(struct net_context *context,
					 void *conn)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return;
#else
	if (!context) {
		return;
	}

	if (context->tuple.ip_proto == IPPROTO_TCP) {
		context->conn = conn;
	}
#endif
}

struct net_context *net_context_find_internal_connection(void *conn)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return NULL;
#else
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (contexts[i].conn == conn) {
			return &contexts[i];
		}
	}

	return NULL;
#endif
}

struct net_buf *net_context_tcp_get_pending(struct net_context *context)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return NULL;
#else
	if (!context) {
		return NULL;
	}

	return context->pending;
#endif
}

void net_context_tcp_set_pending(struct net_context *context,
				 struct net_buf *buf)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return;
#else
	if (!context) {
		return;
	}

	context->pending = buf;
#endif
}

void net_context_tcp_set_retry_count(struct net_context *context,
				     uint8_t count)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return;
#else
	if (!context) {
		return;
	}

	context->retry_count = count;
#endif
}

uint8_t net_context_tcp_get_retry_count(struct net_context *context)
{
#if !defined(CONFIG_NETWORKING_WITH_TCP)
	return 0;
#else
	if (!context) {
		return 0;
	}

	return context->retry_count;
#endif
}
