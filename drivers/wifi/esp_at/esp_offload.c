/*
 * Copyright (c) 2019 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_esp_at_offload, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>

#include "esp.h"

#define TCP_SEND_TIMEOUT                                        \
	K_MSEC(CONFIG_WIFI_ESP_AT_TCP_SEND_TIMEOUT)

static int esp_listen(struct net_context *context, int backlog)
{
	return -ENOTSUP;
}

static int _sock_connect(struct esp_data *dev, struct esp_socket *sock)
{
	/* Calculate the largest possible AT command length based on both TCP and UDP variants. */
	char connect_msg[MAX(sizeof("AT+CIPSTART=000,\"TCP\",\"\",65535,7200") +
				NET_IPV4_ADDR_LEN,
			     sizeof("AT+CIPSTART=000,\"UDP\",\"\",65535,65535,0,\"\"") +
				2 * NET_IPV4_ADDR_LEN)];
	char dst_addr_str[NET_IPV4_ADDR_LEN];
	char src_addr_str[NET_IPV4_ADDR_LEN];
	struct sockaddr src;
	struct sockaddr dst;
	int ret;
	int mode;

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED | EDF_AP_ENABLED)) {
		return -ENETUNREACH;
	}

	k_mutex_lock(&sock->lock, K_FOREVER);
	src = sock->src;
	dst = sock->dst;
	k_mutex_unlock(&sock->lock);

	if (dst.sa_family == AF_INET) {
		net_addr_ntop(dst.sa_family,
			      &net_sin(&dst)->sin_addr,
			      dst_addr_str, sizeof(dst_addr_str));
	} else {
		strcpy(dst_addr_str, "0.0.0.0");
	}

	if (esp_socket_ip_proto(sock) == IPPROTO_TCP) {
		snprintk(connect_msg, sizeof(connect_msg),
			 "AT+CIPSTART=%d,\"TCP\",\"%s\",%d,7200",
			 sock->link_id, dst_addr_str,
			 ntohs(net_sin(&dst)->sin_port));
	} else {
		if (src.sa_family == AF_INET && net_sin(&src)->sin_port != 0) {
			net_addr_ntop(src.sa_family,
				      &net_sin(&src)->sin_addr,
				      src_addr_str, sizeof(src_addr_str));
			/* <mode>: In the UDP Wi-Fi passthrough
			 *
			 * 0: After UDP data is received, the parameters <"remote host">
			 * and <remote port> will stay unchanged (default).
			 * 1: Only the first time that UDP data is received from an IP
			 * address and port that are different from the initially set
			 * value of parameters <remote host> and <remote port>, will
			 * they be changed to the IP address and port of the device that
			 * sends the data.
			 * 2: Each time UDP data is received, the <"remote host"> and
			 * <remote port> will be changed to the IP address and port of the
			 * device that sends the data.
			 *
			 * When remote IP and port are both 0, it means that the socket is
			 * being used as a server, and you need to set the connection mode
			 * to 2.
			 */
			if ((net_sin(&dst)->sin_addr.s_addr == 0) &&
			    (ntohs(net_sin(&dst)->sin_port) == 0)) {
				mode = 2;
				/* Port 0 is reserved and a valid port needs to be provided when
				 * connecting.
				 */
				net_sin(&dst)->sin_port = 65535;
			} else {
				mode = 0;
			}

			snprintk(connect_msg, sizeof(connect_msg),
				 "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d,%d,\"%s\"",
				 sock->link_id, dst_addr_str,
				 ntohs(net_sin(&dst)->sin_port), ntohs(net_sin(&src)->sin_port),
				 mode, src_addr_str);
		} else {
			snprintk(connect_msg, sizeof(connect_msg),
				 "AT+CIPSTART=%d,\"UDP\",\"%s\",%d",
				 sock->link_id, dst_addr_str,
				 ntohs(net_sin(&dst)->sin_port));
		}
	}

	LOG_DBG("link %d, ip_proto %s, addr %s", sock->link_id,
		esp_socket_ip_proto(sock) == IPPROTO_TCP ? "TCP" : "UDP",
		dst_addr_str);

	ret = esp_cmd_send(dev, NULL, 0, connect_msg, ESP_CMD_TIMEOUT);
	if (ret == 0) {
		esp_socket_flags_set(sock, ESP_SOCK_CONNECTED);
		if (esp_socket_type(sock) == SOCK_STREAM) {
			net_context_set_state(sock->context,
					      NET_CONTEXT_CONNECTED);
		}
	} else if (ret == -ETIMEDOUT) {
		/* FIXME:
		 * What if the connection finishes after we return from
		 * here? The caller might think that it can discard the
		 * socket. Set some flag to indicate that the link should
		 * be closed if it ever connects?
		 */
	}

	return ret;
}

void esp_connect_work(struct k_work *work)
{
	struct esp_socket *sock = CONTAINER_OF(work, struct esp_socket,
					       connect_work);
	struct esp_data *dev = esp_socket_to_dev(sock);
	int ret;

	ret = _sock_connect(dev, sock);

	k_mutex_lock(&sock->lock, K_FOREVER);
	if (sock->connect_cb) {
		sock->connect_cb(sock->context, ret, sock->conn_user_data);
	}
	k_mutex_unlock(&sock->lock);
}

static int esp_bind(struct net_context *context, const struct sockaddr *addr,
		    socklen_t addrlen)
{
	struct esp_socket *sock;
	struct esp_data *dev;

	sock = (struct esp_socket *)context->offload_context;
	dev = esp_socket_to_dev(sock);

	if (esp_socket_ip_proto(sock) == IPPROTO_TCP) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		LOG_DBG("link %d", sock->link_id);

		if (esp_socket_connected(sock)) {
			return -EISCONN;
		}

		k_mutex_lock(&sock->lock, K_FOREVER);
		sock->src = *addr;
		k_mutex_unlock(&sock->lock);

		return 0;
	}

	return -EAFNOSUPPORT;
}

static int esp_connect(struct net_context *context,
		       const struct sockaddr *addr,
		       socklen_t addrlen,
		       net_context_connect_cb_t cb,
		       int32_t timeout,
		       void *user_data)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	int ret;

	sock = (struct esp_socket *)context->offload_context;
	dev = esp_socket_to_dev(sock);

	LOG_DBG("link %d, timeout %d", sock->link_id, timeout);

	if (!IS_ENABLED(CONFIG_NET_IPV4) || addr->sa_family != AF_INET) {
		return -EAFNOSUPPORT;
	}

	if (esp_socket_connected(sock)) {
		return -EISCONN;
	}

	k_mutex_lock(&sock->lock, K_FOREVER);
	sock->dst = *addr;
	sock->connect_cb = cb;
	sock->conn_user_data = user_data;
	k_mutex_unlock(&sock->lock);

	if (timeout == 0) {
		esp_socket_work_submit(sock, &sock->connect_work);
		return 0;
	}

	ret = _sock_connect(dev, sock);

	if (ret != -ETIMEDOUT && cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int esp_accept(struct net_context *context,
			     net_tcp_accept_cb_t cb, int32_t timeout,
			     void *user_data)
{
	return -ENOTSUP;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_tx_ready)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	k_sem_give(&dev->sem_tx_ready);
	return len;
}

MODEM_CMD_DEFINE(on_cmd_send_ok)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&dev->sem_response);

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_send_fail)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&dev->sem_response);

	return 0;
}

static int _sock_send(struct esp_socket *sock, struct net_pkt *pkt)
{
	struct esp_data *dev = esp_socket_to_dev(sock);
	char cmd_buf[sizeof("AT+CIPSEND=0,,\"\",") +
		     sizeof(STRINGIFY(ESP_MTU)) - 1 +
		     NET_IPV4_ADDR_LEN + sizeof("65535") - 1];
	char addr_str[NET_IPV4_ADDR_LEN];
	int ret, write_len, pkt_len;
	struct net_buf *frag;
	static const struct modem_cmd cmds[] = {
		MODEM_CMD_DIRECT(">", on_cmd_tx_ready),
		MODEM_CMD("SEND OK", on_cmd_send_ok, 0U, ""),
		MODEM_CMD("SEND FAIL", on_cmd_send_fail, 0U, ""),
	};
	struct sockaddr dst;

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED | EDF_AP_ENABLED)) {
		return -ENETUNREACH;
	}

	pkt_len = net_pkt_get_len(pkt);

	LOG_DBG("link %d, len %d", sock->link_id, pkt_len);

	if (esp_socket_ip_proto(sock) == IPPROTO_TCP) {
		snprintk(cmd_buf, sizeof(cmd_buf),
			 "AT+CIPSEND=%d,%d", sock->link_id, pkt_len);
	} else {
		k_mutex_lock(&sock->lock, K_FOREVER);
		dst = sock->dst;
		k_mutex_unlock(&sock->lock);

		net_addr_ntop(dst.sa_family,
			      &net_sin(&dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		snprintk(cmd_buf, sizeof(cmd_buf),
			 "AT+CIPSEND=%d,%d,\"%s\",%d",
			 sock->link_id, pkt_len, addr_str,
			 ntohs(net_sin(&dst)->sin_port));
	}

	k_sem_take(&dev->cmd_handler_data.sem_tx_lock, K_FOREVER);
	k_sem_reset(&dev->sem_tx_ready);

	ret = modem_cmd_send_ext(&dev->mctx.iface, &dev->mctx.cmd_handler,
				 cmds, ARRAY_SIZE(cmds), cmd_buf,
				 &dev->sem_response, ESP_CMD_TIMEOUT,
				 MODEM_NO_TX_LOCK | MODEM_NO_UNSET_CMDS);
	if (ret < 0) {
		LOG_DBG("Failed to send command");
		goto out;
	}

	/* Reset semaphore that will be released by 'SEND OK' or 'SEND FAIL' */
	k_sem_reset(&dev->sem_response);

	/* Wait for '>' */
	ret = k_sem_take(&dev->sem_tx_ready, K_MSEC(5000));
	if (ret < 0) {
		LOG_DBG("Timeout waiting for tx");
		goto out;
	}

	frag = pkt->frags;
	while (frag && pkt_len) {
		write_len = MIN(pkt_len, frag->len);
		modem_cmd_send_data_nolock(&dev->mctx.iface, frag->data, write_len);
		pkt_len -= write_len;
		frag = frag->frags;
	}

	/* Wait for 'SEND OK' or 'SEND FAIL' */
	if (esp_socket_ip_proto(sock) == IPPROTO_TCP) {
		ret = k_sem_take(&dev->sem_response, TCP_SEND_TIMEOUT);
	} else {
		ret = k_sem_take(&dev->sem_response, ESP_CMD_TIMEOUT);
	}
	if (ret < 0) {
		LOG_DBG("No send response");
		goto out;
	}

	ret = modem_cmd_handler_get_error(&dev->cmd_handler_data);
	if (ret != 0) {
		LOG_DBG("Failed to send data");
	}

out:
	(void)modem_cmd_handler_update_cmds(&dev->cmd_handler_data,
					    NULL, 0U, false);
	k_sem_give(&dev->cmd_handler_data.sem_tx_lock);

	return ret;
}

static bool esp_socket_can_send(struct esp_socket *sock)
{
	atomic_val_t flags = esp_socket_flags(sock);

	if ((flags & ESP_SOCK_CONNECTED) && !(flags & ESP_SOCK_CLOSE_PENDING)) {
		return true;
	}

	return false;
}

static int esp_socket_send_one_pkt(struct esp_socket *sock)
{
	struct net_context *context = sock->context;
	struct net_pkt *pkt;
	int ret;

	pkt = k_fifo_get(&sock->tx_fifo, K_NO_WAIT);
	if (!pkt) {
		return -ENOMSG;
	}

	if (!esp_socket_can_send(sock)) {
		goto pkt_unref;
	}

	ret = _sock_send(sock, pkt);
	if (ret < 0) {
		LOG_ERR("Failed to send data: link %d, ret %d",
			sock->link_id, ret);

		/*
		 * If this is stream data, then we should stop pushing anything
		 * more to this socket, as there will be a hole in the data
		 * stream, which application layer is not expecting.
		 */
		if (esp_socket_type(sock) == SOCK_STREAM) {
			if (!esp_socket_flags_test_and_set(sock,
						ESP_SOCK_CLOSE_PENDING)) {
				esp_socket_work_submit(sock, &sock->close_work);
			}
		}
	} else if (context->send_cb) {
		context->send_cb(context, ret, context->user_data);
	}

pkt_unref:
	net_pkt_unref(pkt);

	return 0;
}

void esp_send_work(struct k_work *work)
{
	struct esp_socket *sock = CONTAINER_OF(work, struct esp_socket,
					       send_work);
	int err;

	do {
		err = esp_socket_send_one_pkt(sock);
	} while (err != -ENOMSG);
}

static int esp_sendto(struct net_pkt *pkt,
		      const struct sockaddr *dst_addr,
		      socklen_t addrlen,
		      net_context_send_cb_t cb,
		      int32_t timeout,
		      void *user_data)
{
	struct net_context *context;
	struct esp_socket *sock;
	struct esp_data *dev;
	int ret = 0;

	context = pkt->context;
	sock = (struct esp_socket *)context->offload_context;
	dev = esp_socket_to_dev(sock);

	LOG_DBG("link %d, timeout %d", sock->link_id, timeout);

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED | EDF_AP_ENABLED)) {
		return -ENETUNREACH;
	}

	if (esp_socket_type(sock) == SOCK_STREAM) {
		atomic_val_t flags = esp_socket_flags(sock);

		if (!(flags & ESP_SOCK_CONNECTED) ||
		     (flags & ESP_SOCK_CLOSE_PENDING)) {
			return -ENOTCONN;
		}
	} else {
		if (!esp_socket_connected(sock)) {
			if (!dst_addr) {
				return -ENOTCONN;
			}

			/* Use a timeout of 5000 ms here even though the
			 * timeout parameter might be different. We want to
			 * have a valid link id before proceeding.
			 */
			ret = esp_connect(context, dst_addr, addrlen, NULL,
					  (5 * MSEC_PER_SEC), NULL);
			if (ret < 0) {
				return ret;
			}
		} else if (esp_socket_type(sock) == SOCK_DGRAM) {
			memcpy(&sock->dst, dst_addr, addrlen);
		}
	}

	return esp_socket_queue_tx(sock, pkt);
}

static int esp_send(struct net_pkt *pkt,
		    net_context_send_cb_t cb,
		    int32_t timeout,
		    void *user_data)
{
	return esp_sendto(pkt, NULL, 0, cb, timeout, user_data);
}

#define CIPRECVDATA_CMD_MIN_LEN (sizeof("+CIPRECVDATA,L:") - 1)

#if defined(CONFIG_WIFI_ESP_AT_CIPDINFO_USE) && !defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define CIPRECVDATA_CMD_MAX_LEN (sizeof("+CIPRECVDATA,LLLL,\"255.255.255.255\",65535:") - 1)
#else
#define CIPRECVDATA_CMD_MAX_LEN (sizeof("+CIPRECVDATA,LLLL:") - 1)
#endif

static int cmd_ciprecvdata_parse(struct esp_socket *sock,
				 struct net_buf *buf, uint16_t len,
				 int *data_offset, int *data_len, char *ip_str,
				 int *port)
{
	char cmd_buf[CIPRECVDATA_CMD_MAX_LEN + 1];
	char *endptr;
	size_t frags_len;
	size_t match_len;

	frags_len = net_buf_frags_len(buf);
	if (frags_len < CIPRECVDATA_CMD_MIN_LEN) {
		return -EAGAIN;
	}

	match_len = net_buf_linearize(cmd_buf, CIPRECVDATA_CMD_MAX_LEN,
				      buf, 0, CIPRECVDATA_CMD_MAX_LEN);
	cmd_buf[match_len] = 0;

	*data_len = strtol(&cmd_buf[len], &endptr, 10);

#if defined(CONFIG_WIFI_ESP_AT_CIPDINFO_USE) && !defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
	char *strstart = endptr + 1;
	char *strend = strchr(strstart, ',');

	if (strstart == NULL || strend == NULL) {
		return -EAGAIN;
	}

	memcpy(ip_str, strstart, strend - strstart);
	ip_str[strend - strstart] = '\0';
	*port = strtol(strend + 1, &endptr, 10);
#else
	ARG_UNUSED(ip_str);
	ARG_UNUSED(port);
#endif

	if (endptr == &cmd_buf[len] ||
	    (*endptr == 0 && match_len >= CIPRECVDATA_CMD_MAX_LEN) ||
	    *data_len > CIPRECVDATA_MAX_LEN) {
		LOG_ERR("Invalid cmd: %s", cmd_buf);
		return -EBADMSG;
	} else if (*endptr == 0) {
		return -EAGAIN;
	} else if (*endptr != _CIPRECVDATA_END) {
		LOG_ERR("Invalid end of cmd: 0x%02x != 0x%02x", *endptr,
			_CIPRECVDATA_END);
		return -EBADMSG;
	}

	/* data_offset is the offset to where the actual data starts */
	*data_offset = (endptr - cmd_buf) + 1;

	/* FIXME: Inefficient way of waiting for data */
	if (*data_offset + *data_len > frags_len) {
		return -EAGAIN;
	}

	*endptr = 0;

	return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_ciprecvdata)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct esp_socket *sock;
	int data_offset, data_len;
	int err;

	sock = esp_socket_ref(dev->rx_sock);
	if (!sock) {
		LOG_ERR("No rx_sock socket");
		return -ENOTCONN;
	}

#if defined(CONFIG_WIFI_ESP_AT_CIPDINFO_USE) && !defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
	char raw_remote_ip[INET_ADDRSTRLEN + 3] = {0};
	int port = 0;

	err = cmd_ciprecvdata_parse(sock, data->rx_buf, len, &data_offset,
				    &data_len, raw_remote_ip, &port);
#else
	err = cmd_ciprecvdata_parse(sock, data->rx_buf, len, &data_offset,
				    &data_len, NULL, NULL);
#endif
	if (err) {
		if (err == -EAGAIN) {
			goto socket_unref;
		}

		goto socket_unref;
	}

#if defined(CONFIG_WIFI_ESP_AT_CIPDINFO_USE) && !defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
	struct sockaddr_in *recv_addr =
			(struct sockaddr_in *) &sock->context->remote;

	recv_addr->sin_port = htons(port);
	recv_addr->sin_family = AF_INET;

	/* IP addr comes within quotation marks, which is disliked by
	 * conv function. So we remove them by subtraction 2 from
	 * raw_remote_ip length and index from &raw_remote_ip[1].
	 */
	char remote_ip_addr[INET_ADDRSTRLEN];
	size_t remote_ip_str_len;

	remote_ip_str_len = MIN(sizeof(remote_ip_addr) - 1,
				strlen(raw_remote_ip) - 2);
	strncpy(remote_ip_addr, &raw_remote_ip[1], remote_ip_str_len);
	remote_ip_addr[remote_ip_str_len] = '\0';

	if (net_addr_pton(AF_INET, remote_ip_addr, &recv_addr->sin_addr) < 0) {
		LOG_ERR("Invalid src addr %s", remote_ip_addr);
		err = -EIO;
		goto socket_unref;
	}
#endif
	esp_socket_rx(sock, data->rx_buf, data_offset, data_len);

	err = data_offset + data_len;
	goto socket_unref;

socket_unref:
	esp_socket_unref(sock);

	return err;
}

void esp_recvdata_work(struct k_work *work)
{
	struct esp_socket *sock = CONTAINER_OF(work, struct esp_socket,
					       recvdata_work);
	struct esp_data *data = esp_socket_to_dev(sock);
	char cmd[sizeof("AT+CIPRECVDATA=000,"STRINGIFY(CIPRECVDATA_MAX_LEN))];
	static const struct modem_cmd cmds[] = {
		MODEM_CMD_DIRECT(_CIPRECVDATA, on_cmd_ciprecvdata),
	};
	int ret;

	LOG_DBG("reading available data on link %d", sock->link_id);

	data->rx_sock = sock;

	snprintk(cmd, sizeof(cmd), "AT+CIPRECVDATA=%d,%d", sock->link_id,
		 CIPRECVDATA_MAX_LEN);

	ret = esp_cmd_send(data, cmds, ARRAY_SIZE(cmds), cmd, ESP_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Error during rx: link %d, ret %d", sock->link_id,
			ret);
	}
}

void esp_close_work(struct k_work *work)
{
	struct esp_socket *sock = CONTAINER_OF(work, struct esp_socket,
					       close_work);
	atomic_val_t old_flags;

	old_flags = esp_socket_flags_clear(sock,
				(ESP_SOCK_CONNECTED | ESP_SOCK_CLOSE_PENDING));

	if ((old_flags & ESP_SOCK_CONNECTED) &&
	    (old_flags & ESP_SOCK_CLOSE_PENDING)) {
		esp_socket_close(sock);
	}

	/* Should we notify that the socket has been closed? */
	if (old_flags & ESP_SOCK_CLOSE_PENDING) {
		k_mutex_lock(&sock->lock, K_FOREVER);
		if (sock->recv_cb) {
			sock->recv_cb(sock->context, NULL, NULL, NULL, 0,
				      sock->recv_user_data);
			k_sem_give(&sock->sem_data_ready);
		}
		k_mutex_unlock(&sock->lock);
	}
}

static int esp_recv(struct net_context *context,
		    net_context_recv_cb_t cb,
		    int32_t timeout,
		    void *user_data)
{
	struct esp_socket *sock = context->offload_context;
	struct esp_data *dev = esp_socket_to_dev(sock);
	int ret;

	LOG_DBG("link_id %d, timeout %d, cb %p, data %p",
		sock->link_id, timeout, cb, user_data);

	/*
	 * UDP "listening" socket needs to be bound using AT+CIPSTART before any
	 * traffic can be received.
	 */
	if (!esp_socket_connected(sock) &&
	    esp_socket_ip_proto(sock) == IPPROTO_UDP &&
	    sock->src.sa_family == AF_INET &&
	    net_sin(&sock->src)->sin_port != 0) {
		_sock_connect(dev, sock);
	}

	k_mutex_lock(&sock->lock, K_FOREVER);
	sock->recv_cb = cb;
	sock->recv_user_data = user_data;
	k_sem_reset(&sock->sem_data_ready);
	k_mutex_unlock(&sock->lock);

	if (timeout == 0) {
		return 0;
	}

	ret = k_sem_take(&sock->sem_data_ready, K_MSEC(timeout));

	k_mutex_lock(&sock->lock, K_FOREVER);
	sock->recv_cb = NULL;
	sock->recv_user_data = NULL;
	k_mutex_unlock(&sock->lock);

	return ret;
}

static int esp_put(struct net_context *context)
{
	struct esp_socket *sock = context->offload_context;

	esp_socket_workq_stop_and_flush(sock);

	if (esp_socket_flags_test_and_clear(sock, ESP_SOCK_CONNECTED)) {
		esp_socket_close(sock);
	}

	k_mutex_lock(&sock->lock, K_FOREVER);
	sock->connect_cb = NULL;
	sock->recv_cb = NULL;
	k_mutex_unlock(&sock->lock);

	k_sem_reset(&sock->sem_free);

	esp_socket_unref(sock);

	/*
	 * Let's get notified when refcount reaches 0. Call to
	 * esp_socket_unref() in this function might or might not be the last
	 * one. The reason is that there might be still some work in progress in
	 * esp_rx thread (parsing unsolicited AT command), so we want to wait
	 * until it finishes.
	 */
	k_sem_take(&sock->sem_free, K_FOREVER);

	sock->context = NULL;

	esp_socket_put(sock);

	return 0;
}

static int esp_get(sa_family_t family,
		   enum net_sock_type type,
		   enum net_ip_protocol ip_proto,
		   struct net_context **context)
{
	struct esp_socket *sock;
	struct esp_data *dev;

	LOG_DBG("");

	if (family != AF_INET) {
		return -EAFNOSUPPORT;
	}

	/* FIXME:
	 * iface has not yet been assigned to context so there is currently
	 * no way to know which interface to operate on. Therefore this driver
	 * only supports one device node.
	 */
	dev = &esp_driver_data;

	sock = esp_socket_get(dev, *context);
	if (!sock) {
		LOG_ERR("No socket available!");
		return -ENOMEM;
	}

	return 0;
}

static struct net_offload esp_offload = {
	.get	       = esp_get,
	.bind	       = esp_bind,
	.listen	       = esp_listen,
	.connect       = esp_connect,
	.accept	       = esp_accept,
	.send	       = esp_send,
	.sendto	       = esp_sendto,
	.recv	       = esp_recv,
	.put	       = esp_put,
};

int esp_offload_init(struct net_if *iface)
{
	iface->if_dev->offload = &esp_offload;

	return 0;
}
