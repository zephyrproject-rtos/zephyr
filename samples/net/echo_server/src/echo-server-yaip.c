/* echo.c - Networking echo server */

/*
 * Copyright (c) 2016 Intel Corporation.
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

#if 1
#define SYS_LOG_DOMAIN "echo-server"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_DEBUG 1
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

#if defined(CONFIG_NETWORKING_WITH_BT)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif /* CONFIG_NET_TESTING */

#if defined(CONFIG_NET_IPV6)
/* admin-local, dynamically allocated multicast address */
#define MCAST_IP6ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

struct in6_addr in6addr_mcast = MCAST_IP6ADDR;

/* Define my IP address where to expect messages */
#if !defined(CONFIG_NET_TESTING)
#define MY_IP6ADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64
#endif

#if defined(CONFIG_NET_TESTING)
static struct in6_addr in6addr_my = IN6ADDR_ANY_INIT;
#else
static struct in6_addr in6addr_my = MY_IP6ADDR;
#endif
#endif /* IPv6 */

#if defined(CONFIG_NET_IPV4)
/* Organization-local 239.192.0.0/14 */
#define MCAST_IP4ADDR { { { 239, 192, 0, 2 } } }

/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IP4ADDR { { { 192, 0, 2, 1 } } }

static struct in_addr in4addr_my = MY_IP4ADDR;
#endif /* IPv4 */

#define MY_PORT 4242

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

/* How many tics to wait for a network packet */
#if 1
#define WAIT_TICKS (sys_clock_ticks_per_sec)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

static struct nano_sem quit_lock;

static inline void quit(void)
{
	nano_sem_give(&quit_lock);
}

static inline void init_app(void)
{
	NET_INFO("%s: run echo server", __func__);

	nano_sem_init(&quit_lock);

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#endif

#if defined(CONFIG_NET_IPV6)
	net_if_ipv6_addr_add(net_if_get_default(), &in6addr_my,
			     NET_ADDR_MANUAL, 0);

	net_if_ipv6_maddr_add(net_if_get_default(), &in6addr_mcast);
#endif

#if defined(CONFIG_NET_IPV4)
	net_if_ipv4_addr_add(net_if_get_default(), &in4addr_my,
			     NET_ADDR_MANUAL, 0);
#endif
}

static inline bool get_context(struct net_context **udp_recv4,
			       struct net_context **udp_recv6,
			       struct net_context **tcp_recv4,
			       struct net_context **tcp_recv6,
			       struct net_context **mcast_recv6)
{
	int ret;

#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 mcast_addr6 = { 0 };
	struct sockaddr_in6 my_addr6 = { 0 };
#endif

#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in my_addr4 = { 0 };
#endif

#if defined(CONFIG_NET_IPV6)
	net_ipaddr_copy(&mcast_addr6.sin6_addr, &in6addr_mcast);
	mcast_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = AF_INET6;
	my_addr6.sin6_port = htons(MY_PORT);
#endif

#if defined(CONFIG_NET_IPV4)
	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(MY_PORT);
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_UDP)
	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, udp_recv6);
	if (ret < 0) {
		NET_ERR("%s: Cannot get network context for IPv6 UDP (%d)",
		      __func__, ret);
		return false;
	}

	ret = net_context_bind(*udp_recv6, (struct sockaddr *)&my_addr6);
	if (ret < 0) {
		NET_ERR("%: Cannot bind IPv6 UDP port %d (%d)",
		      ntohs(my_addr6.sin6_port), ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, mcast_recv6);
	if (ret < 0) {
		NET_ERR("%s: Cannot get receiving IPv6 mcast "
		      "network context (%d)", __func__, ret);
		return false;
	}

	ret = net_context_bind(*mcast_recv6, (struct sockaddr *)&mcast_addr6);
	if (ret < 0) {
		NET_ERR("%: Cannot bind IPv6 mcast (%d)", ret);
		return false;
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_UDP)
	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, udp_recv4);
	if (ret < 0) {
		NET_ERR("%s: Cannot get network context for IPv4 UDP (%d)",
		      __func__, ret);
		return false;
	}

	ret = net_context_bind(*udp_recv4, (struct sockaddr *)&my_addr4);
	if (ret < 0) {
		NET_ERR("%: Cannot bind IPv4 UDP port %d (%d)",
		      ntohs(my_addr4.sin_port), ret);
		return false;
	}
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_TCP)
	if (tcp_recv6) {
		ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
				      tcp_recv6);
		if (ret < 0) {
			NET_ERR("%s: Cannot get network context "
			      "for IPv6 TCP (%d)", __func__, ret);
			return false;
		}

		ret = net_context_bind(*tcp_recv6,
				       (struct sockaddr *)&my_addr6);
		if (ret < 0) {
			NET_ERR("%: Cannot bind IPv6 TCP port %d (%d)",
			      ntohs(my_addr6.sin_port), ret);
			return false;
		}

		ret = net_context_listen(*tcp_recv6, 0);
		if (ret < 0) {
			NET_ERR("%s: Cannot listen IPv6 TCP (%d)", ret);
			return false;
		}
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_TCP)
	if (tcp_recv4) {
		ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
				      tcp_recv4);
		if (ret < 0) {
			NET_ERR("%s: Cannot get network context for IPv4 TCP",
			      __func__);
			return false;
		}

		ret = net_context_bind(*tcp_recv4,
				       (struct sockaddr *)&my_addr4);
		if (ret < 0) {
			NET_ERR("%: Cannot bind IPv4 TCP port %d",
			      ntohs(my_addr4.sin_port));
			return false;
		}

		ret = net_context_listen(*tcp_recv6, 0);
		if (ret < 0) {
			NET_ERR("%s: Cannot listen IPv6 TCP");
			return false;
		}
	}
#endif

	return true;
}

static struct net_buf *udp_recv(const char *name,
				struct net_context *context,
				struct net_buf *buf)
{
	struct net_buf *reply_buf, *frag, *tmp;
	int header_len, recv_len, reply_len;

	NET_INFO("%s received %d bytes", name,
	      net_nbuf_appdatalen(buf));

	reply_buf = net_nbuf_get_tx(context);

	NET_ASSERT(reply_buf);

	recv_len = net_buf_frags_len(buf->frags);

	tmp = buf->frags;

	/* First fragment will contain IP header so move the data
	 * down in order to get rid of it.
	 */
	header_len = net_nbuf_appdata(buf) - tmp->data;

	NET_ASSERT(header_len < CONFIG_NET_NBUF_DATA_SIZE);

	net_buf_pull(tmp, header_len);

	while (tmp) {
		frag = net_nbuf_get_data(context);

		memcpy(net_buf_add(frag, tmp->len), tmp->data, tmp->len);

		net_buf_frag_add(reply_buf, frag);

		net_buf_frag_del(buf, tmp);

		net_nbuf_unref(tmp);

		tmp = buf->frags;
	}

	reply_len = net_buf_frags_len(reply_buf->frags);

	NET_ASSERT_INFO(recv_len != reply_len,
			"Received %d bytes, sending %d bytes",
			recv_len, reply_len);

	return reply_buf;
}

#if defined(CONFIG_NET_UDP)
#define MAX_DBG_PRINT 64

static inline void udp_sent(struct net_context *context,
			    int status,
			    void *token,
			    void *user_data)
{
	if (!status) {
		NET_INFO("Sent %d bytes", POINTER_TO_UINT(token));
	}
}

static inline void set_dst_addr(sa_family_t family,
				struct net_buf *buf,
				struct sockaddr *dst_addr)
{
#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_BUF(buf)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = NET_UDP_BUF(buf)->src_port;
	}
#endif /* CONFIG_NET_IPV6) */

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&NET_IPV4_BUF(buf)->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = NET_UDP_BUF(buf)->src_port;
	}
#endif /* CONFIG_NET_IPV6) */
}

static void udp_received(struct net_context *context,
			 struct net_buf *buf,
			 int status,
			 void *user_data)
{
	struct net_buf *reply_buf;
	struct sockaddr dst_addr;
	sa_family_t family = net_nbuf_family(buf);
	static char dbg[MAX_DBG_PRINT + 1];
	int ret;

	snprintf(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	set_dst_addr(family, buf, &dst_addr);

	reply_buf = udp_recv(dbg, context, buf);

	net_nbuf_unref(buf);

	ret = net_context_sendto(reply_buf, &dst_addr, udp_sent, 0,
				 UINT_TO_POINTER(net_buf_frags_len(reply_buf)),
				 user_data);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_nbuf_unref(reply_buf);

		quit();
	}
}

static void setup_udp_recv(struct net_context *udp_recv4,
			   struct net_context *udp_recv6)
{
	int ret;

#if defined(CONFIG_NET_IPV6)
	ret = net_context_recv(udp_recv6, udp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("%s: Cannot receive IPv6 UDP packets");
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	ret = net_context_recv(udp_recv4, udp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("%s: Cannot receive IPv4 UDP packets");
	}
#endif /* CONFIG_NET_IPV4 */
}
#endif /* CONFIG_NET_UDP */

void receive(void)
{
	struct net_context *udp_recv4 = { 0 };
	struct net_context *udp_recv6 = { 0 };
	struct net_context *tcp_recv4 = { 0 };
	struct net_context *tcp_recv6 = { 0 };
	struct net_context *mcast_recv6 = { 0 };

	if (!get_context(&udp_recv4, &udp_recv6,
			 &tcp_recv4, &tcp_recv6,
			 &mcast_recv6)) {
		NET_ERR("%s: Cannot get network contexts", __func__);
		return;
	}

	NET_INFO("Starting to wait");

#if defined(CONFIG_NET_TCP)
	setup_tcp_accept(tcp_recv4, tcp_recv6);
#endif

#if defined(CONFIG_NET_UDP)
	setup_udp_recv(udp_recv4, udp_recv6);
#endif

	nano_sem_take(&quit_lock, TICKS_UNLIMITED);

	NET_INFO("Stopping...");

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_UDP)
	net_context_put(udp_recv6);
	net_context_put(mcast_recv6);
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_UDP)
	net_context_put(udp_recv4);
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_TCP)
	net_context_put(tcp_recv6);
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_TCP)
	net_context_put(tcp_recv4);
#endif
}

void main(void)
{
	init_app();

#if defined(CONFIG_NETWORKING_WITH_BT)
	if (bt_enable(NULL)) {
		NET_ERR("Bluetooth init failed");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

#if defined(CONFIG_MICROKERNEL)
	receive();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)receive, 0, 0, 7, 0);
#endif
}
