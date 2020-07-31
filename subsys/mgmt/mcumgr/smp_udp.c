/*
 * Copyright (c) 2019, Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the mcumgr SMP protocol.
 */

#include <zephyr.h>
#include <init.h>
#include <net/socket.h>
#include <errno.h>
#include <mgmt/mgmt.h>
#include <mgmt/mcumgr/smp_udp.h>
#include <mgmt/mcumgr/buf.h>
#include <mgmt/mcumgr/smp.h>

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(smp_udp);

struct config {
	int sock;
	const char *proto;
	struct zephyr_smp_transport smp_transport;
	char recv_buffer[CONFIG_MCUMGR_SMP_UDP_MTU];
	struct k_thread thread;
	K_KERNEL_STACK_MEMBER(stack, CONFIG_MCUMGR_SMP_UDP_STACK_SIZE);
};

struct configs {
#if CONFIG_MCUMGR_SMP_UDP_IPV4
	struct config ipv4;
#endif
#if CONFIG_MCUMGR_SMP_UDP_IPV6
	struct config ipv6;
#endif
};

static struct configs configs = {
#if CONFIG_MCUMGR_SMP_UDP_IPV4
	.ipv4 = {
		.proto = "IPv4",
	},
#endif
#if CONFIG_MCUMGR_SMP_UDP_IPV6
	.ipv6 = {
		.proto = "IPv6",
	},
#endif
};

#if CONFIG_MCUMGR_SMP_UDP_IPV4
static int smp_udp4_tx(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	ARG_UNUSED(zst);

	struct sockaddr *addr = net_buf_user_data(nb);
	int ret = sendto(configs.ipv4.sock, nb->data, nb->len,
			 0, addr, sizeof(*addr));
	mcumgr_buf_free(nb);

	return ret < 0 ? MGMT_ERR_EINVAL : MGMT_ERR_EOK;
}
#endif

#if CONFIG_MCUMGR_SMP_UDP_IPV6
static int smp_udp6_tx(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	ARG_UNUSED(zst);

	struct sockaddr *addr = net_buf_user_data(nb);
	int ret = sendto(configs.ipv6.sock, nb->data, nb->len,
			 0, addr, sizeof(*addr));
	mcumgr_buf_free(nb);

	return ret < 0 ? MGMT_ERR_EINVAL : MGMT_ERR_EOK;
}
#endif

static uint16_t smp_udp_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	return CONFIG_MCUMGR_SMP_UDP_MTU;
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
				   CONFIG_MCUMGR_SMP_UDP_MTU,
				   0, &addr, &addr_len);

		if (len > 0) {
			struct sockaddr *ud;
			struct net_buf *nb;

			/* store sender address in user data for reply */
			nb = mcumgr_buf_alloc();
			net_buf_add_mem(nb, conf->recv_buffer, len);
			ud = net_buf_user_data(nb);
			net_ipaddr_copy(ud, &addr);

			zephyr_smp_rx_req(&conf->smp_transport, nb);
		} else if (len < 0) {
			LOG_ERR("recvfrom error (%s): %i", conf->proto, errno);
		}
	}
}

static int smp_udp_init(struct device *dev)
{
	ARG_UNUSED(dev);

#if CONFIG_MCUMGR_SMP_UDP_IPV4
	zephyr_smp_transport_init(&configs.ipv4.smp_transport,
				  smp_udp4_tx, smp_udp_get_mtu,
				  smp_udp_ud_copy, NULL);
#endif

#if CONFIG_MCUMGR_SMP_UDP_IPV6
	zephyr_smp_transport_init(&configs.ipv6.smp_transport,
				  smp_udp6_tx, smp_udp_get_mtu,
				  smp_udp_ud_copy, NULL);
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
			CONFIG_MCUMGR_SMP_UDP_THREAD_PRIO, 0, K_FOREVER);

	k_thread_name_set(&(conf->thread), name);
	k_thread_start(&(conf->thread));
}

SYS_INIT(smp_udp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int smp_udp_open(void)
{
	struct config *conf;

#if CONFIG_MCUMGR_SMP_UDP_IPV4
	struct sockaddr_in addr4;

	memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(CONFIG_MCUMGR_SMP_UDP_PORT);
	inet_pton(AF_INET, INADDR_ANY, &addr4.sin_addr);

	conf = &configs.ipv4;
	conf->sock = create_socket((struct sockaddr *)&addr4, conf->proto);

	if (conf->sock < 0) {
		return -MGMT_ERR_EUNKNOWN;
	}

	create_thread(conf, "smp_udp4");
#endif

#if CONFIG_MCUMGR_SMP_UDP_IPV6
	struct sockaddr_in6 addr6;

	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(CONFIG_MCUMGR_SMP_UDP_PORT);
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
#if CONFIG_MCUMGR_SMP_UDP_IPV4
	k_thread_abort(&(configs.ipv4.thread));
	close(configs.ipv4.sock);
#endif

#if CONFIG_MCUMGR_SMP_UDP_IPV6
	k_thread_abort(&(configs.ipv6.thread));
	close(configs.ipv6.sock);
#endif

	return MGMT_ERR_EOK;
}
