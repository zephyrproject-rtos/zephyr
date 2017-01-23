/* echo.c - Networking echo server */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "echo-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>

#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

/* Allow binding to ANY IP address. */
#define NET_BIND_ANY_ADDR 1

#if defined(CONFIG_NET_IPV6)
/* admin-local, dynamically allocated multicast address */
#define MCAST_IP6ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

struct in6_addr in6addr_mcast = MCAST_IP6ADDR;

/* Define my IP address where to expect messages */
#define MY_IP6ADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64

static struct in6_addr in6addr_my = MY_IP6ADDR;
#endif /* IPv6 */

#if defined(CONFIG_NET_IPV4)
/* Organization-local 239.192.0.0/14 */
#define MCAST_IP4ADDR { { { 239, 192, 0, 2 } } }

/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IP4ADDR { { { 192, 0, 2, 1 } } }

#if !defined(CONFIG_NET_DHCPV4) || !NET_BIND_ANY_ADDR
static struct in_addr in4addr_my = MY_IP4ADDR;
#endif /* CONFIG_NET_DHCPV4 || !NET_BIND_ANY_ADDR */
#endif /* IPv4 */

#define MY_PORT 4242

#define STACKSIZE 2000
char __noinit __stack thread_stack[STACKSIZE];

#define MAX_DBG_PRINT 64

static struct k_sem quit_lock;

static inline void quit(void)
{
	k_sem_give(&quit_lock);
}

static inline void init_app(void)
{
	NET_INFO("Run echo server");

	k_sem_init(&quit_lock, 0, UINT_MAX);

#if defined(CONFIG_NET_IPV6)
#if defined(CONFIG_NET_SAMPLES_MY_IPV6_ADDR)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_SAMPLES_MY_IPV6_ADDR,
			  (struct sockaddr *)&in6addr_my) < 0) {
		NET_ERR("Invalid IPv6 address %s",
			CONFIG_NET_SAMPLES_MY_IPV6_ADDR);
	}
#endif

	do {
		struct net_if_addr *ifaddr;

		ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
					      &in6addr_my, NET_ADDR_MANUAL, 0);
	} while (0);

	net_if_ipv6_maddr_add(net_if_get_default(), &in6addr_mcast);
#endif

#if defined(CONFIG_NET_IPV4)
#if defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(net_if_get_default());
#else
#if defined(CONFIG_NET_SAMPLES_MY_IPV4_ADDR)
	if (net_addr_pton(AF_INET,
			  CONFIG_NET_SAMPLES_MY_IPV4_ADDR,
			  (struct sockaddr *)&in4addr_my) < 0) {
		NET_ERR("Invalid IPv4 address %s",
			CONFIG_NET_SAMPLES_MY_IPV4_ADDR);
	}
#endif

	net_if_ipv4_addr_add(net_if_get_default(), &in4addr_my,
			     NET_ADDR_MANUAL, 0);
#endif /* CONFIG_NET_DHCPV4 */
#endif /* CONFIG_NET_IPV4 */
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

#if !NET_BIND_ANY_ADDR
	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
#endif

	my_addr6.sin6_family = AF_INET6;
	my_addr6.sin6_port = htons(MY_PORT);
#endif

#if defined(CONFIG_NET_IPV4)
#if !NET_BIND_ANY_ADDR
	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
#endif

	my_addr4.sin_family = AF_INET;
	my_addr4.sin_port = htons(MY_PORT);
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_UDP)
	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, udp_recv6);
	if (ret < 0) {
		NET_ERR("Cannot get network context for IPv6 UDP (%d)",
			ret);
		return false;
	}

	ret = net_context_bind(*udp_recv6, (struct sockaddr *)&my_addr6,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		NET_ERR("Cannot bind IPv6 UDP port %d (%d)",
			ntohs(my_addr6.sin6_port), ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, mcast_recv6);
	if (ret < 0) {
		NET_ERR("Cannot get receiving IPv6 mcast "
			"network context (%d)", ret);
		return false;
	}

	ret = net_context_bind(*mcast_recv6, (struct sockaddr *)&mcast_addr6,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		NET_ERR("Cannot bind IPv6 mcast (%d)", ret);
		return false;
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_UDP)
	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, udp_recv4);
	if (ret < 0) {
		NET_ERR("Cannot get network context for IPv4 UDP (%d)",
			ret);
		return false;
	}

	ret = net_context_bind(*udp_recv4, (struct sockaddr *)&my_addr4,
			       sizeof(struct sockaddr_in));
	if (ret < 0) {
		NET_ERR("Cannot bind IPv4 UDP port %d (%d)",
			ntohs(my_addr4.sin_port), ret);
		return false;
	}
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_TCP)
	if (tcp_recv6) {
		ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
				      tcp_recv6);
		if (ret < 0) {
			NET_ERR("Cannot get network context "
				"for IPv6 TCP (%d)", ret);
			return false;
		}

		ret = net_context_bind(*tcp_recv6,
				       (struct sockaddr *)&my_addr6,
				       sizeof(struct sockaddr_in6));
		if (ret < 0) {
			NET_ERR("Cannot bind IPv6 TCP port %d (%d)",
				ntohs(my_addr6.sin6_port), ret);
			return false;
		}

		ret = net_context_listen(*tcp_recv6, 0);
		if (ret < 0) {
			NET_ERR("Cannot listen IPv6 TCP (%d)", ret);
			return false;
		}
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_TCP)
	if (tcp_recv4) {
		ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
				      tcp_recv4);
		if (ret < 0) {
			NET_ERR("Cannot get network context for IPv4 TCP");
			return false;
		}

		ret = net_context_bind(*tcp_recv4,
				       (struct sockaddr *)&my_addr4,
				       sizeof(struct sockaddr_in));
		if (ret < 0) {
			NET_ERR("Cannot bind IPv4 TCP port %d",
				ntohs(my_addr4.sin_port));
			return false;
		}

		ret = net_context_listen(*tcp_recv4, 0);
		if (ret < 0) {
			NET_ERR("Cannot listen IPv4 TCP");
			return false;
		}
	}
#endif

	return true;
}

static struct net_buf *build_reply_buf(const char *name,
				       struct net_context *context,
				       struct net_buf *buf)
{
	struct net_buf *reply_buf, *frag, *tmp;
	int header_len, recv_len, reply_len;

	NET_INFO("%s received %d bytes", name,
	      net_nbuf_appdatalen(buf));

	if (net_nbuf_appdatalen(buf) == 0) {
		return NULL;
	}

	reply_buf = net_nbuf_get_tx(context);

	NET_ASSERT(reply_buf);

	recv_len = net_buf_frags_len(buf->frags);

	tmp = buf->frags;

	/* First fragment will contain IP header so move the data
	 * down in order to get rid of it.
	 */
	header_len = net_nbuf_appdata(buf) - tmp->data;

	NET_ASSERT(header_len < CONFIG_NET_NBUF_DATA_SIZE);

	/* After this pull, the tmp->data points directly to application
	 * data.
	 */
	net_buf_pull(tmp, header_len);

	while (tmp) {
		frag = net_nbuf_get_data(context);

		if (!net_buf_headroom(tmp)) {
			/* If there is no link layer headers in the
			 * received fragment, then get rid of that also
			 * in the sending fragment. We end up here
			 * if MTU is larger than fragment size, this
			 * is typical for ethernet.
			 */
			net_buf_push(frag, net_buf_headroom(frag));

			frag->len = 0; /* to make fragment empty */

			/* Make sure to set the reserve so that
			 * in sending side we add the link layer
			 * header if needed.
			 */
			net_nbuf_set_ll_reserve(reply_buf, 0);
		}

		NET_ASSERT(net_buf_tailroom(frag) >= tmp->len);

		memcpy(net_buf_add(frag, tmp->len), tmp->data, tmp->len);

		net_buf_frag_add(reply_buf, frag);

		tmp = net_buf_frag_del(buf, tmp);
	}

	reply_len = net_buf_frags_len(reply_buf->frags);

	NET_ASSERT_INFO((recv_len - header_len) == reply_len,
			"Received %d bytes, sending %d bytes",
			recv_len - header_len, reply_len);

	return reply_buf;
}

static inline void pkt_sent(struct net_context *context,
			    int status,
			    void *token,
			    void *user_data)
{
	if (!status) {
		NET_INFO("Sent %d bytes", POINTER_TO_UINT(token));
	}
}

#if defined(CONFIG_NET_UDP)
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

	snprintk(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	set_dst_addr(family, buf, &dst_addr);

	reply_buf = build_reply_buf(dbg, context, buf);

	net_nbuf_unref(buf);

	if (!reply_buf) {
		return;
	}

	ret = net_context_sendto(reply_buf, &dst_addr,
				 family == AF_INET6 ?
				 sizeof(struct sockaddr_in6) :
				 sizeof(struct sockaddr_in),
				 pkt_sent, 0,
				 UINT_TO_POINTER(net_buf_frags_len(reply_buf)),
				 user_data);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_nbuf_unref(reply_buf);
	}
}

static void setup_udp_recv(struct net_context *udp_recv4,
			   struct net_context *udp_recv6)
{
	int ret;

#if defined(CONFIG_NET_IPV6)
	ret = net_context_recv(udp_recv6, udp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive IPv6 UDP packets");
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	ret = net_context_recv(udp_recv4, udp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive IPv4 UDP packets");
	}
#endif /* CONFIG_NET_IPV4 */
}
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
static void tcp_received(struct net_context *context,
			 struct net_buf *buf,
			 int status,
			 void *user_data)
{
	static char dbg[MAX_DBG_PRINT + 1];
	sa_family_t family = net_nbuf_family(buf);
	struct net_buf *reply_buf;
	int ret;

	if (!buf) {
		/* EOF condition */
		return;
	}

	snprintk(dbg, MAX_DBG_PRINT, "TCP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	reply_buf = build_reply_buf(dbg, context, buf);

	net_buf_unref(buf);

	if (!reply_buf) {
		return;
	}

	ret = net_context_send(reply_buf, pkt_sent, K_NO_WAIT,
			       UINT_TO_POINTER(net_buf_frags_len(reply_buf)),
			       NULL);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_nbuf_unref(reply_buf);

		quit();
	}
}

static void tcp_accepted(struct net_context *context,
			 struct sockaddr *addr,
			 socklen_t addrlen,
			 int error,
			 void *user_data)
{
	int ret;

	NET_DBG("Accept called, context %p error %d", context, error);

	ret = net_context_recv(context, tcp_received, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive TCP packet (family %d)",
			net_context_get_family(context));
	}
}

static void setup_tcp_accept(struct net_context *tcp_recv4,
			     struct net_context *tcp_recv6)
{
	int ret;

#if defined(CONFIG_NET_IPV6)
	ret = net_context_accept(tcp_recv6, tcp_accepted, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive IPv6 TCP packets (%d)", ret);
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	ret = net_context_accept(tcp_recv4, tcp_accepted, 0, NULL);
	if (ret < 0) {
		NET_ERR("Cannot receive IPv4 TCP packets (%d)", ret);
	}
#endif /* CONFIG_NET_IPV4 */
}
#endif /* CONFIG_NET_TCP */

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
		NET_ERR("Cannot get network contexts");
		return;
	}

	NET_INFO("Starting to wait");

#if defined(CONFIG_NET_TCP)
	setup_tcp_accept(tcp_recv4, tcp_recv6);
#endif

#if defined(CONFIG_NET_UDP)
	setup_udp_recv(udp_recv4, udp_recv6);
#endif

	k_sem_take(&quit_lock, K_FOREVER);

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

#if defined(CONFIG_NET_L2_BLUETOOTH)
	if (bt_enable(NULL)) {
		NET_ERR("Bluetooth init failed");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

	k_thread_spawn(&thread_stack[0], STACKSIZE,
		       (k_thread_entry_t)receive,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
