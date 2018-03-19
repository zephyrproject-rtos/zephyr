/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/sys_log.h>

#include <net/net_pkt.h>
#include <net/net_context.h>

/* Set this to 1 if you want to see what is being sent to server */
#define DEBUG_PRINTING 0

static struct net_context *ctx;
static struct sockaddr server_addr;

/* FIXME: As there is no way to figure out these values in the hook
 * function, use some pre-defined values. Change this to use the
 * real facility and severity of the logging call when that info is
 * available.
 */
static const int facility = 16; /* local0 */
static const int severity = 6; /* info */

#define DATE_EPOCH "1970-01-01T00:00:00.000000-00:00"
static char date[sizeof(DATE_EPOCH)];

#if defined(CONFIG_NET_IPV6) || CONFIG_NET_HOSTNAME_ENABLE
#define MAX_HOSTNAME_LEN NET_IPV6_ADDR_LEN
#else
#define MAX_HOSTNAME_LEN NET_IPV4_ADDR_LEN
#endif

static char hostname[MAX_HOSTNAME_LEN + 1];

NET_PKT_SLAB_DEFINE(syslog_tx_pkts, CONFIG_SYS_LOG_BACKEND_NET_MAX_BUF);
NET_BUF_POOL_DEFINE(syslog_tx_bufs, CONFIG_SYS_LOG_BACKEND_NET_MAX_BUF,
		    CONFIG_SYS_LOG_BACKEND_NET_MAX_BUF_SIZE,
		    CONFIG_NET_BUF_USER_DATA_SIZE, NULL);

static struct k_mem_slab *get_tx_slab(void)
{
	return &syslog_tx_pkts;
}

struct net_buf_pool *get_data_pool(void)
{
	return &syslog_tx_bufs;
}

static void fill_header(struct net_buf *buf)
{
	snprintk(net_buf_tail(buf),
		 net_buf_tailroom(buf),
		 "<%d>1 %s %s - - - - ",
		 facility * 8 + severity,
		 date,
		 hostname);

	net_buf_add(buf, strlen(buf->data));
}

static void syslog_hook_net(const char *fmt, ...)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t *ptr;
	va_list vargs;
	int ret;

	pkt = net_pkt_get_tx(ctx, K_NO_WAIT);
	if (!pkt) {
		return;
	}

	frag = net_pkt_get_data(ctx, K_NO_WAIT);
	if (!frag) {
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_frag_add(pkt, frag);

	fill_header(frag);

	va_start(vargs, fmt);

	ptr = net_buf_tail(frag);

	ret = vsnprintk(ptr, (net_buf_tailroom(frag) - 1), fmt, vargs);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return;
	}

	va_end(vargs);

	if (ret > 0 && ptr[ret - 1] == '\n') {
		/* No need to send \n to peer so strip it away */
		ret--;
	}

	net_buf_add(frag, ret);

#if DEBUG_PRINTING
	{
		static u32_t count;

		printk("%d:%s", ++count, frag->data);
	}
#endif
	ret = net_context_send(pkt, NULL, K_NO_WAIT, NULL, NULL);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
}

void syslog_net_hook_install(void)
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 local_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};
#endif
#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in local_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};
#endif
	struct sockaddr *local_addr = NULL;
	socklen_t local_addr_len = 0;
	socklen_t server_addr_len = 0;
	int ret;

	net_sin(&server_addr)->sin_port = htons(514);

	ret = net_ipaddr_parse(CONFIG_SYS_LOG_BACKEND_NET_SERVER,
			       sizeof(CONFIG_SYS_LOG_BACKEND_NET_SERVER) - 1,
			       &server_addr);
	if (!ret) {
		SYS_LOG_ERR("Cannot configure syslog server address");
		return;
	}

#if defined(CONFIG_NET_IPV4)
	if (server_addr.sa_family == AF_INET) {
		local_addr = (struct sockaddr *)&local_addr4;
		local_addr_len = sizeof(struct sockaddr_in);
		server_addr_len = sizeof(struct sockaddr_in);
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (server_addr.sa_family == AF_INET6) {
		local_addr = (struct sockaddr *)&local_addr6;
		local_addr_len = sizeof(struct sockaddr_in6);
		server_addr_len = sizeof(struct sockaddr_in6);
	}
#endif

	ret = net_context_get(server_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &ctx);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot get context (%d)", ret);
		return;
	}

#if CONFIG_NET_HOSTNAME_ENABLE
	memcpy(hostname, net_hostname_get(), MAX_HOSTNAME_LEN);
#else /* CONFIG_NET_HOSTNAME_ENABLE */
	if (server_addr.sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		const struct in6_addr *src;

		src = net_if_ipv6_select_src_addr(
			NULL, &net_sin6(&server_addr)->sin6_addr);
		if (src) {
			net_addr_ntop(AF_INET6, src, hostname,
				      MAX_HOSTNAME_LEN);

			net_ipaddr_copy(&local_addr6.sin6_addr, src);
		} else {
			goto unknown;
		}
#else
		goto unknown;
#endif
	} else if (server_addr.sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if_ipv4 *ipv4;
		struct net_if *iface;

		iface = net_if_ipv4_select_src_iface(
					&net_sin(&server_addr)->sin_addr);
		ipv4 = iface->config.ip.ipv4;

		net_ipaddr_copy(&local_addr4.sin_addr,
				&ipv4->unicast[0].address.in_addr);

		net_addr_ntop(AF_INET, &local_addr4.sin_addr, hostname,
			      MAX_HOSTNAME_LEN);
#else
		goto unknown;
#endif
	} else {
	unknown:
		strncpy(hostname, "zephyr", MAX_HOSTNAME_LEN);
	}
#endif /* CONFIG_NET_HOSTNAME_ENABLE */

	ret = net_context_bind(ctx, local_addr, local_addr_len);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot bind context (%d)", ret);
		return;
	}

	ret = net_context_connect(ctx, &server_addr, server_addr_len,
				  NULL, K_NO_WAIT, NULL);

	/* We do not care about return value for this UDP connect call that
	 * basically does nothing. Calling the connect is only useful so that
	 * we can see the syslog connection in net-shell.
	 */

	net_context_setup_pools(ctx, get_tx_slab, get_data_pool);

	syslog_hook_install(syslog_hook_net);
}
