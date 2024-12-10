/*
 * Copyright (c) 2019-2020, Prevas A/S
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the mcumgr SMP protocol.
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_udp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <errno.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_udp);

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_UDP_MTU != 0, "CONFIG_MCUMGR_TRANSPORT_UDP_MTU must be > 0");

#if !defined(CONFIG_MCUMGR_TRANSPORT_UDP_IPV4) && !defined(CONFIG_MCUMGR_TRANSPORT_UDP_IPV6)
BUILD_ASSERT(0, "Either IPv4 or IPv6 SMP must be enabled for the MCUmgr UDP SMP transport using "
		"CONFIG_MCUMGR_TRANSPORT_UDP_IPV4 or CONFIG_MCUMGR_TRANSPORT_UDP_IPV6");
#endif

BUILD_ASSERT(sizeof(struct sockaddr) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE must be >= sizeof(struct sockaddr)");

enum proto_type {
	PROTOCOL_IPV4 = 0,
	PROTOCOL_IPV6,
};

struct config {
	int sock;
	enum proto_type proto;
	struct k_sem network_ready_sem;
	struct smp_transport smp_transport;
	char recv_buffer[CONFIG_MCUMGR_TRANSPORT_UDP_MTU];
	struct k_thread thread;
	K_KERNEL_STACK_MEMBER(stack, CONFIG_MCUMGR_TRANSPORT_UDP_STACK_SIZE);
};

struct configs {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	struct config ipv4;
#ifdef CONFIG_SMP_CLIENT
	struct smp_client_transport_entry ipv4_transport;
#endif
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	struct config ipv6;
#ifdef CONFIG_SMP_CLIENT
	struct smp_client_transport_entry ipv6_transport;
#endif
#endif
};

static bool threads_created;

static struct configs smp_udp_configs;

static struct net_mgmt_event_callback smp_udp_mgmt_cb;

static const char *smp_udp_proto_to_name(enum proto_type proto)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (proto == PROTOCOL_IPV4) {
		return "IPv4";
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (proto == PROTOCOL_IPV6) {
		return "IPv6";
	}
#endif

	return "??";
}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
static int smp_udp4_tx(struct net_buf *nb)
{
	int ret;
	struct sockaddr *addr = net_buf_user_data(nb);

	ret = zsock_sendto(smp_udp_configs.ipv4.sock, nb->data, nb->len, 0, addr, sizeof(*addr));

	if (ret < 0) {
		if (errno == ENOMEM) {
			ret = MGMT_ERR_EMSGSIZE;
		} else {
			ret = MGMT_ERR_EINVAL;
		}
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

	ret = zsock_sendto(smp_udp_configs.ipv6.sock, nb->data, nb->len, 0, addr, sizeof(*addr));

	if (ret < 0) {
		if (errno == ENOMEM) {
			ret = MGMT_ERR_EMSGSIZE;
		} else {
			ret = MGMT_ERR_EINVAL;
		}
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

static int create_socket(enum proto_type proto, int *sock)
{
	int tmp_sock;
	int err;
	struct sockaddr *addr;
	socklen_t addr_len = 0;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	struct sockaddr_in addr4;
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	struct sockaddr_in6 addr6;
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (proto == PROTOCOL_IPV4) {
		addr_len = sizeof(struct sockaddr_in);
		memset(&addr4, 0, sizeof(addr4));
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
		addr4.sin_addr.s_addr = htonl(INADDR_ANY);
		addr = (struct sockaddr *)&addr4;
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (proto == PROTOCOL_IPV6) {
		addr_len = sizeof(struct sockaddr_in6);
		memset(&addr6, 0, sizeof(addr6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
		addr6.sin6_addr = in6addr_any;
		addr = (struct sockaddr *)&addr6;
	}
#endif

	tmp_sock = zsock_socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	err = errno;

	if (tmp_sock < 0) {
		LOG_ERR("Could not open receive socket (%s), err: %i",
			smp_udp_proto_to_name(proto), err);

		return -err;
	}

	if (zsock_bind(tmp_sock, addr, addr_len) < 0) {
		err = errno;
		LOG_ERR("Could not bind to receive socket (%s), err: %i",
			smp_udp_proto_to_name(proto), err);

		zsock_close(tmp_sock);

		return -err;
	}

	*sock = tmp_sock;
	return 0;
}

static void smp_udp_receive_thread(void *p1, void *p2, void *p3)
{
	struct config *conf = (struct config *)p1;
	int rc;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_sem_take(&conf->network_ready_sem, K_FOREVER);
	rc = create_socket(conf->proto, &conf->sock);

	if (rc < 0) {
		return;
	}

	__ASSERT(rc >= 0, "Socket is invalid");
	LOG_INF("Started (%s)", smp_udp_proto_to_name(conf->proto));

	while (1) {
		struct sockaddr addr;
		socklen_t addr_len = sizeof(addr);

		int len = zsock_recvfrom(conf->sock, conf->recv_buffer,
					 CONFIG_MCUMGR_TRANSPORT_UDP_MTU, 0, &addr, &addr_len);

		if (len > 0) {
			struct sockaddr *ud;
			struct net_buf *nb;

			/* Store sender address in user data for reply */
			nb = smp_packet_alloc();
			if (!nb) {
				LOG_ERR("Failed to allocate mcumgr buffer");
				/* No free space, drop SMP frame */
				continue;
			}
			net_buf_add_mem(nb, conf->recv_buffer, len);
			ud = net_buf_user_data(nb);
			net_ipaddr_copy(ud, &addr);

			smp_rx_req(&conf->smp_transport, nb);
		} else if (len < 0) {
			LOG_ERR("recvfrom error (%s): %i, %d", smp_udp_proto_to_name(conf->proto),
				errno, len);
		}
	}
}

static void smp_udp_open_iface(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	if (net_if_is_up(iface)) {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
		if (net_if_flag_is_set(iface, NET_IF_IPV4) &&
		    k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == -EBUSY) {
			k_sem_give(&smp_udp_configs.ipv4.network_ready_sem);
		}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
		if (net_if_flag_is_set(iface, NET_IF_IPV6) &&
		    k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == -EBUSY) {
			k_sem_give(&smp_udp_configs.ipv6.network_ready_sem);
		}
#endif
	}
}

static void smp_udp_net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				      struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IF_UP) {
		smp_udp_open_iface(iface, NULL);
	}
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

int smp_udp_open(void)
{
	bool started = false;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == 0 ||
	    threads_created == false) {
		(void)k_sem_reset(&smp_udp_configs.ipv4.network_ready_sem);
		create_thread(&smp_udp_configs.ipv4, "smp_udp4");
		started = true;
	} else {
		LOG_ERR("IPv4 UDP MCUmgr thread is already running");
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == 0 ||
	    threads_created == false) {
		(void)k_sem_reset(&smp_udp_configs.ipv6.network_ready_sem);
		create_thread(&smp_udp_configs.ipv6, "smp_udp6");
		started = true;
	} else {
		LOG_ERR("IPv6 UDP MCUmgr thread is already running");
	}
#endif

	if (started) {
		/* One or more threads were started, check existing interfaces */
		threads_created = true;
		net_if_foreach(smp_udp_open_iface, NULL);
	}

	return 0;
}

int smp_udp_close(void)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == -EBUSY) {
		k_thread_abort(&(smp_udp_configs.ipv4.thread));

		if (smp_udp_configs.ipv4.sock >= 0) {
			zsock_close(smp_udp_configs.ipv4.sock);
			smp_udp_configs.ipv4.sock = -1;
		}
	} else {
		LOG_ERR("IPv4 UDP MCUmgr thread is not running");
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == -EBUSY) {
		k_thread_abort(&(smp_udp_configs.ipv6.thread));

		if (smp_udp_configs.ipv6.sock >= 0) {
			zsock_close(smp_udp_configs.ipv6.sock);
			smp_udp_configs.ipv6.sock = -1;
		}
	} else {
		LOG_ERR("IPv6 UDP MCUmgr thread is not running");
	}
#endif

	return 0;
}

static void smp_udp_start(void)
{
	int rc;

	threads_created = false;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	smp_udp_configs.ipv4.proto = PROTOCOL_IPV4;
	smp_udp_configs.ipv4.sock = -1;

	k_sem_init(&smp_udp_configs.ipv4.network_ready_sem, 0, 1);
	smp_udp_configs.ipv4.smp_transport.functions.output = smp_udp4_tx;
	smp_udp_configs.ipv4.smp_transport.functions.get_mtu = smp_udp_get_mtu;
	smp_udp_configs.ipv4.smp_transport.functions.ud_copy = smp_udp_ud_copy;

	rc = smp_transport_init(&smp_udp_configs.ipv4.smp_transport);
#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_udp_configs.ipv4_transport.smpt = &smp_udp_configs.ipv4.smp_transport;
		smp_udp_configs.ipv4_transport.smpt_type = SMP_UDP_IPV4_TRANSPORT;
		smp_client_transport_register(&smp_udp_configs.ipv4_transport);
	}
#endif
	if (rc) {
		LOG_ERR("Failed to register IPv4 UDP MCUmgr SMP transport: %d", rc);
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	smp_udp_configs.ipv6.proto = PROTOCOL_IPV6;
	smp_udp_configs.ipv6.sock = -1;

	k_sem_init(&smp_udp_configs.ipv6.network_ready_sem, 0, 1);
	smp_udp_configs.ipv6.smp_transport.functions.output = smp_udp6_tx;
	smp_udp_configs.ipv6.smp_transport.functions.get_mtu = smp_udp_get_mtu;
	smp_udp_configs.ipv6.smp_transport.functions.ud_copy = smp_udp_ud_copy;

	rc = smp_transport_init(&smp_udp_configs.ipv6.smp_transport);
#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_udp_configs.ipv6_transport.smpt = &smp_udp_configs.ipv6.smp_transport;
		smp_udp_configs.ipv6_transport.smpt_type = SMP_UDP_IPV6_TRANSPORT;
		smp_client_transport_register(&smp_udp_configs.ipv6_transport);
	}
#endif

	if (rc) {
		LOG_ERR("Failed to register IPv6 UDP MCUmgr SMP transport: %d", rc);
	}
#endif

	net_mgmt_init_event_callback(&smp_udp_mgmt_cb, smp_udp_net_event_handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&smp_udp_mgmt_cb);

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT
	smp_udp_open();
#endif
}

MCUMGR_HANDLER_DEFINE(smp_udp, smp_udp_start);
