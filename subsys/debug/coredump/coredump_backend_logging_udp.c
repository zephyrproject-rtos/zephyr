/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/debug/coredump.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#if defined(CONFIG_NET_CONNECTION_MANAGER)
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#endif
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "coredump_internal.h"

LOG_MODULE_REGISTER(coredump_udp, CONFIG_DEBUG_COREDUMP_LOG_LEVEL);

#define LOG_BUF_SZ     64
#define LOG_BUF_SZ_RAW (LOG_BUF_SZ + 1)

#define COREDUMP_UDP_MAGIC0 'Z'
#define COREDUMP_UDP_MAGIC1 'C'
#define COREDUMP_UDP_MAGIC2 'D'
#define COREDUMP_UDP_MAGIC3 'U'

#define COREDUMP_UDP_HDR_SIZE    16U
#define COREDUMP_UDP_MAX_PAYLOAD 1024U

#define COREDUMP_UDP_FLAG_END 0x0001U

/* Default destination port when DEBUG_COREDUMP_LOGGING_UDP_HOST omits :port */
#define COREDUMP_LOGGING_UDP_DEFAULT_PORT 17777U

static char log_buf[LOG_BUF_SZ_RAW];
static int error;
static int udp_sock = -1;
/* Socket created at boot (thread ctx); fatal path only sendto()s. */
static int udp_presock_fd = -1;
static struct net_sockaddr_storage udp_peer_storage;
static net_socklen_t udp_peer_socklen;
static net_sa_family_t udp_sock_af;
static uint32_t udp_seq;
static uint32_t stream_offset;

static int udp_write_chunk(const uint8_t *payload, size_t payload_len, uint16_t flags)
{
	uint8_t pkt[COREDUMP_UDP_HDR_SIZE + COREDUMP_UDP_MAX_PAYLOAD];

	if (payload_len > COREDUMP_UDP_MAX_PAYLOAD) {
		return -EINVAL;
	}

	if (udp_sock < 0) {
		return -ENOTCONN;
	}

	pkt[0] = COREDUMP_UDP_MAGIC0;
	pkt[1] = COREDUMP_UDP_MAGIC1;
	pkt[2] = COREDUMP_UDP_MAGIC2;
	pkt[3] = COREDUMP_UDP_MAGIC3;
	sys_put_le32(stream_offset, &pkt[4]);
	sys_put_le16((uint16_t)payload_len, &pkt[8]);
	sys_put_le16(flags, &pkt[10]);
	sys_put_le32(udp_seq, &pkt[12]);

	if (payload_len > 0U && payload != NULL) {
		memcpy(&pkt[COREDUMP_UDP_HDR_SIZE], payload, payload_len);
	}

	int plen = (int)(COREDUMP_UDP_HDR_SIZE + payload_len);

	/*
	 * Blocking send uses k_sleep() while buffers drain; in fatal / coredump
	 * context that is unsafe. Use DONTWAIT plus k_busy_wait() retries (IRQs
	 * must be deliverable: CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS).
	 */
	for (unsigned int attempt = 0U;; attempt++) {
		int sent = zsock_sendto(udp_sock, pkt, plen, ZSOCK_MSG_DONTWAIT,
					net_sad(&udp_peer_storage), udp_peer_socklen);

		if (sent == plen) {
			break;
		}

		if (sent < 0) {
			int err = errno;

			if ((err == EAGAIN || err == ENOBUFS) && attempt < 4095U) {
				/* No k_yield(): fatal path may be treated as ISR context. Busy-wait
				 * with IRQs unmasked (see CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS)
				 * so GEM TX-done interrupts can run.
				 */
				k_busy_wait(50U);
				continue;
			}

			return -err;
		}

		return -EIO;
	}

	udp_seq++;
	stream_offset += payload_len;
	return 0;
}

/*
 * Parse CONFIG_DEBUG_COREDUMP_LOGGING_UDP_HOST into udp_peer_storage via
 * net_ipaddr_parse(); apply COREDUMP_LOGGING_UDP_DEFAULT_PORT when :port omitted.
 */
static int coredump_udp_parse_peer(void)
{
	const char *host = CONFIG_DEBUG_COREDUMP_LOGGING_UDP_HOST;
	struct net_sockaddr *sa = net_sad(&udp_peer_storage);

	memset(&udp_peer_storage, 0, sizeof(udp_peer_storage));

	if (host[0] == '\0') {
		return -EINVAL;
	}

	if (!net_ipaddr_parse(host, strlen(host), sa)) {
		return -EINVAL;
	}

	if (net_port_set_default(sa, COREDUMP_LOGGING_UDP_DEFAULT_PORT) < 0) {
		return -EINVAL;
	}

	udp_sock_af = sa->sa_family;

	if (IS_ENABLED(CONFIG_NET_IPV4) && udp_sock_af == NET_AF_INET) {
		udp_peer_socklen = sizeof(struct net_sockaddr_in);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && udp_sock_af == NET_AF_INET6) {
		udp_peer_socklen = sizeof(struct net_sockaddr_in6);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int coredump_udp_bind_ephemeral(int sock)
{
	int ret;
	struct net_sockaddr_storage bind_storage;
	struct net_sockaddr *bind_sa = net_sad(&bind_storage);
	size_t addr_len;

	memset(&bind_storage, 0, sizeof(bind_storage));
	bind_sa->sa_family = udp_sock_af;

	if (IS_ENABLED(CONFIG_NET_IPV4) && udp_sock_af == NET_AF_INET) {
		net_sin(bind_sa)->sin_addr.s_addr = NET_INADDR_ANY;
		addr_len = sizeof(struct net_sockaddr_in);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && udp_sock_af == NET_AF_INET6) {
		net_sin6(bind_sa)->sin6_addr = net_in6addr_any;
		addr_len = sizeof(struct net_sockaddr_in6);
	} else {
		return -ENOTSUP;
	}

	ret = zsock_bind(sock, bind_sa, addr_len);

	return ret < 0 ? -errno : 0;
}

static int coredump_udp_open_bind_socket(void)
{
	int ret;

	ret = coredump_udp_parse_peer();
	if (ret != 0) {
		return ret;
	}

	udp_sock = zsock_socket(udp_sock_af, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (udp_sock < 0) {
		return -errno;
	}

	ret = coredump_udp_bind_ephemeral(udp_sock);
	if (ret < 0) {
		zsock_close(udp_sock);
		udp_sock = -1;
		return ret;
	}

	return 0;
}

#if defined(CONFIG_NET_CONNECTION_MANAGER)

static struct net_mgmt_event_callback coredump_udp_l4_cb;

static void coredump_udp_l4_handler(struct net_mgmt_event_callback *cb, uint64_t evt,
				    struct net_if *iface)
{
	int r;

	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (evt != NET_EVENT_L4_CONNECTED) {
		return;
	}

	if (udp_presock_fd >= 0) {
		return;
	}

	r = coredump_udp_open_bind_socket();
	if (r != 0) {
		LOG_ERR("coredump UDP: presocket on L4 up failed: %d", r);
		return;
	}

	udp_presock_fd = udp_sock;
	udp_sock = -1;
	LOG_DBG("coredump UDP presocket fd=%d", udp_presock_fd);
}

static int coredump_udp_hook_net_l4(void)
{
	net_mgmt_init_event_callback(&coredump_udp_l4_cb, coredump_udp_l4_handler,
				     NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&coredump_udp_l4_cb);
	conn_mgr_mon_resend_status();
	return 0;
}

SYS_INIT(coredump_udp_hook_net_l4, APPLICATION, 80);

#else

/* Fallback when conn_mgr is off: run after net_config SYS_INIT (prio 95). */
static int coredump_udp_presocket_init(void)
{
	int r;

	r = coredump_udp_open_bind_socket();
	if (r != 0) {
		return 0;
	}

	udp_presock_fd = udp_sock;
	udp_sock = -1;
	LOG_DBG("coredump UDP presocket fd=%d", udp_presock_fd);
	return 0;
}

SYS_INIT(coredump_udp_presocket_init, APPLICATION, 99);

#endif /* CONFIG_NET_CONNECTION_MANAGER */

static void coredump_udp_backend_start(void)
{
	int r;

	error = 0;
	udp_seq = 0U;
	stream_offset = 0U;
	udp_sock = -1;

	while (LOG_PROCESS()) {
		;
	}

	LOG_PANIC();
	if (IS_ENABLED(CONFIG_DEBUG_COREDUMP_LOGGING_UDP_MIRROR_TO_LOG)) {
		LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_BEGIN_STR);
	}

	if (udp_presock_fd >= 0) {
		udp_sock = udp_presock_fd;
		return;
	}

	r = coredump_udp_open_bind_socket();
	if (r != 0) {
		error = r;
		LOG_ERR("coredump UDP: presocket missing, in-fatal open failed: %d", r);
		udp_sock = -1;
	}
}

static void coredump_udp_backend_end(void)
{
	if (udp_sock >= 0) {
		if (udp_write_chunk(NULL, 0U, COREDUMP_UDP_FLAG_END) != 0 && error == 0) {
			error = -EIO;
		}
		if (udp_sock != udp_presock_fd) {
			zsock_close(udp_sock);
		}
		udp_sock = -1;
	}

	if (IS_ENABLED(CONFIG_DEBUG_COREDUMP_LOGGING_UDP_MIRROR_TO_LOG)) {
		if (error != 0) {
			LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_ERROR_STR);
		}

		LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_END_STR);
	}
}

static void coredump_udp_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	if ((buf == NULL) || (buflen == 0)) {
		return;
	}

	if (IS_ENABLED(CONFIG_DEBUG_COREDUMP_LOGGING_UDP_MIRROR_TO_LOG) && (buf != NULL) &&
	    (buflen > 0)) {
		uint8_t log_ptr = 0;
		size_t remaining = buflen;
		size_t i = 0;

		while (remaining > 0) {
			if (hex2char(buf[i] >> 4, &log_buf[log_ptr]) < 0) {
				error = -EINVAL;
				break;
			}
			log_ptr++;

			if (hex2char(buf[i] & 0xf, &log_buf[log_ptr]) < 0) {
				error = -EINVAL;
				break;
			}
			log_ptr++;

			i++;
			remaining--;

			if ((log_ptr >= LOG_BUF_SZ) || (remaining == 0)) {
				log_buf[log_ptr] = '\0';
				LOG_ERR(COREDUMP_PREFIX_STR "%s", log_buf);
				log_ptr = 0;
			}
		}
	}

	if ((buf != NULL) && (buflen > 0) && (udp_sock >= 0) && (error == 0)) {
		size_t sent = 0U;

		while (sent < buflen && error == 0) {
			size_t chunk = MIN(buflen - sent, (size_t)COREDUMP_UDP_MAX_PAYLOAD);

			if (udp_write_chunk(buf + sent, chunk, 0U) != 0) {
				if (error == 0) {
					error = -EIO;
				}
				break;
			}
			sent += chunk;
		}
	}
}

static int coredump_udp_backend_query(enum coredump_query_id query_id, void *arg)
{
	int ret;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = error;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int coredump_udp_backend_cmd(enum coredump_cmd_id cmd_id, void *arg)
{
	int ret;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		error = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_logging_udp = {
	.start = coredump_udp_backend_start,
	.end = coredump_udp_backend_end,
	.buffer_output = coredump_udp_backend_buffer_output,
	.query = coredump_udp_backend_query,
	.cmd = coredump_udp_backend_cmd,
};
