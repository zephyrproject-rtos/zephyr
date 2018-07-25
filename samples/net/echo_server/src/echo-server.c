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
#include <linker/sections.h>
#include <errno.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <net/net_app.h>

#include "common.h"

/* The startup time needs to be longish if DHCP is enabled as setting
 * DHCP up takes some time.
 */
#define APP_STARTUP_TIME K_SECONDS(20)

#define BUF_TIMEOUT K_MSEC(100)

#define APP_BANNER "Run echo server"

static struct k_sem quit_lock;

void panic(const char *msg)
{
	if (msg) {
		NET_ERR("%s", msg);
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

void quit(void)
{
	k_sem_give(&quit_lock);
}

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt,
				u8_t proto_len)
{
	struct net_pkt *reply_pkt;
	struct net_buf *frag;
	int header_len = 0;
	int recv_len;
	int ret;

	NET_INFO("%s received %d bytes", name, net_pkt_appdatalen(pkt));

	if (net_pkt_appdatalen(pkt) == 0) {
		return NULL;
	}

	reply_pkt = net_app_get_net_pkt(ctx, net_pkt_family(pkt), BUF_TIMEOUT);
	if (!reply_pkt) {
		return NULL;
	}

	NET_ASSERT(net_pkt_family(reply_pkt) == net_pkt_family(pkt));

	recv_len = net_pkt_get_len(pkt);

	/* If we have link layer headers, then get rid of them here. */
	if (recv_len != net_pkt_appdatalen(pkt)) {
		/* First fragment will contain IP header so move the data
		 * down in order to get rid of it.
		 */
		header_len = net_pkt_ip_hdr_len(pkt);
		header_len += net_pkt_ipv6_ext_len(pkt);
		header_len += proto_len;

		ret = net_pkt_pull(pkt, 0, header_len);
		if (ret != 0) {
			net_pkt_unref(reply_pkt);
			return NULL;
		}
	}

	net_pkt_set_appdatalen(reply_pkt, net_pkt_appdatalen(pkt));

	frag = net_pkt_copy_all(pkt, 0, BUF_TIMEOUT);
	if (!frag) {
		NET_ERR("Failed to copy all data");
		net_pkt_unref(reply_pkt);
		return NULL;
	}

	net_pkt_frag_add(reply_pkt, frag);

	return reply_pkt;
}

void pkt_sent(struct net_app_ctx *ctx,
	     int status,
	     void *user_data_send,
	     void *user_data)
{
	if (!status) {
		NET_INFO("Sent %d bytes", POINTER_TO_UINT(user_data_send));
	}
}

static inline int init_app(void)
{
	k_sem_init(&quit_lock, 0, UINT_MAX);

	return 0;
}

void main(void)
{
	init_app();

	if (IS_ENABLED(CONFIG_NET_TCP)) {
		start_tcp();
	}

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		start_udp();
	}

	k_sem_take(&quit_lock, K_FOREVER);

	NET_INFO("Stopping...");

	if (IS_ENABLED(CONFIG_NET_TCP)) {
		stop_tcp();
	}

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}
