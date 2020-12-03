/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_esp_offload, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_offload.h>

#include "esp.h"

static int esp_bind(struct net_context *context, const struct sockaddr *addr,
		    socklen_t addrlen)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		return 0;
	}

	return -EAFNOSUPPORT;
}

static int esp_listen(struct net_context *context, int backlog)
{
	return -ENOTSUP;
}

static int _sock_connect(struct esp_data *dev, struct esp_socket *sock)
{
	char connect_msg[sizeof("AT+CIPSTART=0,\"TCP\",\"\",65535,7200") +
			 NET_IPV4_ADDR_LEN];
	char addr_str[NET_IPV4_ADDR_LEN];
	int ret;

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		return -ENETUNREACH;
	}

	if (sock->ip_proto == IPPROTO_TCP) {
		net_addr_ntop(sock->dst.sa_family,
			      &net_sin(&sock->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		snprintk(connect_msg, sizeof(connect_msg),
			 "AT+CIPSTART=%d,\"TCP\",\"%s\",%d,7200",
			 sock->link_id, addr_str,
			 ntohs(net_sin(&sock->dst)->sin_port));
	} else {
		net_addr_ntop(sock->dst.sa_family,
			      &net_sin(&sock->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		snprintk(connect_msg, sizeof(connect_msg),
			 "AT+CIPSTART=%d,\"UDP\",\"%s\",%d",
			 sock->link_id, addr_str,
			 ntohs(net_sin(&sock->dst)->sin_port));
	}

	LOG_DBG("link %d, ip_proto %s, addr %s", sock->link_id,
		sock->ip_proto == IPPROTO_TCP ? "TCP" : "UDP",
		log_strdup(addr_str));

	ret = esp_cmd_send(dev, NULL, 0, connect_msg, ESP_CMD_TIMEOUT);
	if (ret == 0) {
		sock->flags |= ESP_SOCK_CONNECTED;
		if (sock->type == SOCK_STREAM) {
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

static void esp_connect_work(struct k_work *work)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	int ret;

	sock = CONTAINER_OF(work, struct esp_socket, connect_work);
	dev = esp_socket_to_dev(sock);

	if (!esp_socket_in_use(sock)) {
		LOG_DBG("Socket %d not in use", sock->idx);
		return;
	}

	ret = _sock_connect(dev, sock);

	if (sock->connect_cb) {
		sock->connect_cb(sock->context, ret, sock->conn_user_data);
	}

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

	sock->dst = *addr;
	sock->connect_cb = cb;
	sock->conn_user_data = user_data;

	if (timeout == 0) {
		k_work_submit_to_queue(&dev->workq, &sock->connect_work);
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

static int _sock_send(struct esp_data *dev, struct esp_socket *sock)
{
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

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		return -ENETUNREACH;
	}

	pkt_len = net_pkt_get_len(sock->tx_pkt);

	LOG_DBG("link %d, len %d", sock->link_id, pkt_len);

	if (sock->ip_proto == IPPROTO_TCP) {
		snprintk(cmd_buf, sizeof(cmd_buf),
			 "AT+CIPSEND=%d,%d", sock->link_id, pkt_len);
	} else {
		net_addr_ntop(sock->dst.sa_family,
			      &net_sin(&sock->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		snprintk(cmd_buf, sizeof(cmd_buf),
			 "AT+CIPSEND=%d,%d,\"%s\",%d",
			 sock->link_id, pkt_len, addr_str,
			 ntohs(net_sin(&sock->dst)->sin_port));
	}

	k_sem_take(&dev->cmd_handler_data.sem_tx_lock, K_FOREVER);
	k_sem_reset(&dev->sem_tx_ready);

	ret = modem_cmd_send_nolock(&dev->mctx.iface, &dev->mctx.cmd_handler,
				    cmds, ARRAY_SIZE(cmds), cmd_buf,
				    &dev->sem_response, ESP_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("Failed to send command");
		goto out;
	}

	ret = modem_cmd_handler_update_cmds(&dev->cmd_handler_data,
					    cmds, ARRAY_SIZE(cmds),
					    true);
	if (ret < 0) {
		goto out;
	}

	/*
	 * After modem handlers have been updated the receive buffer
	 * needs to be processed again since there might now be a match.
	 */
	k_sem_give(&dev->iface_data.rx_sem);

	/* Wait for '>' */
	ret = k_sem_take(&dev->sem_tx_ready, K_MSEC(5000));
	if (ret < 0) {
		LOG_DBG("Timeout waiting for tx");
		goto out;
	}

	frag = sock->tx_pkt->frags;
	while (frag && pkt_len) {
		write_len = MIN(pkt_len, frag->len);
		dev->mctx.iface.write(&dev->mctx.iface, frag->data, write_len);
		pkt_len -= write_len;
		frag = frag->frags;
	}

	/* Wait for 'SEND OK' or 'SEND FAIL' */
	k_sem_reset(&dev->sem_response);
	ret = k_sem_take(&dev->sem_response, ESP_CMD_TIMEOUT);
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

static void esp_send_work(struct k_work *work)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	int ret = 0;

	sock = CONTAINER_OF(work, struct esp_socket, send_work);
	dev = esp_socket_to_dev(sock);

	if (!esp_socket_in_use(sock)) {
		LOG_DBG("Socket %d not in use", sock->idx);
		return;
	}

	ret = _sock_send(dev, sock);
	if (ret < 0) {
		LOG_ERR("Failed to send data: link %d, ret %d", sock->link_id,
			ret);
	}

	net_pkt_unref(sock->tx_pkt);
	sock->tx_pkt = NULL;

	if (sock->send_cb) {
		sock->send_cb(sock->context, ret, sock->send_user_data);
	}
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

	if (!esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		return -ENETUNREACH;
	}

	if (sock->tx_pkt) {
		return -EBUSY;
	}

	if (sock->type == SOCK_STREAM) {
		if (!esp_socket_connected(sock)) {
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
		} else if (dst_addr && memcmp(dst_addr, &sock->dst, addrlen)) {
			/* This might be unexpected behaviour but the ESP
			 * doesn't support changing endpoint.
			 */
			return -EISCONN;
		}
	}

	sock->tx_pkt = pkt;
	sock->send_cb = cb;
	sock->send_user_data = user_data;

	if (timeout == 0) {
		k_work_submit_to_queue(&dev->workq, &sock->send_work);
		return 0;
	}

	ret = _sock_send(dev, sock);
	if (ret == 0) {
		net_pkt_unref(sock->tx_pkt);
	} else {
		LOG_ERR("Failed to send data: link %d, ret %d", sock->link_id,
			ret);
	}

	sock->tx_pkt = NULL;

	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int esp_send(struct net_pkt *pkt,
		    net_context_send_cb_t cb,
		    int32_t timeout,
		    void *user_data)
{
	return esp_sendto(pkt, NULL, 0, cb, timeout, user_data);
}

#define CIPRECVDATA_CMD_MIN_LEN (sizeof("+CIPRECVDATA,L:") - 1)
#define CIPRECVDATA_CMD_MAX_LEN (sizeof("+CIPRECVDATA,LLLL:") - 1)

static int cmd_ciprecvdata_parse(struct esp_socket *sock,
				 struct net_buf *buf, uint16_t len,
				 int *data_offset, int *data_len)
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
	if (endptr == &cmd_buf[len] ||
	    (*endptr == 0 && match_len >= CIPRECVDATA_CMD_MAX_LEN) ||
	    *data_len > sock->bytes_avail) {
		LOG_ERR("Invalid cmd: %s", log_strdup(cmd_buf));
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
	struct esp_socket *sock = dev->rx_sock;
	int data_offset, data_len;
	int err;

	err = cmd_ciprecvdata_parse(sock, data->rx_buf, len, &data_offset,
				    &data_len);
	if (err) {
		if (err == -EAGAIN) {
			return -EAGAIN;
		}

		return err;
	}

	sock->bytes_avail -= data_len;

	esp_socket_rx(sock, data->rx_buf, data_offset, data_len);

	return data_offset + data_len;
}

static void esp_recvdata_work(struct k_work *work)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	int len = CIPRECVDATA_MAX_LEN, ret;
	char cmd[sizeof("AT+CIPRECVDATA=0,"STRINGIFY(CIPRECVDATA_MAX_LEN))];
	static const struct modem_cmd cmds[] = {
		MODEM_CMD_DIRECT(_CIPRECVDATA, on_cmd_ciprecvdata),
	};

	sock = CONTAINER_OF(work, struct esp_socket, recvdata_work);
	dev = esp_socket_to_dev(sock);

	if (!esp_socket_in_use(sock)) {
		LOG_DBG("Socket %d not in use", sock->idx);
		return;
	}

	LOG_DBG("%d bytes available on link %d", sock->bytes_avail,
		sock->link_id);

	if (sock->bytes_avail == 0) {
		LOG_WRN("No data available on link %d", sock->link_id);
		return;
	} else if (len > sock->bytes_avail) {
		len = sock->bytes_avail;
	}

	dev->rx_sock = sock;

	snprintk(cmd, sizeof(cmd), "AT+CIPRECVDATA=%d,%d", sock->link_id, len);

	ret = esp_cmd_send(dev, cmds, ARRAY_SIZE(cmds), cmd, ESP_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Error during rx: link %d, ret %d", sock->link_id,
			ret);
	} else if (sock->bytes_avail > 0) {
		k_work_submit_to_queue(&dev->workq, &sock->recvdata_work);
	}
}


static void esp_recv_work(struct k_work *work)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	struct net_pkt *pkt;

	sock = CONTAINER_OF(work, struct esp_socket, recv_work);
	dev = esp_socket_to_dev(sock);

	if (!esp_socket_in_use(sock)) {
		LOG_DBG("Socket %d not in use", sock->idx);
		return;
	}

	pkt = k_fifo_get(&sock->fifo_rx_pkt, K_NO_WAIT);
	while (pkt) {
		if (sock->recv_cb) {
			sock->recv_cb(sock->context, pkt, NULL, NULL,
				      0, sock->recv_user_data);
			k_sem_give(&sock->sem_data_ready);
		} else {
			/* Discard */
			net_pkt_unref(pkt);
		}

		pkt = k_fifo_get(&sock->fifo_rx_pkt, K_NO_WAIT);
	}

	if (esp_socket_close_pending(sock)) {
		if (esp_socket_connected(sock)) {
			esp_socket_close(sock);
		}

		sock->flags &= ~(ESP_SOCK_CONNECTED | ESP_SOCK_CLOSE_PENDING);
	}

	/* Should we notify that the socket has been closed? */
	if (!esp_socket_connected(sock) && sock->bytes_avail == 0 &&
	    sock->recv_cb) {
		sock->recv_cb(sock->context, NULL, NULL, NULL, 0,
			      sock->recv_user_data);
		k_sem_give(&sock->sem_data_ready);
	}
}

static int esp_recv(struct net_context *context,
		    net_context_recv_cb_t cb,
		    int32_t timeout,
		    void *user_data)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	int ret;

	sock = (struct esp_socket *)context->offload_context;
	dev = esp_socket_to_dev(sock);

	LOG_DBG("link_id %d, timeout %d, cb 0x%x, data 0x%x", sock->link_id,
		timeout, (int)cb, (int)user_data);

	sock->recv_cb = cb;
	sock->recv_user_data = user_data;
	k_sem_reset(&sock->sem_data_ready);

	if (timeout == 0) {
		return 0;
	}

	ret = k_sem_take(&sock->sem_data_ready, K_MSEC(timeout));

	sock->recv_cb = NULL;

	return ret;
}

static int esp_put(struct net_context *context)
{
	struct esp_socket *sock;
	struct net_pkt *pkt;

	sock = (struct esp_socket *)context->offload_context;

	if (esp_socket_connected(sock)) {
		esp_socket_close(sock);
	}

	sock->connect_cb = NULL;
	sock->recv_cb = NULL;
	sock->send_cb = NULL;
	sock->tx_pkt = NULL;

	/* Drain rxfifo */
	for (pkt = k_fifo_get(&sock->fifo_rx_pkt, K_NO_WAIT);
	     pkt != NULL;
	     pkt = k_fifo_get(&sock->fifo_rx_pkt, K_NO_WAIT)) {
		net_pkt_unref(pkt);
	}

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

	sock = esp_socket_get(dev);
	if (sock == NULL) {
		return -ENOMEM;
	}

	k_work_init(&sock->connect_work, esp_connect_work);
	k_work_init(&sock->send_work, esp_send_work);
	k_work_init(&sock->recv_work, esp_recv_work);
	k_work_init(&sock->recvdata_work, esp_recvdata_work);
	sock->type = type;
	sock->ip_proto = ip_proto;
	sock->context = *context;
	(*context)->offload_context = sock;

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
