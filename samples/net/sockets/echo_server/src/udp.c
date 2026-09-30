/* udp.c - UDP specific code for echo server */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/zvfs/eventfd.h>

#include "common.h"
#include "certificate.h"

#define DTLS_POLL_MS 1000

static APP_BMEM bool udp_stopping;

static void process_udp4(void);
static void process_udp6(void);

K_THREAD_DEFINE(udp4_thread_id, STACK_SIZE,
		process_udp4, NULL, NULL, NULL,
		THREAD_PRIORITY,
		IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

K_THREAD_DEFINE(udp6_thread_id, STACK_SIZE,
		process_udp6, NULL, NULL, NULL,
		THREAD_PRIORITY,
		IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

static int start_udp_proto(struct data *data, struct sockaddr *bind_addr,
			   socklen_t bind_addrlen)
{
	int optval;
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	data->udp.sock = socket(bind_addr->sa_family, SOCK_DGRAM,
				IPPROTO_DTLS_1_2);
#else
	data->udp.sock = socket(bind_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
#endif
	if (data->udp.sock < 0) {
		LOG_ERR("Failed to create UDP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		SERVER_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
	};
	int role = TLS_DTLS_ROLE_SERVER;

	ret = setsockopt(data->udp.sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set UDP secure option (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	/* Set role to DTLS server. */
	ret = setsockopt(data->udp.sock, SOL_TLS, TLS_DTLS_ROLE,
			 &role, sizeof(role));
	if (ret < 0) {
		LOG_ERR("Failed to set DTLS role secure option (%s): %d",
			data->proto, errno);
		ret = -errno;
	}
#endif

	if (bind_addr->sa_family == AF_INET6) {
		/* Prefer IPv6 temporary addresses */
		optval = IPV6_PREFER_SRC_PUBLIC;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6,
				 IPV6_ADDR_PREFERENCES,
				 &optval, sizeof(optval));

		/*
		 * Bind only to IPv6 without mapping to IPv4, since we bind to
		 * IPv4 using another socket
		 */
		optval = 1;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6, IPV6_V6ONLY,
				 &optval, sizeof(optval));
	}

	ret = bind(data->udp.sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to bind UDP socket (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

static int process_udp(struct data *data)
{
	int ret = 0;
	int received;
	struct sockaddr client_addr;
	socklen_t client_addr_len;

	LOG_INF("Waiting for UDP packets on port %d (%s)...",
		 MY_PORT, data->proto);

	do {
		int flags = 0;

		if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
			struct pollfd pfd[] = {
				{
					.fd = data->udp.sock,
					.events = POLLIN,
				},
				{
					.fd = data->udp.stop_fd,
					.events = POLLIN,
				},
			};

			if (poll(pfd, ARRAY_SIZE(pfd), DTLS_POLL_MS) < 0) {
				LOG_ERR("UDP (%s): poll error %d", data->proto, errno);
				(void)k_msleep(DTLS_POLL_MS);
			}

			if (udp_stopping) {
				ret = -ECANCELED;
				break;
			}

			flags = MSG_DONTWAIT;
		}

		client_addr_len = sizeof(client_addr);
		received = recvfrom(data->udp.sock, data->udp.recv_buffer,
				    sizeof(data->udp.recv_buffer), flags,
				    &client_addr, &client_addr_len);

		if (received < 0) {
			if (udp_stopping) {
				ret = -ECANCELED;
				break;
			}

			if ((flags != 0) && (errno == EAGAIN)) {
				continue;
			}

			/* Socket error */
			LOG_ERR("UDP (%s): Connection error %d", data->proto,
				errno);
			ret = -errno;
			break;
		} else if (received) {
			atomic_add(&data->udp.bytes_received, received);
		}

		ret = sendto(data->udp.sock, data->udp.recv_buffer, received, flags,
			     &client_addr, client_addr_len);
		if (ret < 0) {
			if (udp_stopping) {
				ret = -ECANCELED;
				break;
			}

			if ((flags != 0) && (errno == EAGAIN)) {
				continue;
			}

			LOG_ERR("UDP (%s): Failed to send %d", data->proto,
				errno);
			ret = -errno;
			break;
		}

		if (++data->udp.counter % 1000 == 0U) {
			LOG_INF("%s UDP: Sent %u packets", data->proto,
				 data->udp.counter);
		}

		LOG_DBG("UDP (%s): Received and replied with %d bytes",
			data->proto, received);
	} while (true);

	return ret;
}

static void process_udp4(void)
{
	int ret;
	struct sockaddr_in addr4;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(MY_PORT);

	ret = start_udp_proto(&conf.ipv4, (struct sockaddr *)&addr4,
			      sizeof(addr4));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = process_udp(&conf.ipv4);
		if (ret < 0) {
			quit();
		}
	}
}

static void process_udp6(void)
{
	int ret;
	struct sockaddr_in6 addr6;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(MY_PORT);

	ret = start_udp_proto(&conf.ipv6, (struct sockaddr *)&addr6,
			      sizeof(addr6));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = process_udp(&conf.ipv6);
		if (ret < 0) {
			quit();
		}
	}
}

static void print_stats(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct data *data = CONTAINER_OF(dwork, struct data, udp.stats_print);
	int total_received = atomic_get(&data->udp.bytes_received);

	if (total_received) {
		if ((total_received / STATS_TIMER) < 1024) {
			LOG_INF("%s UDP: Received %d B/sec", data->proto,
				total_received / STATS_TIMER);
		} else {
			LOG_INF("%s UDP: Received %d KiB/sec", data->proto,
				total_received / 1024 / STATS_TIMER);
		}

		atomic_set(&data->udp.bytes_received, 0);
	}

	k_work_reschedule(&data->udp.stats_print, K_SECONDS(STATS_TIMER));
}

static void open_stop_fd(struct data *cfg)
{
	cfg->udp.stop_fd = -1;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && IS_ENABLED(CONFIG_ZVFS_EVENTFD)) {
		cfg->udp.stop_fd = zvfs_eventfd(0, 0);
		if (cfg->udp.stop_fd < 0) {
			LOG_ERR("UDP (%s): eventfd failed: %d", cfg->proto, errno);
		}
	}
}

void start_udp(void)
{
	udp_stopping = false;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
#if defined(CONFIG_USERSPACE)
		k_mem_domain_add_thread(&app_domain, udp6_thread_id);
#endif

		k_work_init_delayable(&conf.ipv6.udp.stats_print, print_stats);
		k_thread_name_set(udp6_thread_id, "udp6");
		open_stop_fd(&conf.ipv6);
		k_thread_start(udp6_thread_id);
		k_work_reschedule(&conf.ipv6.udp.stats_print,
				  K_SECONDS(STATS_TIMER));
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
#if defined(CONFIG_USERSPACE)
		k_mem_domain_add_thread(&app_domain, udp4_thread_id);
#endif

		k_work_init_delayable(&conf.ipv4.udp.stats_print, print_stats);
		k_thread_name_set(udp4_thread_id, "udp4");
		open_stop_fd(&conf.ipv4);
		k_thread_start(udp4_thread_id);
		k_work_reschedule(&conf.ipv4.udp.stats_print,
				  K_SECONDS(STATS_TIMER));
	}
}

static void stop_thread(struct k_thread *thread)
{
	if (k_thread_join(thread, K_MSEC(2 * DTLS_POLL_MS)) != 0) {
		/* Not very graceful way to close a thread, but as we may be blocked
		 * in recvfrom call it seems to be necessary
		 */
		k_thread_abort(thread);
	}
}

static void stop_proto(struct data *cfg, struct k_thread *thread)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && (cfg->udp.sock >= 0)) {
		(void)close(cfg->udp.sock);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) &&
	    IS_ENABLED(CONFIG_ZVFS_EVENTFD) && (cfg->udp.stop_fd >= 0)) {
		(void)zvfs_eventfd_write(cfg->udp.stop_fd, 1);
	}

	stop_thread(thread);

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && (cfg->udp.sock >= 0)) {
		(void)close(cfg->udp.sock);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) &&
	    IS_ENABLED(CONFIG_ZVFS_EVENTFD) && (cfg->udp.stop_fd >= 0)) {
		(void)close(cfg->udp.stop_fd);
		cfg->udp.stop_fd = -1;
	}
}

void stop_udp(void)
{
	udp_stopping = true;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		stop_proto(&conf.ipv6, udp6_thread_id);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		stop_proto(&conf.ipv4, udp4_thread_id);
	}
}
