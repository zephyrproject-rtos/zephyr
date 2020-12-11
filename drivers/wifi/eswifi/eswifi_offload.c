/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
#include "eswifi_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <net/net_pkt.h>
#include <net/net_if.h>

#include "eswifi.h"


static int eswifi_off_bind(struct net_context *context,
			   const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;

	LOG_DBG("");
	eswifi_lock(eswifi);
	err = __eswifi_bind(eswifi, socket, addr, addrlen);
	eswifi_unlock(eswifi);

	return err;
}

static int eswifi_off_listen(struct net_context *context, int backlog)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;

	LOG_DBG("Listening backlog=%d", backlog);

	eswifi_lock(eswifi);

	__select_socket(eswifi, socket->index);

	/* Set backlog */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P8=%d\r", backlog);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to start set listen backlog");
		err = -EIO;
	}

	socket->is_server = true;

	eswifi_unlock(eswifi);

	return err;
}

static void eswifi_off_connect_work(struct k_work *work)
{
	struct eswifi_off_socket *socket;
	net_context_connect_cb_t cb;
	struct net_context *context;
	struct eswifi_dev *eswifi;
	void *user_data;
	int err;

	socket = CONTAINER_OF(work, struct eswifi_off_socket, connect_work);
	eswifi = eswifi_socket_to_dev(socket);

	eswifi_lock(eswifi);

	cb = socket->conn_cb;
	context = socket->context;
	user_data = socket->conn_data;

	err = __eswifi_off_start_client(eswifi, socket);
	if (!err) {
		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
		net_context_set_state(socket->context, NET_CONTEXT_CONNECTED);
	} else {
		socket->state = ESWIFI_SOCKET_STATE_NONE;
	}

	eswifi_unlock(eswifi);

	if (cb) {
		cb(context, err, user_data);
	}
}

static int eswifi_off_connect(struct net_context *context,
			      const struct sockaddr *addr,
			      socklen_t addrlen,
			      net_context_connect_cb_t cb,
			      int32_t timeout,
			      void *user_data)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;

	LOG_DBG("timeout=%d", timeout);

	if (addr->sa_family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	eswifi_lock(eswifi);

	if (socket->state != ESWIFI_SOCKET_STATE_NONE) {
		eswifi_unlock(eswifi);
		return -EBUSY;
	}

	socket->peer_addr = *addr;
	socket->conn_data = user_data;
	socket->conn_cb = cb;
	socket->state = ESWIFI_SOCKET_STATE_CONNECTING;

	if (timeout == 0) {
		/* async */
		k_work_submit_to_queue(&eswifi->work_q, &socket->connect_work);
		eswifi_unlock(eswifi);
		return 0;
	}

	err = __eswifi_off_start_client(eswifi, socket);
	if (!err) {
		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
	} else {
		socket->state = ESWIFI_SOCKET_STATE_NONE;
	}

	eswifi_unlock(eswifi);

	if (cb) {
		cb(context, err, user_data);
	}

	return err;
}

static int eswifi_off_accept(struct net_context *context,
			     net_tcp_accept_cb_t cb, int32_t timeout,
			     void *user_data)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int ret;

	eswifi_lock(eswifi);

	ret = __eswifi_accept(eswifi, socket);
	if (ret < 0) {
		eswifi_unlock(eswifi);
		return ret;
	}

	socket->accept_cb = cb;
	socket->accept_data = user_data;
	k_sem_reset(&socket->accept_sem);

	eswifi_unlock(eswifi);

	if (timeout == 0) {
		return 0;
	}

	return k_sem_take(&socket->accept_sem, K_MSEC(timeout));
}

static int __eswifi_off_send_pkt(struct eswifi_dev *eswifi,
				 struct eswifi_off_socket *socket)
{
	struct net_pkt *pkt = socket->tx_pkt;
	unsigned int bytes;
	int err, offset;

	LOG_DBG("");

	if (!pkt) {
		return -EINVAL;
	}

	bytes = net_pkt_get_len(pkt);

	__select_socket(eswifi, socket->index);

	/* header */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "S3=%u\r", bytes);
	offset = strlen(eswifi->buf);

	/* copy payload */
	if (net_pkt_read(pkt, &eswifi->buf[offset], bytes)) {
		return -ENOBUFS;
	}

	offset += bytes;

	err = eswifi_request(eswifi, eswifi->buf, offset + 1,
			     eswifi->buf, sizeof(eswifi->buf));
	if (err < 0) {
		LOG_ERR("Unable to send data");
		return -EIO;
	}

	net_pkt_unref(pkt);

	return 0;
}

static void eswifi_off_send_work(struct k_work *work)
{
	struct eswifi_off_socket *socket;
	net_context_send_cb_t cb;
	struct net_context *context;
	struct eswifi_dev *eswifi;
	void *user_data;
	int err;

	socket = CONTAINER_OF(work, struct eswifi_off_socket, send_work);
	eswifi = eswifi_socket_to_dev(socket);

	eswifi_lock(eswifi);

	user_data = socket->send_data;
	cb = socket->send_cb;
	context = socket->context;

	err = __eswifi_off_send_pkt(eswifi, socket);
	socket->tx_pkt = NULL;

	eswifi_unlock(eswifi);

	if (cb) {
		cb(context, err, user_data);
	}
}

static int eswifi_off_send(struct net_pkt *pkt,
			   net_context_send_cb_t cb,
			   int32_t timeout,
			   void *user_data)
{
	struct eswifi_off_socket *socket = pkt->context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(socket->context->iface);
	int err;

	LOG_DBG("timeout=%d", timeout);

	eswifi_lock(eswifi);

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		eswifi_unlock(eswifi);
		return -ENOTCONN;
	}

	if (socket->tx_pkt) {
		eswifi_unlock(eswifi);
		return -EBUSY;
	}
	socket->tx_pkt = pkt;

	if (timeout == 0) {
		socket->send_data = user_data;
		socket->send_cb = cb;

		k_work_submit_to_queue(&eswifi->work_q, &socket->send_work);

		eswifi_unlock(eswifi);

		return 0;
	}

	err = __eswifi_off_send_pkt(eswifi, socket);
	socket->tx_pkt = NULL;

	eswifi_unlock(eswifi);

	if (cb) {
		cb(socket->context, err, user_data);
	}

	return err;
}

static int eswifi_off_sendto(struct net_pkt *pkt,
			     const struct sockaddr *dst_addr,
			     socklen_t addrlen,
			     net_context_send_cb_t cb,
			     int32_t timeout,
			     void *user_data)
{
	struct eswifi_off_socket *socket = pkt->context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(socket->context->iface);
	int err;

	LOG_DBG("timeout=%d", timeout);

	eswifi_lock(eswifi);

	if (socket->tx_pkt) {
		eswifi_unlock(eswifi);
		return -EBUSY;
	}

	socket->tx_pkt = pkt;

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		socket->peer_addr = *dst_addr;
		err = __eswifi_off_start_client(eswifi, socket);
		if (err < 0) {
			eswifi_unlock(eswifi);
			return err;
		}

		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
	}

	if (timeout == 0) {
		socket->send_data = user_data;
		socket->send_cb = cb;

		k_work_submit_to_queue(&eswifi->work_q, &socket->send_work);

		eswifi_unlock(eswifi);

		return 0;
	}

	err = __eswifi_off_send_pkt(eswifi, socket);
	socket->tx_pkt = NULL;

	eswifi_unlock(eswifi);

	if (cb) {
		cb(socket->context, err, user_data);
	}

	return err;
}

static int eswifi_off_recv(struct net_context *context,
			   net_context_recv_cb_t cb,
			   int32_t timeout,
			   void *user_data)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;


	LOG_DBG("");

	eswifi_lock(eswifi);
	socket->recv_cb = cb;
	socket->recv_data = user_data;
	k_sem_reset(&socket->read_sem);
	eswifi_unlock(eswifi);

	if (timeout == 0) {
		return 0;
	}

	err = k_sem_take(&socket->read_sem, K_MSEC(timeout));

	/* Unregister cakkback */
	eswifi_lock(eswifi);
	socket->recv_cb = NULL;
	eswifi_unlock(eswifi);

	return err;
}

static int eswifi_off_put(struct net_context *context)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int ret;

	LOG_DBG("");

	eswifi_lock(eswifi);

	ret = __eswifi_socket_free(eswifi, socket);
	if (ret) {
		goto done;
	}

	if (--socket->usage <= 0) {
		memset(socket, 0, sizeof(*socket));
	}
done:
	eswifi_unlock(eswifi);
	return ret;
}

static int eswifi_off_get(sa_family_t family,
			  enum net_sock_type type,
			  enum net_ip_protocol ip_proto,
			  struct net_context **context)
{
	struct eswifi_dev *eswifi = eswifi_by_iface_idx((*context)->iface);
	struct eswifi_off_socket *socket = NULL;
	int idx;

	LOG_DBG("");

	eswifi_lock(eswifi);

	idx = __eswifi_socket_new(eswifi, family, type, ip_proto, *context);
	if (idx < 0) {
		goto unlock;
	}

	socket = &eswifi->socket[idx];
	(*context)->offload_context = socket;

	LOG_DBG("Socket index %d", socket->index);

	k_work_init(&socket->connect_work, eswifi_off_connect_work);
	k_work_init(&socket->send_work, eswifi_off_send_work);
	k_sem_init(&socket->read_sem, 1, 1);
	k_sem_init(&socket->accept_sem, 1, 1);

	k_delayed_work_submit_to_queue(&eswifi->work_q, &socket->read_work,
				       K_MSEC(500));

unlock:
	eswifi_unlock(eswifi);
	return idx;
}

void eswifi_offload_async_msg(struct eswifi_dev *eswifi, char *msg, size_t len)
{
	static const char msg_tcp_accept[] = "[TCP SVR] Accepted ";

	if (!strncmp(msg, msg_tcp_accept, sizeof(msg_tcp_accept) - 1)) {
		struct eswifi_off_socket *socket = NULL;
		struct in_addr *sin_addr;
		uint8_t ip[4];
		uint16_t port = 0;
		char *str;
		int i = 0;

		/* extract client ip/port e.g. 192.168.1.1:8080 */
		/* TODO: use net_ipaddr_parse */
		str = msg + sizeof(msg_tcp_accept) - 1;
		while (*str) {
			if (i < 4) {
				ip[i++] = atoi(str);
			} else if (i < 5) {
				port = atoi(str);
				break;
			}

			while (*str && (*str != '.') && (*str != ':')) {
				str++;
			}

			str++;
		}

		for (i = 0; i < ESWIFI_OFFLOAD_MAX_SOCKETS; i++) {
			struct eswifi_off_socket *s = &eswifi->socket[i];
			if (s->context && s->port == port &&
			    s->state == ESWIFI_SOCKET_STATE_ACCEPTING) {
				socket = s;
				break;
			}
		}

		if (!socket) {
			LOG_ERR("No listening socket");
			return;
		}

		struct sockaddr_in *peer = net_sin(&socket->peer_addr);

		sin_addr = &peer->sin_addr;
		memcpy(&sin_addr->s4_addr, ip, 4);
		peer->sin_port = htons(port);
		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
		socket->usage++;

		/* Save information about remote. */
		socket->context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		memcpy(&socket->context->remote, &socket->peer_addr,
		       sizeof(struct sockaddr));

		LOG_DBG("%u.%u.%u.%u connected to port %u",
			ip[0], ip[1], ip[2], ip[3], port);

		if (socket->accept_cb) {
			socket->accept_cb(socket->context,
					  &socket->peer_addr,
					  sizeof(struct sockaddr_in), 0,
					  socket->accept_data);
		}

		k_sem_give(&socket->accept_sem);
		k_yield();
	}
}

static struct net_offload eswifi_offload = {
	.get	       = eswifi_off_get,
	.bind	       = eswifi_off_bind,
	.listen	       = eswifi_off_listen,
	.connect       = eswifi_off_connect,
	.accept	       = eswifi_off_accept,
	.send	       = eswifi_off_send,
	.sendto	       = eswifi_off_sendto,
	.recv	       = eswifi_off_recv,
	.put	       = eswifi_off_put,
};

static int eswifi_off_enable_dhcp(struct eswifi_dev *eswifi)
{
	char cmd[] = "C4=1\r";
	int err;

	LOG_DBG("");

	eswifi_lock(eswifi);

	err = eswifi_at_cmd(eswifi, cmd);

	eswifi_unlock(eswifi);

	return 0;
}

static int eswifi_off_disable_bypass(struct eswifi_dev *eswifi)
{
	char cmd[] = "PR=0\r";
	int err;

	LOG_DBG("");

	eswifi_lock(eswifi);

	err = eswifi_at_cmd(eswifi, cmd);

	eswifi_unlock(eswifi);

	return err;
}

int eswifi_offload_init(struct eswifi_dev *eswifi)
{
	eswifi->iface->if_dev->offload = &eswifi_offload;
	int err;

	err = eswifi_off_enable_dhcp(eswifi);
	if (err < 0) {
		LOG_ERR("Unable to configure dhcp");
		return err;
	}

	err = eswifi_off_disable_bypass(eswifi);
	if (err < 0) {
		LOG_ERR("Unable to disable bypass mode");
		return err;
	}

	return 0;
}
