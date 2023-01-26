/*
 * Copyright (c) 2019-2020, Prevas A/S
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the mcumgr SMP protocol.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_udp.h>
#include <errno.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>
#endif

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_udp);

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_UDP_MTU != 0, "CONFIG_MCUMGR_TRANSPORT_UDP_MTU must be > 0");

struct config {
	int sock;
	const char *proto;
	struct smp_transport smp_transport;
	char recv_buffer[CONFIG_MCUMGR_TRANSPORT_UDP_MTU];
	struct k_thread thread;
	K_KERNEL_STACK_MEMBER(stack, CONFIG_MCUMGR_TRANSPORT_UDP_STACK_SIZE);
};

struct configs {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	struct config ipv4;
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	struct config ipv6;
#endif
};

static struct configs configs = {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	.ipv4 = {
		.proto = "IPv4",
		.sock  = -1,
	},
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	.ipv6 = {
		.proto = "IPv6",
		.sock  = -1,
	},
#endif
};

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT
static struct net_mgmt_event_callback smp_udp_mgmt_cb;
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
static int smp_udp4_tx(struct net_buf *nb)
{
	int ret;
	struct sockaddr *addr = net_buf_user_data(nb);

	ret = sendto(configs.ipv4.sock, nb->data, nb->len, 0, addr, sizeof(*addr));

	if (ret < 0) {
		ret = MGMT_ERR_EINVAL;
	} else {
		ret = MGMT_ERR_EOK;
	}

	smp_packet_free(nb);

	return ret;
}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
static int smp_udp6_tx(struct net_buf *nb)
{
	int ret;
	struct sockaddr *addr = net_buf_user_data(nb);

	ret = sendto(configs.ipv6.sock, nb->data, nb->len, 0, addr, sizeof(*addr));

	if (ret < 0) {
		ret = MGMT_ERR_EINVAL;
	} else {
		ret = MGMT_ERR_EOK;
	}

	smp_packet_free(nb);

	return ret;
}
#endif

static uint16_t smp_udp_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	return CONFIG_MCUMGR_TRANSPORT_UDP_MTU;
}

static int smp_udp_ud_copy(struct net_buf *dst, const struct net_buf *src)
{
	struct sockaddr *src_ud = net_buf_user_data(src);
	struct sockaddr *dst_ud = net_buf_user_data(dst);

	net_ipaddr_copy(dst_ud, src_ud);

	return MGMT_ERR_EOK;
}

static void smp_udp_receive_thread(void *p1, void *p2, void *p3)
{
	struct config *conf = (struct config *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Started (%s)", conf->proto);

	while (1) {
		struct sockaddr addr;
		socklen_t addr_len = sizeof(addr);

		int len = recvfrom(conf->sock, conf->recv_buffer,
				   CONFIG_MCUMGR_TRANSPORT_UDP_MTU,
				   0, &addr, &addr_len);

		if (len > 0) {
			struct sockaddr *ud;
			struct net_buf *nb;

			/* store sender address in user data for reply */
			nb = smp_packet_alloc();
			if (!nb) {
				LOG_ERR("Failed to allocate mcumgr buffer");
				/* No free space, drop smp frame */
				continue;
			}
			net_buf_add_mem(nb, conf->recv_buffer, len);
			ud = net_buf_user_data(nb);
			net_ipaddr_copy(ud, &addr);

			smp_rx_req(&conf->smp_transport, nb);
		} else if (len < 0) {
			LOG_ERR("recvfrom error (%s): %i", conf->proto, errno);
		}
	}
}

static int smp_udp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	smp_transport_init(&configs.ipv4.smp_transport,
			   smp_udp4_tx, smp_udp_get_mtu,
			   smp_udp_ud_copy, NULL, NULL);
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	smp_transport_init(&configs.ipv6.smp_transport,
			   smp_udp6_tx, smp_udp_get_mtu,
			   smp_udp_ud_copy, NULL, NULL);
#endif

	return MGMT_ERR_EOK;
}

static int create_socket(struct sockaddr *addr, const char *proto)
{
	int sock = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	int err = errno;

	if (sock < 0) {
		LOG_ERR("Could not open receive socket (%s), err: %i",
			proto, err);

		return -err;
	}

	if (bind(sock, addr, sizeof(*addr)) < 0) {
		err = errno;
		LOG_ERR("Could not bind to receive socket (%s), err: %i",
			proto, err);

		close(sock);

		return -err;
	}

	return sock;
}

static void create_thread(struct config *conf, const char *name)
{
	k_thread_create(&(conf->thread), conf->stack,
			K_KERNEL_STACK_SIZEOF(conf->stack),
			smp_udp_receive_thread, conf, NULL, NULL,
			CONFIG_MCUMGR_TRANSPORT_UDP_THREAD_PRIO, 0, K_FOREVER);

	k_thread_name_set(&(conf->thread), name);
	k_thread_start(&(conf->thread));
}

SYS_INIT(smp_udp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int smp_udp_open(void)
{
	struct config *conf;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	struct sockaddr_in addr4;

	memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
	addr4.sin_addr.s_addr = htonl(INADDR_ANY);

	conf = &configs.ipv4;
	conf->sock = create_socket((struct sockaddr *)&addr4, conf->proto);

	if (conf->sock < 0) {
		return -MGMT_ERR_EUNKNOWN;
	}

	create_thread(conf, "smp_udp4");
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	struct sockaddr_in6 addr6;

	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
	addr6.sin6_addr = in6addr_any;

	conf = &configs.ipv6;
	conf->sock = create_socket((struct sockaddr *)&addr6, conf->proto);

	if (conf->sock < 0) {
		return -MGMT_ERR_EUNKNOWN;
	}

	create_thread(conf, "smp_udp6");
#endif

	return MGMT_ERR_EOK;
}

int smp_udp_close(void)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (configs.ipv4.sock >= 0) {
		k_thread_abort(&(configs.ipv4.thread));
		close(configs.ipv4.sock);
		configs.ipv4.sock = -1;
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (configs.ipv6.sock >= 0) {
		k_thread_abort(&(configs.ipv6.thread));
		close(configs.ipv6.sock);
		configs.ipv6.sock = -1;
	}
#endif

	return MGMT_ERR_EOK;
}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT
static void smp_udp_net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				      struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		if (smp_udp_open() < 0) {
			LOG_ERR("Could not open SMP UDP");
		}
	} else if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");
		smp_udp_close();
	}
}

static void smp_udp_start(void)
{
	net_mgmt_init_event_callback(&smp_udp_mgmt_cb, smp_udp_net_event_handler,
				     (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED));
	net_mgmt_add_event_callback(&smp_udp_mgmt_cb);
	net_conn_mgr_resend_status();
}

MCUMGR_HANDLER_DEFINE(smp_udp, smp_udp_start);
#endif
