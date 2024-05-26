/* udp.c - UDP specific code for echo client */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_client_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/random/random.h>

#include "common.h"
#include "ca_certificate.h"

#define RECV_BUF_SIZE 1280
#define UDP_SLEEP K_MSEC(150)
#define UDP_WAIT K_SECONDS(10)

static APP_BMEM char recv_buf[RECV_BUF_SIZE];

static K_THREAD_STACK_DEFINE(udp_tx_thread_stack, UDP_STACK_SIZE);
static struct k_thread udp_tx_thread;

/* Kernel objects should not be placed in a memory area accessible from user
 * threads.
 */
static struct udp_control udp4_ctrl, udp6_ctrl;
static struct k_poll_signal udp_kill;

static int send_udp_data(struct data *data);
static void wait_reply(struct k_timer *timer);
static void wait_transmit(struct k_timer *timer);

static void process_udp_tx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &udp_kill),
#if defined(CONFIG_NET_IPV4)
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &udp4_ctrl.tx_signal),
#endif
#if defined(CONFIG_NET_IPV6)
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &udp6_ctrl.tx_signal),
#endif
	};

	while (true) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);

		for (int i = 0; i < ARRAY_SIZE(events); i++) {
			unsigned int signaled;
			int result;

			k_poll_signal_check(events[i].signal, &signaled, &result);
			if (signaled == 0) {
				continue;
			}

			k_poll_signal_reset(events[i].signal);
			events[i].state = K_POLL_STATE_NOT_READY;

			if (events[i].signal == &udp_kill) {
				return;
			} else if (events[i].signal == &udp4_ctrl.tx_signal) {
				send_udp_data(&conf.ipv4);
			} else if (events[i].signal == &udp6_ctrl.tx_signal) {
				send_udp_data(&conf.ipv6);
			}
		}
	}
}

static void udp_control_init(struct udp_control *ctrl)
{
	k_timer_init(&ctrl->rx_timer, wait_reply, NULL);
	k_timer_init(&ctrl->tx_timer, wait_transmit, NULL);
	k_poll_signal_init(&ctrl->tx_signal);
}

static void udp_control_access_grant(struct udp_control *ctrl)
{
	k_thread_access_grant(k_current_get(),
			      &ctrl->rx_timer,
			      &ctrl->tx_timer,
			      &ctrl->tx_signal);
}

void init_udp(void)
{
	/* k_timer_init() is not a system call, therefore initialize kernel
	 * objects here.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		udp_control_init(&udp4_ctrl);
		conf.ipv4.udp.ctrl = &udp4_ctrl;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		udp_control_init(&udp6_ctrl);
		conf.ipv6.udp.ctrl = &udp6_ctrl;
	}

	k_poll_signal_init(&udp_kill);

	if (IS_ENABLED(CONFIG_USERSPACE)) {
		k_thread_access_grant(k_current_get(),
				      &udp_tx_thread,
				      &udp_tx_thread_stack,
				      &udp_kill);

		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			udp_control_access_grant(&udp4_ctrl);
		}

		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			udp_control_access_grant(&udp6_ctrl);
		}
	}
}

static int send_udp_data(struct data *data)
{
	int ret;

	do {
		data->udp.expecting = sys_rand32_get() % ipsum_len;
	} while (data->udp.expecting == 0U ||
		 data->udp.expecting > data->udp.mtu);

	ret = send(data->udp.sock, lorem_ipsum, data->udp.expecting, 0);

	if (PRINT_PROGRESS) {
		LOG_DBG("%s UDP: Sent %d bytes", data->proto, data->udp.expecting);
	}

	k_timer_start(&data->udp.ctrl->rx_timer, UDP_WAIT, K_NO_WAIT);

	return ret < 0 ? -EIO : 0;
}

static int compare_udp_data(struct data *data, const char *buf, uint32_t received)
{
	if (received != data->udp.expecting) {
		LOG_ERR("Invalid amount of data received: UDP %s", data->proto);
		return -EIO;
	}

	if (memcmp(buf, lorem_ipsum, received) != 0) {
		LOG_ERR("Invalid data received: UDP %s", data->proto);
		return -EIO;
	}

	return 0;
}

static void wait_reply(struct k_timer *timer)
{
	/* This means that we did not receive response in time. */
	struct udp_control *ctrl = CONTAINER_OF(timer, struct udp_control, rx_timer);
	struct data *data = (ctrl == conf.ipv4.udp.ctrl) ? &conf.ipv4 : &conf.ipv6;

	LOG_ERR("UDP %s: Data packet not received", data->proto);

	/* Send a new packet at this point */
	k_poll_signal_raise(&ctrl->tx_signal, 0);
}

static void wait_transmit(struct k_timer *timer)
{
	struct udp_control *ctrl = CONTAINER_OF(timer, struct udp_control, tx_timer);

	k_poll_signal_raise(&ctrl->tx_signal, 0);
}

static int start_udp_proto(struct data *data, struct sockaddr *addr,
			   socklen_t addrlen)
{
	int optval;
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	data->udp.sock = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_DTLS_1_2);
#else
	data->udp.sock = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
#endif
	if (data->udp.sock < 0) {
		LOG_ERR("Failed to create UDP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		CA_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
	};

	ret = setsockopt(data->udp.sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SEC_TAG_LIST option (%s): %d",
			data->proto, errno);
		ret = -errno;
	}

	ret = setsockopt(data->udp.sock, SOL_TLS, TLS_HOSTNAME,
			 TLS_PEER_HOSTNAME, sizeof(TLS_PEER_HOSTNAME));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option (%s): %d",
			data->proto, errno);
		ret = -errno;
	}
#endif

	/* Prefer IPv6 temporary addresses */
	if (addr->sa_family == AF_INET6) {
		optval = IPV6_PREFER_SRC_TMP;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6,
				 IPV6_ADDR_PREFERENCES,
				 &optval, sizeof(optval));
	}

	/* Call connect so we can use send and recv. */
	ret = connect(data->udp.sock, addr, addrlen);
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

static int process_udp_proto(struct data *data)
{
	int ret, received;

	received = recv(data->udp.sock, recv_buf, sizeof(recv_buf),
			MSG_DONTWAIT);

	if (received == 0) {
		return -EIO;
	}
	if (received < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}
		return ret;
	}

	ret = compare_udp_data(data, recv_buf, received);
	if (ret != 0) {
		LOG_WRN("%s UDP: Received and compared %d bytes, data "
			"mismatch", data->proto, received);
		return 0;
	}

	if (PRINT_PROGRESS) {
		/* Correct response received */
		LOG_DBG("%s UDP: Received and compared %d bytes, all ok",
			data->proto, received);
	}

	if (++data->udp.counter % 1000 == 0U) {
		LOG_INF("%s UDP: Exchanged %u packets", data->proto,
			data->udp.counter);
	}

	k_timer_stop(&data->udp.ctrl->rx_timer);

	/* Do not flood the link if we have also TCP configured */
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		k_timer_start(&data->udp.ctrl->tx_timer, UDP_SLEEP, K_NO_WAIT);
	} else {
		k_poll_signal_raise(&data->udp.ctrl->tx_signal, 0);
	}

	return ret;
}

int start_udp(void)
{
	int ret = 0;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(PEER_PORT);
		inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &addr6.sin6_addr);

		ret = start_udp_proto(&conf.ipv6,
				      (struct sockaddr *)&addr6,
				      sizeof(addr6));
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(PEER_PORT);
		inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &addr4.sin_addr);

		ret = start_udp_proto(&conf.ipv4, (struct sockaddr *)&addr4,
				      sizeof(addr4));
		if (ret < 0) {
			return ret;
		}
	}

	k_thread_create(&udp_tx_thread, udp_tx_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_tx_thread_stack),
			process_udp_tx,
			NULL, NULL, NULL, THREAD_PRIORITY,
			IS_ENABLED(CONFIG_USERSPACE) ?
						K_USER | K_INHERIT_PERMS : 0,
			K_NO_WAIT);

	k_thread_name_set(&udp_tx_thread, "udp_tx");

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_poll_signal_raise(&conf.ipv6.udp.ctrl->tx_signal, 0);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_poll_signal_raise(&conf.ipv4.udp.ctrl->tx_signal, 0);
	}

	return ret;
}

int process_udp(void)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = process_udp_proto(&conf.ipv6);
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = process_udp_proto(&conf.ipv4);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

void stop_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_timer_stop(&udp6_ctrl.tx_timer);
		k_timer_stop(&udp6_ctrl.rx_timer);

		if (conf.ipv6.udp.sock >= 0) {
			(void)close(conf.ipv6.udp.sock);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_timer_stop(&udp4_ctrl.tx_timer);
		k_timer_stop(&udp4_ctrl.rx_timer);

		if (conf.ipv4.udp.sock >= 0) {
			(void)close(conf.ipv4.udp.sock);
		}
	}

	k_poll_signal_raise(&udp_kill, 0);
}
