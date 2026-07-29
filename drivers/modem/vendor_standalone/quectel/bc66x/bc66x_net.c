/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

/*
 * Notes:
 * - Direct push mode only (AT+QIOPEN access_mode = 1)
 * - Hex data mode for send/recv (AT+QICFG="dataformat",1,1)
 * - This driver file handles only the network offloading.
 * - Incoming datagrams are queued in a slab-backed FIFO, consumed when recvfrom
 * (or equivalent) is invoked.
 */
#include <zephyr/drivers/cellular.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/modem/chat.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/net/dns_resolve.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bc66x.h"
#include "bc66x_net.h"
#include "bc66x_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bc66x_net, CONFIG_MODEM_QUECTEL_BC66X_LOG_LEVEL);

K_MEM_SLAB_DEFINE(bc66x_recv_slab, sizeof(struct bc66x_recv_buf), BC66X_SOCKET_RECV_SLAB_COUNT,
		  4); /* Alignment of the memory slab's buffer */

/* Used only in getaddrinfo */
static struct net_if *g_iface;

static int bc66x_error_result_to_errno(int result)
{
	/* Fixed by hw specs */
	switch (result) {
	case 0:
		return 0;
	case 552:
		return -EINVAL;
	case 553:
		return -ENOMEM;
	case 563:
		return -EADDRINUSE;
	case 555:
		return -ENOTSUP;
	case 566:
		return -ECONNREFUSED;
	case 568:
		return -EBUSY;
	case 569:
		return -ETIMEDOUT;
	case 570:
		return -ENETDOWN;
	case 572:
		return -EACCES;
	case 573:
		return -ENETUNREACH;
	case 574:
		return -EADDRINUSE;
	default:
		LOG_WRN("BC660K error %d not translated", result);
		return -EIO;
	}
}

static void bc66x_socket_flush_recv_fifo(struct bc66x_socket *sock)
{
	struct bc66x_recv_buf *pkt;

	while ((pkt = k_fifo_get(&sock->recv_fifo, K_NO_WAIT)) != NULL) {
		atomic_sub(&sock->recv_buff_size, pkt->len);
		LOG_DBG("flush: discarding %zu byte packet for socket %d", pkt->len,
			sock->modem_id);

		k_mem_slab_free(&bc66x_recv_slab, (void *)pkt);
	}
}

static int bc66x_sockaddr_to_str(const struct net_sockaddr *addr, char *ip_buf, size_t ip_buf_len,
				 uint16_t *port_out)
{
	if (addr->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *a = (const struct net_sockaddr_in *)addr;

		net_addr_ntop(NET_AF_INET, &a->sin_addr, ip_buf, ip_buf_len);
		*port_out = net_ntohs(a->sin_port);
		return 0;
	}
	return -EAFNOSUPPORT;
}

/* Opens a modem socket for "TCP" or "UDP" */
/* NOTE: "TCP LISTENER" and "UDP SERVICE" are not supported yet (BC660K only) */
static int bc66x_open_and_wait_socket(struct bc66x_socket *sock, const char *service_type,
				      const struct net_sockaddr *dst_addr)
{
	LOG_DBG("Open and wait invoked");

	struct bc66x_data *data = sock->parent;
	const struct bc66x_config *cfg = data->dev->config;
	char ip_str[NET_IPV4_ADDR_LEN];
	uint16_t port;
	int ret;

	ret = bc66x_sockaddr_to_str(dst_addr, ip_str, sizeof(ip_str), &port);
	if (ret < 0) {
		LOG_ERR("sockaddr to str error %d", ret);
		return ret;
	}

	k_sem_take(&sock->lock, K_FOREVER);

	k_sem_reset(&sock->open_sem);
	sock->open_result = -EIO;
	atomic_set(&sock->state, BC66X_SOCKET_OPENING);

	ret = snprintk(sock->cmd_buf, sizeof(sock->cmd_buf), BC66X_OPEN_SOCKET_AT_COMMAND,
		       cfg->supported_ctx_id, sock->modem_id, service_type, ip_str, port);
	if (ret < 0 || ret >= sizeof(sock->cmd_buf)) {
		k_sem_give(&sock->lock);
		LOG_ERR("Command string error");
		return -ENOMEM;
	}

	LOG_DBG("Opening command: %s", sock->cmd_buf);

	ret = bc66x_run_cmd(data, sock->cmd_buf, BC66X_SCRIPT_TIMEOUT_S);

	/* need to hold the lock until now to "protect" the sock->cmd_buf */
	k_sem_give(&sock->lock);

	if (ret < 0) {
		LOG_ERR("Cannot open modem socket %d error %d", sock->modem_id, ret);
		atomic_set(&sock->state, BC66X_SOCKET_ALLOCATED);
		return ret;
	}

	/* Wait until +QIOPEN: <id>,<err> URC is received. */
	/* URC handler set sock->open_result and give a semaphore.  */
	ret = k_sem_take(&sock->open_sem, BC66X_SOCKET_OPEN_TIMEOUT);
	if (ret < 0 || sock->open_result != 0) {
		LOG_ERR("Error while opening socket. timeout: %d open_result: %d", ret,
			sock->open_result);
		atomic_set(&sock->state, BC66X_SOCKET_ALLOCATED);
		return -EIO;
	}

	atomic_set(&sock->is_connected, 1);

	atomic_set(&sock->state, BC66X_SOCKET_CONNECTED);

	memcpy(&sock->remote_addr, dst_addr, sizeof(struct net_sockaddr_in));

	/* TODO implement "AT+QISTATE" commmand invocation instead of the above memcpy */

	LOG_DBG("SOCKET %d OPENED CORRECTLY", sock->modem_id);
	return 0;
}

static int bc66x_close_and_wait_socket(struct bc66x_socket *sock)
{
	int ret;

	k_sem_take(&sock->lock, K_FOREVER);

	struct bc66x_data *data = sock->parent;

	ret = snprintk(sock->cmd_buf, sizeof(sock->cmd_buf), "AT+QICLOSE=%u", sock->modem_id);
	if (ret < 0 || ret >= sizeof(sock->cmd_buf)) {
		k_sem_give(&sock->lock);
		LOG_ERR("Command string error");
		return -ENOMEM;
	}

	LOG_DBG("Closing command: %s", sock->cmd_buf);

	k_sem_take(&data->net.close_lock, K_FOREVER);
	k_sem_reset(&data->net.close_sem);

	ret = bc66x_run_cmd(data, sock->cmd_buf, BC66X_SCRIPT_TIMEOUT_S);

	/* need to hold the lock until now to "protect" the sock->cmd_buf */
	k_sem_give(&sock->lock);

	if (ret != 0) {
		LOG_WRN("Command error while closing socket %d error: %d", sock->modem_id, ret);
	}

	ret = k_sem_take(&data->net.close_sem, K_MSEC(BC66X_SOCKET_CLOSE_TIMEOUT_MS));
	if (ret != 0) {
		k_sem_give(&data->net.close_lock);
		LOG_ERR("Timeout for CLOSE OK reached");
		errno = ETIMEDOUT;
		return -1;
	}

	k_sem_give(&data->net.close_lock);
	atomic_set(&sock->is_connected, 0);
	return 0;
}

static int bc66x_reset_socket_data(struct bc66x_socket *sock)
{
	if (atomic_get(&sock->is_connected)) {
		LOG_WRN("Trying to reset data for a connected socket. Aborting");
		return -1;
	}

	LOG_DBG("Resetting socket info");

	k_sem_take(&sock->lock, K_FOREVER);

	bc66x_socket_flush_recv_fifo(sock);
	sock->sock_fd = -1;
	sock->open_result = 0;
	sock->recv_timeout = BC66X_SOCKET_DEFAULT_RECV_TIMEOUT;
	sock->send_timeout = BC66X_SOCKET_DEFAULT_SEND_TIMEOUT;
	k_sem_reset(&sock->open_sem);
	k_fifo_cancel_wait(&sock->recv_fifo);
	memset(&sock->remote_addr, 0, sizeof(sock->remote_addr));
	memset(&sock->local_addr, 0, sizeof(sock->local_addr));
	atomic_set(&sock->is_connected, 0);
	atomic_set(&sock->is_nonblocking, 0);
	atomic_set(&sock->recv_buff_size, 0);
	atomic_set(&sock->state, BC66X_SOCKET_FREE);

	k_sem_give(&sock->lock);

	return 0;
}

void on_send_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	LOG_DBG("ON SEND OK received");
	data->net.send_result = 0;
	k_sem_give(&data->net.send_sem);
}

void on_send_fail(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	LOG_DBG("ON SEND FAIL received");
	data->net.send_result = -EIO;
	k_sem_give(&data->net.send_sem);
}

void on_close_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	LOG_DBG("ON CLOSE OK received");
	k_sem_give(&data->net.close_sem);
}

/* Note: +QIDNSGIP: response is treated as URC */
void on_qidnsgip(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;
	int ret, count;

	/* Possible responses:
	 * Format 1: +QIDNSGIP: <result>,<IP_count>,<DNS_ttl>
	 * Format 2: +QIDNSGIP: <hostIPaddr>
	 */
	/* Is this the 1st or 2nd response? */
	if (argc == 4) {
		/* Response format: +QIDNSGIP: <result>,<IP_count>,<DNS_ttl> */
		ret = atoi(argv[1]);
		if (ret != 0) {
			/* DNS error */
			LOG_ERR("DNS error in modem response: %d", ret);
			data->net.dns_error = DNS_EAI_NONAME;
			data->net.dns_host_ip[0] = '\0';
			k_sem_give(&data->net.dns_sem);
			return;
		}

		count = atoi(argv[2]);
		if (count < 1) {
			/* No IP found */
			data->net.dns_error = DNS_EAI_NODATA;
			data->net.dns_host_ip[0] = '\0';
			k_sem_give(&data->net.dns_sem);
			return;
		}
		/* At least 1 result was found, wait for the 2nd response */
		data->net.dns_wait_ip = true;
		return;
	}

	if (argc == 2) {
		/* Response format: +QIDNSGIP: <hostIPaddr> or +QIDNSGIP: <error> */
		if (data->net.dns_wait_ip) {
			bc66x_strip_quotes(argv[1]);
			strncpy(data->net.dns_host_ip, argv[1], NET_IPV4_ADDR_LEN - 1);
			data->net.dns_host_ip[NET_IPV4_ADDR_LEN - 1] = '\0';
			data->net.dns_error = 0;
		} else {
			/* Error */
			LOG_ERR("DNS error in modem response: %s", argv[1]);
			data->net.dns_error = DNS_EAI_NONAME;
			data->net.dns_host_ip[0] = '\0';
		}
		k_sem_give(&data->net.dns_sem);
		return;
	}
}

void on_urc_qiopen(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;
	struct bc66x_socket *sock = NULL;
	int conn_id;
	int result;

	ARG_UNUSED(chat);

	if (argc < 3U) {
		return;
	}

	conn_id = atoi(argv[1]);
	result = atoi(argv[2]);

	if (conn_id < 0 || conn_id >= BC66X_MAX_SOCKETS) {
		LOG_ERR("Received invalid conn_id: %d", conn_id);
		return;
	}

	sock = &data->net.sockets[conn_id];

	if (result != 0) {
		LOG_ERR("Cannot open socket conn_id: %d result: %d", conn_id, result);
	}

	sock->open_result = bc66x_error_result_to_errno(result);

	k_sem_give(&sock->open_sem);
}

void on_urc_closed(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;
	struct bc66x_socket *sock;
	int conn_id;

	ARG_UNUSED(chat);

	if (argc < 2U) {
		return;
	}

	conn_id = atoi(argv[1]);
	if (conn_id < 0 || conn_id >= BC66X_MAX_SOCKETS) {
		LOG_ERR("+QIURC closed invalid socket received: %d", conn_id);
		return;
	}

	sock = &data->net.sockets[conn_id];

	if (atomic_get(&sock->state) != BC66X_SOCKET_CONNECTED) {
		LOG_WRN("+QIURC closed for a non-connected socket (modem ID): %d", sock->modem_id);
	}

	LOG_DBG("Socket closed by remote");
}

void on_urc_recv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* BC660K direct-push UDP format:
	 * argv[0] = "+QIURC: \"recv\","
	 * argv[1] = socket id
	 * argv[2] = data length
	 * argv[3] = "hexdata" (quoted)
	 * NO IP/PORT in this mode
	 */

	LOG_DBG("URC recv invoked");

	if (argc < 4) {
		/* TODO: implement UDP SERVICE for BC660K */
		LOG_WRN("unexpected argc %d", argc);
		return;
	}

	int modem_id = atoi(argv[1]);
	size_t data_len = (size_t)atoi(argv[2]);
	char *hex_str = argv[3];
	struct bc66x_socket *sock;
	struct bc66x_recv_buf *pkt;
	int decoded_len;

	if (modem_id < 0 || modem_id >= BC66X_MAX_SOCKETS) {
		LOG_ERR("Invalid modem_id %d", modem_id);
		return;
	}

	bc66x_strip_quotes(hex_str);

	LOG_DBG("RECV id=%d len=%zu", modem_id, data_len);

	if (data_len > BC66X_SOCKET_RECV_MAX_SIZE) {
		LOG_ERR("Payload too large (%zu > %u), dropping", data_len,
			BC66X_SOCKET_RECV_MAX_SIZE);
		return;
	}

	sock = &data->net.sockets[modem_id];
	if (!sock) {
		LOG_ERR("No socket for modem_id %d", modem_id);
		return;
	}

	if (k_mem_slab_alloc(&bc66x_recv_slab, (void **)&pkt, K_NO_WAIT) != 0) {
		LOG_ERR("Slab exhausted, dropping datagram");
		return;
	}

	decoded_len = hex2bin(hex_str, strlen(hex_str), pkt->data, sizeof(pkt->data));
	if (decoded_len < 0) {
		LOG_ERR("Hex decode failed: %d", decoded_len);
		k_mem_slab_free(&bc66x_recv_slab, (void *)pkt);
		return;
	}
	pkt->len = (size_t)decoded_len;

	LOG_DBG("Encoded packet: %s", hex_str);

	/* The modem does not report the sender's address in this URC mode
	 * Assume it is who we sent the request to
	 */
	memcpy(&pkt->src_addr, &sock->remote_addr, sizeof(struct net_sockaddr_in));

	k_fifo_put(&sock->recv_fifo, pkt);
	atomic_add(&sock->recv_buff_size, pkt->len);
	LOG_DBG("Queued %zu bytes to socket %d fifo", data_len, modem_id);
}

void on_urc_qiping(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	int result = -1;

	/* Adding some extra space to contain also the quotes "" */
	char ip_str[NET_IPV4_ADDR_LEN + 4];

	switch (argc) {
	case 2:
		/* Error case
		 * Response format: +QPING: <err>
		 */
		LOG_ERR("Received ping error: %s", argv[1]);
		/* no break (intended) */
	case 8:
		/* Summary response case
		 * Response format: +QPING: <finresult>,<sent>,<rcvd>,<lost>,<min>,<max>,<avg>
		 * Ignoring every param
		 */
		LOG_DBG("Clearing ping ctx and user_data");
		data->net.active_ping_ctx = NULL;
		data->net.active_ping_user_data = NULL;
		return;
	case 6:
		/* Single ping reply case
		 * Response format: +QPING: <result>,<IP_address>,<bytes>,<time>,<ttl>
		 */
		result = atoi(argv[1]);
		strncpy(ip_str, argv[2], sizeof(ip_str) - 1);
		ip_str[sizeof(ip_str) - 1] = '\0';
		bc66x_strip_quotes(ip_str);
		break;
	default:
		LOG_WRN("Unhandled ping response");
	}

	if (data->net.active_ping_ctx == NULL) {
		LOG_WRN("Received PING response but no ping ctx is set. Discarding...");
		return;
	}

	if (result != 0) {
		LOG_DBG("Reported IP failure code: %d", result);
		return;
	}

	/* RTT ignored */
	int ttl = atoi(argv[5]);

	net_icmp_handler_t resp_handler = NULL;

	if (net_icmp_get_offload_rsp_handler(&data->net.icmp_offload, &resp_handler) < 0 ||
	    resp_handler == NULL) {
		LOG_WRN("No ICMP offload response handler registered");
		return;
	}

	struct net_ipv4_hdr ipv4_hdr = {0};

	if (net_addr_pton(NET_AF_INET, ip_str, &ipv4_hdr.src) < 0) {
		LOG_ERR("Invalid IP in ping reply: %s", ip_str);
		return;
	}

	ipv4_hdr.ttl = (uint8_t)ttl;
	ipv4_hdr.proto = NET_IPPROTO_ICMP;

	struct net_icmp_ip_hdr ip_hdr = {
		.family = NET_AF_INET,
		.ipv4 = &ipv4_hdr,
	};

	struct net_icmp_hdr icmp_hdr = {
		.type = NET_ICMPV4_ECHO_REPLY,
		.code = 0,
	};

	enum net_verdict verdict = resp_handler(data->net.active_ping_ctx, NULL, &ip_hdr, &icmp_hdr,
						data->net.active_ping_ctx->user_data);

	switch (verdict) {
	case NET_OK:
		LOG_DBG("Ping response delivered (NET_OK)");
		break;
	case NET_CONTINUE:
		LOG_DBG("Ping response not consumed (NET_CONTINUE)");
		break;
	case NET_DROP:
		LOG_DBG("Ping response dropped (NET_DROP)");
		break;
	default:
		LOG_WRN("Unhandled ping response verdict");
	}
}

/* BC66 only */
void on_urc_dnsgip(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	/* +QIURC: "dnsgip", ... */
	on_qidnsgip(chat, argv, argc, user_data);
}

/* TODO: Honor the flags (e.g. implement "non blocking" path) */
static ssize_t bc66x_network_sendto(void *obj, const void *buf, size_t len, int flags,
				    const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{

	struct bc66x_socket *sock = (struct bc66x_socket *)obj;
	struct bc66x_data *data = sock->parent;
	const struct bc66x_config *cfg = data->dev->config;

	/* Each byte needs 2 hex chars to be represented, +1 for null terminator */
	char hex_buf[(BC66X_MAX_HEX_SEND_LENGTH * 2U) + 1U];
	int ret;

	ARG_UNUSED(flags);

	k_sem_take(&sock->lock, K_FOREVER);

	int proto = sock->ip_proto;

	k_sem_give(&sock->lock);

	switch (proto) {
	case NET_IPPROTO_TCP:
		if (!atomic_get(&sock->is_connected)) {
			LOG_ERR("sendto invoked for a non-connected TCP socket");
			errno = ENOTCONN;
			return -1;
		}
		break;

	case NET_IPPROTO_UDP:
		if (!atomic_get(&sock->is_connected)) {
			/* dest_addr MUST be valid if the socket is not connected.  */
			if (!dest_addr || addrlen < sizeof(struct net_sockaddr_in)) {
				LOG_ERR("sendto invoked for non-connected UDP socket and no addr "
					"passed");
				errno = EDESTADDRREQ;
				return -1;
			}

			LOG_DBG("Opening and connecting UDP socket");

			ret = bc66x_open_and_wait_socket(sock, BC66X_SOCKET_SERVICE_TYPE_UDP,
							 dest_addr);
			if (ret < 0) {
				errno = ENETUNREACH;
				return -1;
			}
		} else {
			/* TODO: Check and send to the address in dest_addr, ignoring the
			 * pre-connected peer.
			 */
			LOG_DBG("Already connected UDP socket");
		}
		break;

	default:
		LOG_ERR("Invalid socket protocol: %d", proto);
		errno = EIO; /* Generic */
		return -1;
	}

	if (len == 0 || len > BC66X_MAX_HEX_SEND_LENGTH) {
		errno = EMSGSIZE;
		return -1;
	}

	k_timeout_t timeout;

	/* Prepare to send */
	bin2hex((const uint8_t *)buf, len, hex_buf, sizeof(hex_buf));

	k_sem_take(&sock->lock, K_FOREVER);

	data->net.send_result = -EIO;

	ret = snprintk(sock->cmd_buf, sizeof(sock->cmd_buf), cfg->send_cmd, sock->modem_id,
		       (unsigned int)len, hex_buf);
	if (ret < 0 || ret >= sizeof(sock->cmd_buf)) {
		LOG_ERR("Command string error");
		k_sem_give(&sock->lock);
		errno = EMSGSIZE;
		return -1;
	}

	LOG_DBG("Sending command: %s", sock->cmd_buf);

	k_sem_take(&data->net.send_lock, K_FOREVER);
	k_sem_reset(&data->net.send_sem);

	timeout = sock->send_timeout;

	ret = bc66x_run_cmd(data, sock->cmd_buf, BC66X_SCRIPT_TIMEOUT_S);

	/* need to hold the lock until now to "protect" the sock->cmd_buf */
	k_sem_give(&sock->lock);

	if (ret != 0) {
		LOG_ERR("Command error for SEND, error %d", ret);
		k_sem_give(&data->net.send_lock);
		errno = ECOMM;
		return -1;
	}

	/* Wait for OK by modem */
	ret = k_sem_take(&data->net.send_sem, timeout);
	if (ret != 0) {
		k_sem_give(&data->net.send_lock);
		LOG_ERR("Timeout for SEND OK reached");
		errno = ETIMEDOUT;
		return -1;
	}

	if (data->net.send_result != 0) {
		k_sem_give(&data->net.send_lock);
		LOG_ERR("Received SEND FAIL");
		errno = EIO;
		return -1;
	}
	k_sem_give(&data->net.send_lock);

	LOG_DBG("SEND OK received");

	return (ssize_t)len;
}

/* TODO: Honor every flag */
static ssize_t bc66x_network_recvfrom(void *obj, void *buf, size_t max_len, int flags,
				      struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	struct bc66x_socket *sock = (struct bc66x_socket *)obj;

	LOG_DBG("Recv from invoked");

	if (!buf || max_len == 0) {
		errno = EINVAL;
		return -1;
	}

	k_timeout_t timeout = sock->recv_timeout;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = K_NO_WAIT;
	}

	if (atomic_get(&sock->is_nonblocking)) {
		timeout = K_NO_WAIT;
	}

	struct bc66x_recv_buf *pkt = k_fifo_get(&sock->recv_fifo, timeout);

	if (!pkt) {
		LOG_DBG("No packet received");
		errno = EAGAIN;
		return -1;
	}

	size_t copy_len = MIN(max_len, pkt->len);

	memcpy(buf, pkt->data, copy_len);

	atomic_sub(&sock->recv_buff_size, pkt->len);

	if (src_addr && addrlen) {
		size_t addr_copy = MIN(*addrlen, sizeof(struct net_sockaddr_in));

		memcpy(src_addr, &pkt->src_addr, addr_copy);
		*addrlen = sizeof(struct net_sockaddr_in);
	}

	LOG_DBG("socket %d got %zu bytes", sock->modem_id, pkt->len);

	k_mem_slab_free(&bc66x_recv_slab, (void *)pkt);

	return (ssize_t)copy_len;
}

static int bc66x_network_connect(void *obj, const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	LOG_DBG("Connect invoked");

	if (addr->sa_family != NET_AF_INET) {
		errno = ENOTSUP;
		return -1;
	}

	struct bc66x_socket *sock = (struct bc66x_socket *)obj;
	int ret;

	k_sem_take(&sock->lock, K_FOREVER);

	if (atomic_get(&sock->is_connected)) {
		k_sem_give(&sock->lock);
		LOG_DBG("Already connected socket, closing and reopening (fd: %d modem_id: %d)",
			sock->sock_fd, sock->modem_id);

		ret = bc66x_close_and_wait_socket(sock);
		/* is_connected is set to zero inside the above function */
		if (ret != 0) {
			LOG_ERR("Error while closing socket : %d", errno);
			errno = EFAULT;
			return -1;
		}
		k_sem_take(&sock->lock, K_FOREVER);
	}

	/* Will contain "UDP" or "TCP" */
	char service_type[4];

	memset(service_type, 0, sizeof(service_type));

	if (sock->ip_proto == NET_IPPROTO_UDP) {
		LOG_DBG("Connecting UDP socket");
		strncpy(service_type, BC66X_SOCKET_SERVICE_TYPE_UDP, sizeof(service_type));
	} else if (sock->ip_proto == NET_IPPROTO_TCP) {
		LOG_DBG("Connecting TCP socket");
		strncpy(service_type, BC66X_SOCKET_SERVICE_TYPE_TCP, sizeof(service_type));
	} else {
		k_sem_give(&sock->lock);
		LOG_ERR("Cannot connect socket, family %d not supported", sock->family);
		errno = EPROTONOSUPPORT;
		return -1;
	}

	k_sem_give(&sock->lock);

	ret = bc66x_open_and_wait_socket(sock, service_type, addr);
	if (ret < 0) {
		LOG_ERR("Cannot open modem socket");
		errno = ENETUNREACH;
		return -1;
	}

	return 0;
}

static int bc66x_network_close(void *obj)
{
	struct bc66x_socket *sock = (struct bc66x_socket *)obj;

	LOG_DBG("Invoked close on socket id %d", sock->modem_id);

	int ret = 0;

	switch (atomic_get(&sock->state)) {
	case BC66X_SOCKET_FREE:
		LOG_WRN("Closing a free socket??");
		break;
	case BC66X_SOCKET_ALLOCATED:
		/* Nothing to do, just set the state to FREE  */
		LOG_DBG("Freeing allocated socket");
		break;
	case BC66X_SOCKET_OPENING:
	case BC66X_SOCKET_OPENED:
	case BC66X_SOCKET_CONNECTED:
		LOG_DBG("Closing opened / connected socket");

		ret = bc66x_close_and_wait_socket(sock);
		if (ret != 0) {
			LOG_ERR("Error while closing socket : %d", errno);
			ret = -1;
		}
		break;
	default:
		LOG_WRN("Closing socket with unknown status: %ld", atomic_get(&sock->state));
		break;
	}

	bc66x_reset_socket_data(sock);

	return ret;
}

static ssize_t bc66x_network_read(void *obj, void *buf, size_t sz)
{
	return bc66x_network_recvfrom(obj, buf, sz, 0, NULL, NULL);
}

static ssize_t bc66x_network_write(void *obj, const void *buf, size_t sz)
{
	return bc66x_network_sendto(obj, buf, sz, 0, NULL, 0);
}

static void bc66x_ioctl_fionread(struct bc66x_socket *sock, int *response)
{
	atomic_val_t unread_bytes = atomic_get(&sock->recv_buff_size);

	/* This should never happen... */
	if (unread_bytes < 0) {
		*response = 0;
	} else if (unread_bytes > INT_MAX) {
		LOG_WRN("INT_MAX reached with recv_buff_size");
		*response = INT_MAX;
	} else {
		*response = (int)unread_bytes;
	}

	LOG_DBG("%ld bytes in the fifo queue ready to be read", unread_bytes);
}

static void bc66x_ioctl_poll_update(struct bc66x_socket *sock, struct zsock_pollfd *pfd,
				    struct k_poll_event **pev)
{
	if (pfd->events & ZSOCK_POLLIN) {
		/* Check the actual FIFO data structure directly. */
		if (!k_fifo_is_empty(&sock->recv_fifo)) {
			LOG_DBG("POLLIN UPDATE: Data available in FIFO");
			pfd->revents |= ZSOCK_POLLIN;
		} else if ((*pev)->state == K_POLL_STATE_CANCELLED) {
			LOG_DBG("POLLIN UPDATE: Wait cancelled (socket closed/error)");
			pfd->revents |= ZSOCK_POLLERR;
		} else {
			LOG_DBG("POLLIN UPDATE: Spurious wakeup, FIFO is empty");
		}

		/* Always advance the pointer to match the PREPARE stage  */
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		/* Assumption: always ready to transmit */
		pfd->revents |= ZSOCK_POLLOUT;
	}
}

/* TODO: Implement every request */
static int bc66x_network_ioctl(void *obj, unsigned int request, va_list args)
{
	LOG_DBG("ioctl invoked with request: %u", request);

	struct bc66x_socket *sock = (struct bc66x_socket *)obj;

	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;
	int *response;
	int *mode_ptr;

	switch (request) {
	case ZVFS_F_GETFL:
		LOG_DBG("Flag requested");
		return atomic_get(&sock->is_nonblocking) ? ZVFS_O_NONBLOCK : 0;

	case ZFD_IOCTL_FIONREAD:
		/* "How many bytes are available to read right now without blocking?" */
		response = va_arg(args, int *);
		if (response == NULL) {
			errno = EFAULT;
			return -1;
		}

		bc66x_ioctl_fionread(sock, response);
		break;
	case ZFD_IOCTL_FIONBIO:
		/* "toggle non-blocking mode" */
		mode_ptr = va_arg(args, int *);

		if (mode_ptr == NULL) {
			errno = EFAULT;
			return -1;
		}

		atomic_set(&sock->is_nonblocking, (*mode_ptr != 0) ? 1 : 0);

		LOG_DBG("Setting nonblocking to %d", (*mode_ptr != 0) ? 1 : 0);
		break;

	case ZFD_IOCTL_POLL_PREPARE:
		LOG_DBG("ZFD_IOCTL_POLL_PREPARE");

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		struct k_poll_event *pev_end = va_arg(args, struct k_poll_event *);

		if (pfd->events & ZSOCK_POLLIN) {
			if (*pev == pev_end) {
				LOG_ERR("Cannot register poll: no slots");
				return -ENOMEM;
			}
			/* Register the event to wait for data on the FIFO */
			k_poll_event_init(*pev, K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY, &sock->recv_fifo);
			(*pev)++;
		}
		break;

	case ZFD_IOCTL_POLL_UPDATE:
		LOG_DBG("ZFD_IOCTL_POLL_UPDATE");

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		if (pfd == NULL || pev == NULL) {
			errno = EFAULT;
			return -1;
		}

		bc66x_ioctl_poll_update(sock, pfd, pev);
		break;

	case ZFD_IOCTL_POLL_OFFLOAD:
		LOG_DBG("%d = ZFD_IOCTL_POLL_OFFLOAD", request);
		return -ENOTSUP;

	case ZFD_IOCTL_SET_LOCK:
		LOG_DBG("%d = ZFD_IOCTL_SET_LOCK", request);
		return -EOPNOTSUPP;
	case ZFD_IOCTL_FSYNC:
		LOG_DBG("%d = ZFD_IOCTL_FSYNC", request);
		return -ENOTSUP;
	case ZFD_IOCTL_LSEEK:
		LOG_DBG("%d = ZFD_IOCTL_LSEEK", request);
		return -ENOTSUP;
	case ZFD_IOCTL_STAT:
		LOG_DBG("%d = ZFD_IOCTL_STAT", request);
		return -ENOTSUP;
	case ZFD_IOCTL_TRUNCATE:
		LOG_DBG("%d = ZFD_IOCTL_TRUNCATE", request);
		return -ENOTSUP;
	case ZFD_IOCTL_MMAP:
		LOG_DBG("%d = ZFD_IOCTL_MMAP", request);
		return -ENOTSUP;
	default:
		LOG_WRN("Invalid / not implemented ioctl request: %d", request);
		return -ENOTSUP;
	}

	return 0;
}

static int bc66x_network_setsockopt(void *obj, int level, int optname, const void *optval,
				    net_socklen_t optlen)
{
	LOG_DBG("Setsockopt invoked");

	struct bc66x_socket *sock = (struct bc66x_socket *)obj;
	int64_t total_msec;
	const struct timeval *tv_timeout;

	switch (optname) {
	case ZSOCK_SO_DEBUG:
	case ZSOCK_SO_BROADCAST:
	case ZSOCK_SO_REUSEADDR:
	case ZSOCK_SO_KEEPALIVE:
	case ZSOCK_SO_LINGER:
	case ZSOCK_SO_OOBINLINE:
	case ZSOCK_SO_SNDBUF:
	case ZSOCK_SO_RCVBUF:
	case ZSOCK_SO_DONTROUTE:
	case ZSOCK_SO_RCVLOWAT:
	case ZSOCK_SO_SNDLOWAT:
		LOG_WRN("Valid optname received but not implemented: %d", optname);
		errno = ENOTSUP;
		return -1;

	case ZSOCK_SO_BINDTODEVICE:
		errno = EOPNOTSUPP;
		return -1;

	case ZSOCK_SO_SNDTIMEO:
		if (!optval) {
			errno = EINVAL;
			return -1;
		}
		tv_timeout = (const struct timeval *)optval;
		total_msec =
			(tv_timeout->tv_sec * MSEC_PER_SEC) + (tv_timeout->tv_usec / USEC_PER_MSEC);
		LOG_DBG("Setting send timeout to %lld ms", total_msec);
		k_sem_take(&sock->lock, K_FOREVER);

		sock->send_timeout = K_MSEC(total_msec);

		k_sem_give(&sock->lock);
		break;

	case ZSOCK_SO_RCVTIMEO:
		if (!optval) {
			errno = EINVAL;
			return -1;
		}

		tv_timeout = (const struct timeval *)optval;
		total_msec =
			(tv_timeout->tv_sec * MSEC_PER_SEC) + (tv_timeout->tv_usec / USEC_PER_MSEC);
		LOG_DBG("Setting receive timeout to %lld ms", total_msec);
		k_sem_take(&sock->lock, K_FOREVER);

		sock->recv_timeout = K_MSEC(total_msec);

		k_sem_give(&sock->lock);
		break;

	default:
		LOG_WRN("Invalid / unsupported optname received: %d", optname);
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int bc66x_network_getpeername(void *obj, struct net_sockaddr *addr, net_socklen_t *addrlen)
{
	LOG_DBG("Get peer name invoked");
	struct bc66x_socket *sock = (struct bc66x_socket *)obj;

	if (!addr || !addrlen) {
		errno = EINVAL;
		return -1;
	}

	if (!atomic_get(&sock->is_connected)) {
		errno = ENOTCONN;
		return -1;
	}

	if (*addrlen < sizeof(struct net_sockaddr_in)) {
		errno = EINVAL;
		return -1;
	}

	memcpy(addr, &sock->remote_addr, sizeof(struct net_sockaddr_in));
	*addrlen = sizeof(struct net_sockaddr_in);

	return 0;
}

/* TODO: Implement missing functions */
static const struct socket_op_vtable bc66x_socket_vtable = {
	.fd_vtable = {
			.read = bc66x_network_read,
			.write = bc66x_network_write,
			.close = bc66x_network_close,
			.ioctl = bc66x_network_ioctl,
		},
	.sendto = bc66x_network_sendto,
	.recvfrom = bc66x_network_recvfrom,
	.setsockopt = bc66x_network_setsockopt,
	.connect = bc66x_network_connect,
	.getpeername = bc66x_network_getpeername,
};

static int bc66x_getaddrinfo(const char *node, const char *service,
			     const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{

	LOG_DBG("DNS offload invoked: resolving %s", node);

	int ret;

	if (g_iface == NULL) {
		LOG_ERR("Global iface not initialized");
		errno = ENXIO;
		return DNS_EAI_SYSTEM;
	}

	const struct device *dev = net_if_get_device(g_iface);

	if (!dev || !device_is_ready(dev)) {
		LOG_ERR("Cannot get device or device not ready");
		errno = ENXIO;
		return DNS_EAI_SYSTEM;
	}

	struct bc66x_data *data = (struct bc66x_data *)dev->data;
	const struct bc66x_config *cfg = dev->config;

	if (!atomic_get(&data->is_initialized)) {
		LOG_ERR("Modem not initialized yet");
		return DNS_EAI_AGAIN;
	}

	char cmd_buf[BC66X_MEDIUM_CMD_BUFF_SIZE];
	char dns_res[NET_IPV4_ADDR_LEN];

	/* Setup DNS  */

	/* NOTE: "should be configured after the module has successfully registered to
	 * the network, namely after the IP address URC (e.g. +IP: 10.18.237.42) is
	 * reported."
	 */
	ret = snprintk(cmd_buf, sizeof(cmd_buf), "AT+QIDNSCFG=%d,\"%s\",\"%s\"",
		       cfg->supported_ctx_id, CONFIG_MODEM_QUECTEL_BC66X_DNS_ADDRESS_1,
		       CONFIG_MODEM_QUECTEL_BC66X_DNS_ADDRESS_2);
	if (ret < 0 || ret >= sizeof(cmd_buf)) {
		LOG_ERR("Command string error");
		return DNS_EAI_MEMORY;
	}

	LOG_DBG("DNS config command: %s", cmd_buf);

	ret = bc66x_run_cmd(data, cmd_buf, BC66X_SCRIPT_TIMEOUT_S);
	if (ret < 0) {
		LOG_ERR("Cannot configure DNS, error %d", ret);
		return DNS_EAI_AGAIN;
	}

	/* Send the DNS request */
	k_sem_take(&data->net.dns_lock, K_FOREVER);

	data->net.dns_error = 0;
	k_sem_reset(&data->net.dns_sem);
	data->net.dns_wait_ip = false;

	ret = snprintk(cmd_buf, sizeof(cmd_buf), "AT+QIDNSGIP=%u,\"%s\"", cfg->supported_ctx_id,
		       node);
	if (ret < 0 || ret >= sizeof(cmd_buf)) {
		k_sem_give(&data->net.dns_lock);
		LOG_ERR("Command string error");
		return DNS_EAI_MEMORY;
	}

	LOG_DBG("DNS request command: %s", cmd_buf);
	ret = bc66x_run_cmd(data, cmd_buf, BC66X_SCRIPT_TIMEOUT_S);
	if (ret < 0) {
		k_sem_give(&data->net.dns_lock);
		LOG_ERR("Cannot send DNS request, error %d", ret);
		return DNS_EAI_AGAIN;
	}

	ret = k_sem_take(&data->net.dns_sem, K_SECONDS(CONFIG_MODEM_QUECTEL_BC66X_DNS_TIMEOUT));
	if (ret < 0) {
		k_sem_give(&data->net.dns_lock);
		LOG_WRN("DNS timeout");
		return DNS_EAI_AGAIN;
	}

	if (data->net.dns_error != 0) {
		k_sem_give(&data->net.dns_lock);
		LOG_WRN("DNS error: %d", data->net.dns_error);
		return DNS_EAI_NONAME;
	}

	strncpy(dns_res, data->net.dns_host_ip, sizeof(dns_res) - 1);
	dns_res[sizeof(dns_res) - 1] = '\0';

	k_sem_give(&data->net.dns_lock);

	LOG_DBG("Received %s from DNS", dns_res);

	struct zsock_addrinfo *ai = k_calloc(1, sizeof(*ai));

	if (!ai) {
		return DNS_EAI_MEMORY;
	}

	struct net_sockaddr_in *sa = k_calloc(1, sizeof(*sa));

	if (!sa) {
		k_free(ai);
		return DNS_EAI_MEMORY;
	}

	sa->sin_family = NET_AF_INET;

	/* Convert string IP to network byte order */
	zsock_inet_pton(NET_AF_INET, dns_res, &sa->sin_addr);

	/* If a service/port was requested (e.g., "80"), convert it */
	if (service != NULL) {
		sa->sin_port = net_htons(atoi(service));
	} else {
		sa->sin_port = 0;
	}

	ai->ai_family = NET_AF_INET;
	ai->ai_addrlen = sizeof(struct net_sockaddr_in);
	ai->ai_addr = (struct net_sockaddr *)sa;
	ai->ai_canonname = NULL;
	ai->ai_next = NULL;

	/* Inherit socket type and protocol from hints if provided, otherwise default
	 * to UDP
	 */
	if (hints != NULL) {
		ai->ai_socktype = hints->ai_socktype ? hints->ai_socktype : NET_SOCK_DGRAM;
		ai->ai_protocol = hints->ai_protocol ? hints->ai_protocol : NET_IPPROTO_UDP;
	} else {
		ai->ai_socktype = NET_SOCK_DGRAM;
		ai->ai_protocol = NET_IPPROTO_UDP;
	}

	*res = ai;

	return 0;
}

static void bc66x_freeaddrinfo(struct zsock_addrinfo *res)
{
	LOG_DBG("Freeaddrinfo invoked");
	while (res) {
		struct zsock_addrinfo *next = res->ai_next;

		k_free(res->ai_addr);
		k_free(res);
		res = next;
	}
}

static const struct socket_dns_offload bc66x_dns_ops = {
	.getaddrinfo = bc66x_getaddrinfo,
	.freeaddrinfo = bc66x_freeaddrinfo,
};

static int bc66x_ping(struct net_icmp_ctx *ctx, struct net_if *iface, struct net_sockaddr *dst,
		      struct net_icmp_ping_params *params, void *user_data)
{
	/* Params are not supported by the modem */
	ARG_UNUSED(params);

	LOG_DBG("Offload PING invoked");

	if (iface == NULL) {
		return -EINVAL;
	}

	const struct device *dev = net_if_get_device(iface);
	struct bc66x_data *data = dev->data;
	const struct bc66x_config *cfg = dev->config;

	int ret;

	char ip_str[NET_IPV4_ADDR_LEN];
	uint16_t ignored_port;

	ret = bc66x_sockaddr_to_str(dst, ip_str, sizeof(ip_str), &ignored_port);
	if (ret < 0) {
		LOG_ERR("Cannot convert IP to string, error %d", ret);
		return ret;
	}

	/* NOTE: Sending multiple AT+QPING without waiting "the first one" to be
	 * completed will result in `ERROR`
	 * i.e. The concurrency is "managed" by the modem, no need to
	 * keep track here.
	 */

	char ping_cmd_buf[BC66X_SMALL_CMD_BUFF_SIZE];

	/* Send just 1 ping with default timeout (4) */
	ret = snprintk(ping_cmd_buf, sizeof(ping_cmd_buf), "AT+QPING=%u,\"%s\",4,1",
		       cfg->supported_ctx_id, ip_str);
	if (ret < 0 || ret >= sizeof(ping_cmd_buf)) {
		LOG_ERR("Command string error");
		return -ENOMEM;
	}

	LOG_DBG("Sending PING command: %s", ping_cmd_buf);

	ret = bc66x_run_cmd(data, ping_cmd_buf, BC66X_SCRIPT_TIMEOUT_S);
	if (ret < 0) {
		LOG_ERR("Cannot send PING command, probably another ping is still awaited");
		return -EAGAIN;
	}

	/* Cleared when handling URC */
	data->net.active_ping_ctx = ctx;
	data->net.active_ping_user_data = user_data;

	return 0;
}

bool bc66x_offload_is_supported(int family, int type, int proto)
{
	if (family != NET_AF_INET) {
		return false;
	}

	if (type == NET_SOCK_STREAM && (proto == NET_IPPROTO_TCP || proto == 0)) {
		return true;
	}

	if (type == NET_SOCK_DGRAM && (proto == NET_IPPROTO_UDP || proto == 0)) {
		return true;
	}

	return false;
}

/* May be blocking due to mutexes */
int bc66x_create_socket(struct bc66x_data *data, int family, int type, int proto)
{
	/* This should've already been checked by bc66x_offload_is_supported, but
	 * still...
	 */
	if (family != NET_AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (type != NET_SOCK_STREAM && type != NET_SOCK_DGRAM) {
		errno = EPROTONOSUPPORT;
		return -1;
	}

	struct bc66x_network_data *net = &data->net;
	struct bc66x_socket *sock = NULL;
	int fd;

	LOG_DBG("Create socket invoked");

	/* Find a free socket  */
	k_sem_take(&net->sockets_lock, K_FOREVER);
	for (int i = 0; i < BC66X_MAX_SOCKETS; i++) {
		if (atomic_cas(&net->sockets[i].state, BC66X_SOCKET_FREE, BC66X_SOCKET_ALLOCATED)) {
			sock = &net->sockets[i];
			break;
		}
	}
	k_sem_give(&net->sockets_lock);

	if (!sock) {
		LOG_ERR("No socket available");
		errno = ENOMEM;
		return -1;
	}

	k_sem_take(&sock->lock, K_FOREVER);

	sock->family = family;
	sock->type = type;
	sock->ip_proto = proto;
	sock->parent = data;

	/* Ask a file descriptor to Zephyr  */
	fd = zvfs_reserve_fd();
	if (fd < 0) {
		k_sem_give(&sock->lock);
		bc66x_reset_socket_data(sock);
		atomic_set(&sock->state, BC66X_SOCKET_FREE);
		errno = ENOMEM;
		return -1;
	}

	sock->sock_fd = fd;

	/* Link to bc66x_socket_vtable */
	zvfs_finalize_fd(fd, sock, (const struct fd_op_vtable *)&bc66x_socket_vtable);

	k_sem_give(&sock->lock);

	LOG_DBG("Reserved socket %d with fd %d", sock->modem_id, fd);
	return fd;
}

void bc66x_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct bc66x_data *data = dev->data;
	int ret;

	LOG_DBG("Iface init invoked");

	data->net.iface = iface;
	g_iface = iface;

	net_if_socket_offload_set(iface, data->net.socket_creator);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);

	/* DNS offloading */
	socket_offload_dns_register(&bc66x_dns_ops);

	/* PING offloading */
	ret = net_icmp_register_offload_ping(&data->net.icmp_offload, iface, bc66x_ping);
	if (ret < 0) {
		LOG_ERR("Cannot register ping offloading");
	}
}

int bc66x_network_init(const struct device *dev)
{
	LOG_DBG("Network init invoked");

	struct bc66x_data *data = dev->data;

	struct bc66x_network_data *net_data = &data->net;

	k_sem_init(&net_data->send_lock, 1, 1);
	k_sem_init(&net_data->sockets_lock, 1, 1);
	k_sem_init(&net_data->close_lock, 1, 1);

	k_sem_init(&net_data->send_sem, 0, BC66X_SOCKET_SEM_LIMIT);
	k_sem_init(&net_data->close_sem, 0, BC66X_SOCKET_SEM_LIMIT);

	for (int i = 0; i < BC66X_MAX_SOCKETS; i++) {
		struct bc66x_socket *sock = &net_data->sockets[i];

		sock->parent = data;
		sock->modem_id = i;
		sock->state = ATOMIC_INIT(BC66X_SOCKET_FREE);
		sock->open_result = 0;
		sock->recv_timeout = BC66X_SOCKET_DEFAULT_RECV_TIMEOUT;
		sock->send_timeout = BC66X_SOCKET_DEFAULT_SEND_TIMEOUT;
		memset(&sock->remote_addr, 0, sizeof(sock->remote_addr));
		memset(&sock->local_addr, 0, sizeof(sock->local_addr));
		sock->is_connected = ATOMIC_INIT(0);
		sock->is_nonblocking = ATOMIC_INIT(0);
		sock->recv_buff_size = ATOMIC_INIT(0);

		k_sem_init(&sock->lock, 1, 1);
		k_sem_init(&sock->open_sem, 0, BC66X_SOCKET_SEM_LIMIT);
		k_fifo_init(&sock->recv_fifo);
	}

	/* DNS */
	k_sem_init(&net_data->dns_lock, 1, 1);
	k_sem_init(&net_data->dns_sem, 0, 1);
	net_data->dns_error = 0;
	net_data->dns_host_ip[0] = '\0';
	net_data->dns_wait_ip = false;

	return 0;
}
