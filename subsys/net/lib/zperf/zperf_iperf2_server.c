/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* iperf2 server: the UDP and the TCP receiver */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include <zephyr/kernel.h>

#include <zephyr/net/igmp.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "ipv6.h"

#if defined(CONFIG_NET_UDP)

static struct net_sockaddr_in6 *in6_addr_my;
static struct net_sockaddr_in *in4_addr_my;

#define UDP_SOCK_ID_IPV4 0
#define UDP_SOCK_ID_IPV6 1
#define UDP_SOCK_ID_MAX  2

#define UDP_RECEIVER_BUF_SIZE   1500
#define POLL_TIMEOUT_MS         100
#define UDP_REORDER_WINDOW_SIZE 64U

static zperf_callback udp_session_cb;
static void *udp_user_data;
static bool udp_server_running;
static uint16_t udp_server_port;
static struct net_sockaddr_storage udp_server_addr;

struct zsock_pollfd udp_fds[UDP_SOCK_ID_MAX] = { 0 };

static void udp_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc_udp, udp_svc_handler, UDP_SOCK_ID_MAX);
static char udp_server_iface_name[NET_IFNAMSIZ];

static void udp_update_sequence(struct session *ses, uint32_t packet_id)
{
	uint32_t advance;

	if (packet_id < ses->next_id) {
		uint32_t distance = ses->next_id - packet_id - 1U;

		ses->outorder++;
		if (distance < UDP_REORDER_WINDOW_SIZE &&
		    (ses->missing_id_bitmap & BIT64(distance)) != 0U) {
			ses->missing_id_bitmap &= ~BIT64(distance);
			if (ses->error > 0U) {
				ses->error--;
			}
		}

		return;
	}

	advance = packet_id - ses->next_id;
	ses->error += advance;

	if (advance >= UDP_REORDER_WINDOW_SIZE - 1U) {
		/* Track the most recent missing IDs and account for older ones as lost. */
		ses->missing_id_bitmap = ~BIT64(0);
	} else {
		ses->missing_id_bitmap <<= advance + 1U;
		ses->missing_id_bitmap |= BIT64_MASK(advance) << 1U;
	}

	ses->next_id = packet_id + 1U;
}

static inline void build_reply(struct zperf_udp_datagram *hdr, struct zperf_server_hdr *stat,
			       uint8_t *buf)
{
	int pos = 0;
	struct zperf_server_hdr *stat_hdr;

	memcpy(&buf[pos], hdr, sizeof(struct zperf_udp_datagram));
	pos += sizeof(struct zperf_udp_datagram);

	stat_hdr = (struct zperf_server_hdr *)&buf[pos];

	stat_hdr->flags = net_htonl(stat->flags);
	stat_hdr->total_len1 = net_htonl(stat->total_len1);
	stat_hdr->total_len2 = net_htonl(stat->total_len2);
	stat_hdr->stop_sec = net_htonl(stat->stop_sec);
	stat_hdr->stop_usec = net_htonl(stat->stop_usec);
	stat_hdr->error_cnt = net_htonl(stat->error_cnt);
	stat_hdr->outorder_cnt = net_htonl(stat->outorder_cnt);
	stat_hdr->datagrams = net_htonl(stat->datagrams);
	stat_hdr->jitter1 = net_htonl(stat->jitter1);
	stat_hdr->jitter2 = net_htonl(stat->jitter2);
}

/* Send statistics to the remote client */
#define BUF_SIZE sizeof(struct zperf_udp_datagram) +	\
	sizeof(struct zperf_server_hdr)

static int zperf_receiver_send_stat(int sock, const struct net_sockaddr *addr,
				    struct zperf_udp_datagram *hdr,
				    struct zperf_server_hdr *stat)
{
	uint8_t reply[BUF_SIZE];
	int ret;

	build_reply(hdr, stat, reply);

	ret = zsock_sendto(sock, reply, sizeof(reply), 0, addr,
			   addr->sa_family == NET_AF_INET6 ?
			   sizeof(struct net_sockaddr_in6) :
			   sizeof(struct net_sockaddr_in));
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", errno);
	}

	return ret;
}

static void udp_received(int sock, const struct net_sockaddr *addr, uint8_t *data,
			 size_t datalen)
{
	struct zperf_udp_datagram *hdr;
	struct session *ses;
	int32_t transit_time;
	int64_t now;
	int32_t id;
	uint32_t packet_id;
	bool is_final;

	if (datalen < sizeof(struct zperf_udp_datagram)) {
		NET_WARN("Short iperf packet!");
		return;
	}

	hdr = (struct zperf_udp_datagram *)data;
	now = k_uptime_ticks();

	ses = get_session(addr, SESSION_UDP);
	if (!ses) {
		NET_ERR("Cannot get a session!");
		return;
	}

	id = net_ntohl(hdr->id);
	is_final = id < 0;
	packet_id = is_final ? (uint32_t)(-(int64_t)id) : (uint32_t)id;

	switch (ses->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		if (is_final) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (zperf_receiver_send_stat(sock, addr, hdr,
						     &ses->stat) < 0) {
				NET_ERR("Failed to send the packet");
			}
			break;
		}

		/* Start a new session */
		zperf_reset_session_stats(ses);
		ses->state = STATE_ONGOING;
		ses->start_time = now;

		if (udp_session_cb != NULL) {
			udp_session_cb(ZPERF_SESSION_STARTED, NULL,
				       udp_user_data);
		}

		/* This is already the first packet of the new session */
		__fallthrough;
	case STATE_ONGOING:
		/* Update counter */
		ses->counter++;
		ses->length += datalen;

		/* Compute jitter */
		transit_time = time_delta(
			k_ticks_to_us_ceil32(now),
			net_ntohl(hdr->tv_sec) * USEC_PER_SEC +
			net_ntohl(hdr->tv_usec));
		if (ses->last_transit_time != 0) {
			int32_t delta_transit = transit_time -
				ses->last_transit_time;

			delta_transit =
				(delta_transit < 0) ?
				-delta_transit : delta_transit;

			ses->jitter +=
				(delta_transit - ses->jitter) / 16;
		}

		ses->last_transit_time = transit_time;

		/* Check header id */
		udp_update_sequence(ses, packet_id);

		if (is_final) { /* Negative id means session end. */
			struct zperf_results results = {0};
			uint64_t duration;

			duration = k_ticks_to_us_ceil64(now - ses->start_time);

			/* Update state machine */
			ses->state = STATE_COMPLETED;

			/* Fill statistics */
			ses->stat.flags = 0x80000000;
			ses->stat.total_len1 = ses->length >> 32;
			ses->stat.total_len2 =
				ses->length % 0xFFFFFFFF;
			ses->stat.stop_sec = duration / USEC_PER_SEC;
			ses->stat.stop_usec = duration % USEC_PER_SEC;
			ses->stat.error_cnt = ses->error;
			ses->stat.outorder_cnt = ses->outorder;
			/* iPerf encodes the total sequence count, including loss. */
			ses->stat.datagrams = packet_id;
			ses->stat.jitter1 = 0;
			ses->stat.jitter2 = ses->jitter;

			if (zperf_receiver_send_stat(sock, addr, hdr,
						     &ses->stat) < 0) {
				NET_ERR("Failed to send the packet");
			}

			results.nb_packets_rcvd = ses->counter;
			results.nb_packets_lost = ses->error;
			results.nb_packets_outorder = ses->outorder;
			results.total_len = ses->length;
			results.time_in_us = duration;
			results.jitter_in_us = ses->jitter;
			results.packet_size = ses->length / ses->counter;

			if (udp_session_cb != NULL) {
				udp_session_cb(ZPERF_SESSION_FINISHED, &results,
					       udp_user_data);
			}
		}
		break;
	default:
		break;
	}
}

static void zperf_udp_join_mcast_ipv4(char *if_name, struct net_in_addr *addr)
{
	struct net_if *iface = NULL;

	if (if_name[0]) {
		iface = net_if_get_by_index(net_if_get_by_name(if_name));
		if (iface == NULL) {
			iface = net_if_get_default();
		}
	} else {
		iface = net_if_get_default();
	}

	if (iface != NULL) {
		net_ipv4_igmp_join(iface, addr, NULL);
	}
}

static void zperf_udp_join_mcast_ipv6(char *if_name, struct net_in6_addr *addr)
{
	struct net_if *iface = NULL;

	if (if_name[0]) {
		iface = net_if_get_by_index(net_if_get_by_name(if_name));
		if (iface == NULL) {
			iface = net_if_get_default();
		}
	} else {
		iface = net_if_get_default();
	}

	if (iface != NULL) {
		net_ipv6_mld_join(iface, addr);
	}
}

static void zperf_udp_leave_mcast(int sock)
{
	struct net_if *iface = NULL;
	struct net_sockaddr_storage addr = {0};
	struct net_sockaddr *sa = net_sad(&addr);
	net_socklen_t addr_len = sizeof(addr);

	if (zsock_getsockname(sock, sa, &addr_len) < 0) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr.ss_family == NET_AF_INET) {
		struct net_sockaddr_in *addr4 = net_sin(sa);

		if (net_ipv4_is_addr_mcast(&addr4->sin_addr)) {
			net_ipv4_igmp_leave(iface, &addr4->sin_addr);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.ss_family == NET_AF_INET6) {
		struct net_sockaddr_in6 *addr6 = net_sin6(sa);

		if (net_ipv6_is_addr_mcast(&addr6->sin6_addr)) {
			net_ipv6_mld_leave(iface, &addr6->sin6_addr);
		}
	}
}

static void udp_receiver_cleanup(void)
{
	int i;

	(void)net_socket_service_unregister(&svc_udp);

	for (i = 0; i < ARRAY_SIZE(udp_fds); i++) {
		if (udp_fds[i].fd >= 0) {
			zperf_udp_leave_mcast(udp_fds[i].fd);
			zsock_close(udp_fds[i].fd);
			udp_fds[i].fd = -1;
		}
	}

	udp_server_running = false;
	udp_session_cb = NULL;

	zperf_session_reset(SESSION_UDP);
}

static int udp_recv_data(struct net_socket_service_event *pev)
{
	static uint8_t buf[UDP_RECEIVER_BUF_SIZE];
	int ret = 1;
	int family, sock_error = 0;
	int default_error = EIO;
	struct net_sockaddr_storage addr = { 0 };
	struct net_sockaddr *sa = net_sad(&addr);
	net_socklen_t optlen = sizeof(int);
	net_socklen_t addrlen = sizeof(addr);

	if (!udp_server_running) {
		return -ENOENT;
	}

	if ((pev->event.revents & ZSOCK_POLLERR) ||
	    (pev->event.revents & ZSOCK_POLLNVAL)) {
		if (pev->event.revents & ZSOCK_POLLNVAL) {
			default_error = EBADF;
		}

		(void)zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
				       ZSOCK_SO_DOMAIN, &family, &optlen);
		if (zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
				     ZSOCK_SO_ERROR, &sock_error, &optlen) < 0 ||
		    sock_error == 0) {
			sock_error = default_error;
		}

		NET_ERR("UDP receiver IPv%d socket error (%d)",
			family == NET_AF_INET ? 4 : 6, sock_error);
		ret = -sock_error;
		goto error;
	}

	if (!(pev->event.revents & ZSOCK_POLLIN)) {
		return 0;
	}

	while (ret > 0) {
		ret = zsock_recvfrom(pev->event.fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT,
				     sa, &addrlen);
		if ((ret < 0) && (errno == EAGAIN)) {
			ret = 0;
			break;
		}

		if (ret < 0) {
			ret = -errno;
			(void)zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
					       ZSOCK_SO_DOMAIN, &family, &optlen);
			NET_ERR("recv failed on IPv%d socket (%d)",
				family == NET_AF_INET ? 4 : 6, -ret);
			goto error;
		}

		udp_received(pev->event.fd, sa, buf, ret);
	}
	return ret;

error:
	if (udp_session_cb != NULL) {
		udp_session_cb(ZPERF_SESSION_ERROR, NULL, udp_user_data);
	}

	return ret;
}

static void udp_svc_handler(struct net_socket_service_event *pev)
{
	int ret;

	ret = udp_recv_data(pev);
	if (ret < 0) {
		udp_receiver_cleanup();
	}
}

static int zperf_udp_receiver_init(void)
{
	struct net_sockaddr *server_sa = net_sad(&udp_server_addr);
	int ret;
	int family;

	for (int i = 0; i < ARRAY_SIZE(udp_fds); i++) {
		udp_fds[i].fd = -1;
	}

	family = udp_server_addr.ss_family;

	if (IS_ENABLED(CONFIG_NET_IPV4) && (family == NET_AF_INET || family == NET_AF_UNSPEC)) {
		const struct net_in_addr *in4_addr = NULL;

		in4_addr_my = zperf_get_sin();

		udp_fds[UDP_SOCK_ID_IPV4].fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM,
						    NET_IPPROTO_UDP);
		if (udp_fds[UDP_SOCK_ID_IPV4].fd < 0) {
			ret = -errno;
			NET_ERR("Cannot create IPv4 network socket.");
			goto error;
		}

		in4_addr = &net_sin(server_sa)->sin_addr;

		if (!net_ipv4_is_addr_unspecified(in4_addr)) {
			memcpy(&in4_addr_my->sin_addr, in4_addr,
				sizeof(struct net_in_addr));
		} else if (strlen(MY_IP4ADDR ? MY_IP4ADDR : "")) {
			/* Use setting IP */
			ret = zperf_get_ipv4_addr(MY_IP4ADDR,
						  &in4_addr_my->sin_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv4");
				goto use_any_ipv4;
			}
		} else {
use_any_ipv4:
			in4_addr_my->sin_addr.s_addr = NET_INADDR_ANY;
		}

		if (net_ipv4_is_addr_mcast(&in4_addr_my->sin_addr)) {
			zperf_udp_join_mcast_ipv4(udp_server_iface_name,
						  &in4_addr_my->sin_addr);
		}

		NET_INFO("Binding to %s",
			 net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

		in4_addr_my->sin_port = net_htons(udp_server_port);

		ret = zsock_bind(udp_fds[UDP_SOCK_ID_IPV4].fd,
				 (struct net_sockaddr *)in4_addr_my,
				 sizeof(struct net_sockaddr_in));
		if (ret < 0) {
			NET_ERR("Cannot bind IPv4 UDP port %d (%d)",
				net_ntohs(in4_addr_my->sin_port),
				errno);
			goto error;
		}

		udp_fds[UDP_SOCK_ID_IPV4].events = ZSOCK_POLLIN;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && (family == NET_AF_INET6 || family == NET_AF_UNSPEC)) {
		const struct net_in6_addr *in6_addr = NULL;

		in6_addr_my = zperf_get_sin6();

		udp_fds[UDP_SOCK_ID_IPV6].fd = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM,
						    NET_IPPROTO_UDP);
		if (udp_fds[UDP_SOCK_ID_IPV6].fd < 0) {
			ret = -errno;
			NET_ERR("Cannot create IPv4 network socket.");
			goto error;
		}

		in6_addr = &net_sin6(server_sa)->sin6_addr;

		if (!net_ipv6_is_addr_unspecified(in6_addr)) {
			memcpy(&in6_addr_my->sin6_addr, in6_addr,
				sizeof(struct net_in6_addr));
		} else if (strlen(MY_IP6ADDR ? MY_IP6ADDR : "")) {
			/* Use setting IP */
			ret = zperf_get_ipv6_addr(MY_IP6ADDR,
						  MY_PREFIX_LEN_STR,
						  &in6_addr_my->sin6_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv6");
				goto use_any_ipv6;
			}
		} else {
use_any_ipv6:
			memcpy(&in6_addr_my->sin6_addr,
			       net_ipv6_unspecified_address(),
			       sizeof(struct net_in6_addr));
		}

		if (net_ipv6_is_addr_mcast(&in6_addr_my->sin6_addr)) {
			zperf_udp_join_mcast_ipv6(udp_server_iface_name,
						  &in6_addr_my->sin6_addr);
		}

		NET_INFO("Binding to %s",
			 net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

		in6_addr_my->sin6_port = net_htons(udp_server_port);

		ret = zsock_bind(udp_fds[UDP_SOCK_ID_IPV6].fd,
				 (struct net_sockaddr *)in6_addr_my,
				 sizeof(struct net_sockaddr_in6));
		if (ret < 0) {
			NET_ERR("Cannot bind IPv6 UDP port %d (%d)",
				net_ntohs(in6_addr_my->sin6_port),
				ret);
			goto error;
		}

		udp_fds[UDP_SOCK_ID_IPV6].events = ZSOCK_POLLIN;
	}

	NET_INFO("Listening on port %d", udp_server_port);

	ret = net_socket_service_register(&svc_udp, udp_fds,
					  ARRAY_SIZE(udp_fds), NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
	}

error:

	return ret;
}

int zperf_udp_download(const struct zperf_download_params *param,
		       zperf_callback callback, void *user_data)
{
	int ret;

	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (udp_server_running) {
		return -EALREADY;
	}

	udp_session_cb = callback;
	udp_user_data  = user_data;
	udp_server_port = param->port;
	memcpy(&udp_server_addr, &param->addr_storage, sizeof(udp_server_addr));

	if (param->if_name[0]) {
		/*
		 * NET_IFNAMSIZ by default CONFIG_NET_INTERFACE_NAME_LEN
		 * is at least 1 so no overflow risk here
		 */
		(void)memset(udp_server_iface_name, 0, NET_IFNAMSIZ);
		strncpy(udp_server_iface_name, param->if_name, NET_IFNAMSIZ);
		udp_server_iface_name[NET_IFNAMSIZ - 1] = 0;
	} else {
		udp_server_iface_name[0] = 0;
	}

	ret = zperf_udp_receiver_init();
	if (ret < 0) {
		udp_receiver_cleanup();
		return ret;
	}

	udp_server_running = true;

	return 0;
}

int zperf_udp_download_stop(void)
{
	if (!udp_server_running) {
		return -EALREADY;
	}

	udp_receiver_cleanup();

	return 0;
}

#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)

#define TCP_SOCK_ID_IPV4_LISTEN 0
#define TCP_SOCK_ID_IPV6_LISTEN 1
#define TCP_SOCK_ID_MAX         (CONFIG_NET_ZPERF_MAX_SESSIONS + 2)

static zperf_callback tcp_session_cb;
static void *tcp_user_data;
static bool tcp_server_running;
static uint16_t tcp_server_port;
static struct net_sockaddr_storage tcp_server_addr;

static struct zsock_pollfd tcp_fds[TCP_SOCK_ID_MAX];
static struct net_sockaddr_storage tcp_sock_addr[TCP_SOCK_ID_MAX];

static void tcp_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc_tcp, tcp_svc_handler,
				      TCP_SOCK_ID_MAX);

static void tcp_received(const struct net_sockaddr *addr, size_t datalen)
{
	struct session *ses;
	int64_t now;

	now = k_uptime_ticks();

	ses = get_session(addr, SESSION_TCP);
	if (!ses) {
		NET_ERR("Cannot get a session!");
		return;
	}

	switch (ses->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		zperf_reset_session_stats(ses);
		ses->start_time = k_uptime_ticks();
		ses->state = STATE_ONGOING;

		if (tcp_session_cb != NULL) {
			tcp_session_cb(ZPERF_SESSION_STARTED, NULL,
				       tcp_user_data);
		}

		__fallthrough;
	case STATE_ONGOING:
		ses->counter++;
		ses->length += datalen;

		if (datalen == 0) { /* EOF */
			struct zperf_results results = { 0 };

			ses->state = STATE_COMPLETED;

			results.total_len = ses->length;
			results.time_in_us = k_ticks_to_us_ceil64(
						now - ses->start_time);

			if (tcp_session_cb != NULL) {
				tcp_session_cb(ZPERF_SESSION_FINISHED, &results,
					       tcp_user_data);
			}
		}
		break;
	default:
		NET_ERR("Unsupported case");
	}
}

static void tcp_session_error_report(void)
{
	if (tcp_session_cb != NULL) {
		tcp_session_cb(ZPERF_SESSION_ERROR, NULL, tcp_user_data);
	}
}

static void tcp_receiver_cleanup(void)
{
	int i;

	(void)net_socket_service_unregister(&svc_tcp);

	for (i = 0; i < ARRAY_SIZE(tcp_fds); i++) {
		if (tcp_fds[i].fd >= 0) {
			zsock_close(tcp_fds[i].fd);
			tcp_fds[i].fd = -1;
			memset(&tcp_sock_addr[i], 0, sizeof(tcp_sock_addr[i]));
		}
	}

	tcp_server_running = false;
	tcp_session_cb = NULL;

	zperf_session_reset(SESSION_TCP);
}

static int tcp_recv_data(struct net_socket_service_event *pev)
{
	static uint8_t buf[CONFIG_NET_ZPERF_TCP_RECEIVER_BUF_SIZE];
	int i, ret = 0;
	int family, sock, sock_error = 0;
	int default_error = EIO;
	struct net_sockaddr_storage addr_incoming_conn = { 0 };
	net_socklen_t optlen = sizeof(int);
	net_socklen_t addrlen = sizeof(addr_incoming_conn);

	if (!tcp_server_running) {
		return -ENOENT;
	}

	if ((pev->event.revents & ZSOCK_POLLERR) ||
	    (pev->event.revents & ZSOCK_POLLNVAL)) {
		if (pev->event.revents & ZSOCK_POLLNVAL) {
			default_error = EBADF;
		}

		(void)zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
				       ZSOCK_SO_DOMAIN, &family, &optlen);
		if (zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
				     ZSOCK_SO_ERROR, &sock_error, &optlen) < 0 ||
		    sock_error == 0) {
			sock_error = default_error;
		}

		NET_ERR("TCP receiver IPv%d socket error (%d)",
			family == NET_AF_INET ? 4 : 6, sock_error);
		ret = -sock_error;
		goto error;
	}

	/* POLLHUP means the peer closed the connection (TCP FIN). Treat it as
	 * EOF on the accepted data socket so that zperf can finalize the
	 * session and report results normally. POLLHUP on a listen socket is
	 * unexpected and should be ignored here (it will be caught by POLLERR).
	 * Only handle POLLHUP when POLLIN is not set to avoid duplicate
	 * processing - when both are set, recv() will return 0 (EOF) naturally.
	 */
	if ((pev->event.revents & ZSOCK_POLLHUP) &&
	    !(pev->event.revents & ZSOCK_POLLIN)) {
		if (pev->event.fd != tcp_fds[TCP_SOCK_ID_IPV4_LISTEN].fd &&
		    pev->event.fd != tcp_fds[TCP_SOCK_ID_IPV6_LISTEN].fd) {
			i = TCP_SOCK_ID_IPV6_LISTEN + 1;
			for (; i < TCP_SOCK_ID_MAX; i++) {
				if (tcp_fds[i].fd == pev->event.fd) {
					break;
				}
			}

			if (i < TCP_SOCK_ID_MAX) {
				tcp_received(net_sad(&tcp_sock_addr[i]), 0);
				zsock_close(tcp_fds[i].fd);
				tcp_fds[i].fd = -1;
				memset(&tcp_sock_addr[i], 0, sizeof(tcp_sock_addr[i]));
				(void)net_socket_service_register(&svc_tcp, tcp_fds,
								  ARRAY_SIZE(tcp_fds),
								  NULL);
			}
		}
		return 0;
	}

	if (!(pev->event.revents & ZSOCK_POLLIN)) {
		return 0;
	}

	/* What is the index to first accepted socket */
	i = TCP_SOCK_ID_IPV6_LISTEN + 1;

	/* Check first if we need to accept a connection */
	if (tcp_fds[TCP_SOCK_ID_IPV4_LISTEN].fd == pev->event.fd ||
	    tcp_fds[TCP_SOCK_ID_IPV6_LISTEN].fd == pev->event.fd) {
		sock = zsock_accept(pev->event.fd,
				    net_sad(&addr_incoming_conn),
				    &addrlen);
		if (sock < 0) {
			ret = -errno;
			(void)zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
					       ZSOCK_SO_DOMAIN, &family, &optlen);
			NET_ERR("TCP receiver IPv%d accept error (%d)",
				family == NET_AF_INET ? 4 : 6, ret);
			goto error;
		}

		for (; i < TCP_SOCK_ID_MAX; i++) {
			if (tcp_fds[i].fd < 0) {
				break;
			}
		}

		if (i == TCP_SOCK_ID_MAX) {
			/* Too many connections. */
			NET_ERR("Dropping TCP connection, reached maximum limit.");
			zsock_close(sock);
		} else {
			tcp_fds[i].fd = sock;
			tcp_fds[i].events = ZSOCK_POLLIN;
			memcpy(&tcp_sock_addr[i], &addr_incoming_conn, addrlen);

			(void)net_socket_service_register(&svc_tcp, tcp_fds,
							  ARRAY_SIZE(tcp_fds),
							  NULL);
		}

	} else {
		ret = zsock_recv(pev->event.fd, buf, sizeof(buf), 0);
		if (ret < 0) {
			(void)zsock_getsockopt(pev->event.fd, ZSOCK_SOL_SOCKET,
					       ZSOCK_SO_DOMAIN, &family, &optlen);
			NET_ERR("recv failed on IPv%d socket (%d)",
				family == NET_AF_INET ? 4 : 6,
				errno);
			tcp_session_error_report();
			/* This will close the zperf session */
			ret = 0;
		}

		for (; i < TCP_SOCK_ID_MAX; i++) {
			if (tcp_fds[i].fd == pev->event.fd) {
				break;
			}
		}

		if (i == TCP_SOCK_ID_MAX) {
			NET_ERR("Descriptor %d not found.", pev->event.fd);
		} else {
			tcp_received(net_sad(&tcp_sock_addr[i]), ret);
			if (ret == 0) {
				zsock_close(tcp_fds[i].fd);
				tcp_fds[i].fd = -1;
				memset(&tcp_sock_addr[i], 0, sizeof(tcp_sock_addr[i]));

				(void)net_socket_service_register(&svc_tcp, tcp_fds,
								  ARRAY_SIZE(tcp_fds),
								  NULL);
			}
		}
	}

	return ret;

error:
	tcp_session_error_report();

	return ret;
}

static void tcp_svc_handler(struct net_socket_service_event *pev)
{
	int ret;

	ret = tcp_recv_data(pev);
	if (ret < 0) {
		tcp_receiver_cleanup();
	}
}

static int tcp_bind_listen_connection(struct zsock_pollfd *pollfd,
				      struct net_sockaddr *address)
{
	uint16_t port;
	int ret;

	if (address->sa_family == NET_AF_INET) {
		port = net_ntohs(net_sin(address)->sin_port);
	} else {
		port = net_ntohs(net_sin6(address)->sin6_port);
	}

	ret = zsock_bind(pollfd->fd, address, sizeof(*address));
	if (ret < 0) {
		NET_ERR("Cannot bind IPv%d TCP port %d (%d)",
			address->sa_family == NET_AF_INET ? 4 : 6, port, errno);
		goto out;
	}

	ret = zsock_listen(pollfd->fd, 1);
	if (ret < 0) {
		NET_ERR("Cannot listen IPv%d TCP (%d)",
			address->sa_family == NET_AF_INET ? 4 : 6, errno);
		goto out;
	}

	pollfd->events = ZSOCK_POLLIN;

out:
	return ret;
}

static int zperf_tcp_receiver_init(void)
{
	struct net_sockaddr *server_sa = net_sad(&tcp_server_addr);
	int ret;
	int family;

	for (int i = 0; i < ARRAY_SIZE(tcp_fds); i++) {
		tcp_fds[i].fd = -1;
	}

	family = tcp_server_addr.ss_family;

	if (IS_ENABLED(CONFIG_NET_IPV4) && (family == NET_AF_INET || family == NET_AF_UNSPEC)) {
		struct net_sockaddr_in *in4_addr = zperf_get_sin();
		const struct net_in_addr *addr = NULL;

		tcp_fds[TCP_SOCK_ID_IPV4_LISTEN].fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM,
							   NET_IPPROTO_TCP);
		if (tcp_fds[TCP_SOCK_ID_IPV4_LISTEN].fd < 0) {
			ret = -errno;
			NET_ERR("Cannot create IPv4 network socket.");
			goto error;
		}

		addr = &net_sin(server_sa)->sin_addr;

		if (!net_ipv4_is_addr_unspecified(addr)) {
			memcpy(&in4_addr->sin_addr, addr,
				sizeof(struct net_in_addr));
		} else if (strlen(MY_IP4ADDR ? MY_IP4ADDR : "")) {
			/* Use Setting IP */
			ret = zperf_get_ipv4_addr(MY_IP4ADDR,
						  &in4_addr->sin_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv4");
				goto use_any_ipv4;
			}
		} else {
use_any_ipv4:
			in4_addr->sin_addr.s_addr = NET_INADDR_ANY;
		}

		in4_addr->sin_port = net_htons(tcp_server_port);

		NET_INFO("Binding to %s",
			 net_sprint_ipv4_addr(&in4_addr->sin_addr));

		memcpy(&tcp_sock_addr[TCP_SOCK_ID_IPV4_LISTEN], in4_addr,
		       sizeof(struct net_sockaddr_in));

		ret = tcp_bind_listen_connection(
				&tcp_fds[TCP_SOCK_ID_IPV4_LISTEN],
				net_sad(&tcp_sock_addr[TCP_SOCK_ID_IPV4_LISTEN]));
		if (ret < 0) {
			goto error;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && (family == NET_AF_INET6 || family == NET_AF_UNSPEC)) {
		struct net_sockaddr_in6 *in6_addr = zperf_get_sin6();
		const struct net_in6_addr *addr = NULL;

		tcp_fds[TCP_SOCK_ID_IPV6_LISTEN].fd = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM,
							   NET_IPPROTO_TCP);
		if (tcp_fds[TCP_SOCK_ID_IPV6_LISTEN].fd < 0) {
			ret = -errno;
			NET_ERR("Cannot create IPv6 network socket.");
			goto error;
		}

		addr = &net_sin6(server_sa)->sin6_addr;

		if (!net_ipv6_is_addr_unspecified(addr)) {
			memcpy(&in6_addr->sin6_addr, addr,
			       sizeof(struct net_in6_addr));
		} else if (strlen(MY_IP6ADDR ? MY_IP6ADDR : "")) {
			/* Use Setting IP */
			ret = zperf_get_ipv6_addr(MY_IP6ADDR,
						  MY_PREFIX_LEN_STR,
						  &in6_addr->sin6_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv6");
				goto use_any_ipv6;
			}
		} else {
use_any_ipv6:
			memcpy(&in6_addr->sin6_addr, net_ipv6_unspecified_address(),
			       sizeof(struct net_in6_addr));
		}

		in6_addr->sin6_port = net_htons(tcp_server_port);

		NET_INFO("Binding to %s",
			 net_sprint_ipv6_addr(&in6_addr->sin6_addr));

		memcpy(&tcp_sock_addr[TCP_SOCK_ID_IPV6_LISTEN], in6_addr,
		       sizeof(struct net_sockaddr_in6));

		ret = tcp_bind_listen_connection(
				&tcp_fds[TCP_SOCK_ID_IPV6_LISTEN],
				net_sad(&tcp_sock_addr[TCP_SOCK_ID_IPV6_LISTEN]));
		if (ret < 0) {
			goto error;
		}
	}

	NET_INFO("Listening on port %d", tcp_server_port);

	ret = net_socket_service_register(&svc_tcp, tcp_fds,
					  ARRAY_SIZE(tcp_fds), NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
	}

error:
	return ret;
}

int zperf_tcp_download(const struct zperf_download_params *param,
		       zperf_callback callback, void *user_data)
{
	int ret;

	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (tcp_server_running) {
		return -EALREADY;
	}

	tcp_session_cb = callback;
	tcp_user_data = user_data;
	tcp_server_port = param->port;
	memcpy(&tcp_server_addr, &param->addr_storage, sizeof(tcp_server_addr));

	ret = zperf_tcp_receiver_init();
	if (ret < 0) {
		tcp_receiver_cleanup();
		return ret;
	}

	tcp_server_running = true;

	return 0;
}

int zperf_tcp_download_stop(void)
{
	if (!tcp_server_running) {
		return -EALREADY;
	}

	tcp_receiver_cleanup();

	return 0;
}

#endif /* CONFIG_NET_TCP */
