/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_eswifi_offload);

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

static inline int __select_socket(struct eswifi_dev *eswifi, u8_t idx)
{
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P0=%d\r", idx);
	return eswifi_at_cmd(eswifi, eswifi->buf);
}

static int __read_data(struct eswifi_dev *eswifi, size_t len, char **data)
{
	char cmd[] = "R0\r";
	char size[] = "R1=9999\r";
	char timeout[] = "R2=30000\r";
	int ret;

	/* Set max read size */
	snprintf(size, sizeof(size), "R1=%u\r", len);
	ret = eswifi_at_cmd(eswifi, size);
	if (ret < 0) {
		LOG_ERR("Unable to set read size");
		return -EIO;
	}

	/* Set timeout */
	snprintf(timeout, sizeof(timeout), "R2=%u\r", 30); /* 30 ms */
	ret = eswifi_at_cmd(eswifi, timeout);
	if (ret < 0) {
		LOG_ERR("Unable to set timeout");
		return -EIO;
	}

	return eswifi_at_cmd_rsp(eswifi, cmd, data);
}

static inline
struct eswifi_dev *eswifi_socket_to_dev(struct eswifi_off_socket *socket)
{
	return CONTAINER_OF(socket - socket->index, struct eswifi_dev, socket);
}

static void eswifi_off_read_work(struct k_work *work)
{
	struct eswifi_off_socket *socket;
	struct eswifi_dev *eswifi;
	struct net_pkt *pkt;
	int err, len;
	char *data;

	LOG_DBG("");

	socket = CONTAINER_OF(work, struct eswifi_off_socket, read_work);
	eswifi = eswifi_socket_to_dev(socket);

	eswifi_lock(eswifi);

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		goto done;
	}

	__select_socket(eswifi, socket->index);

	len = __read_data(eswifi, 1460, &data); /* 1460 is max size */
	if (len <= 0 || !socket->recv_cb) {
		goto done;
	}

	LOG_ERR("payload sz = %d", len);

	pkt = net_pkt_rx_alloc_with_buffer(eswifi->iface, len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Cannot allocate rx packet");
		goto done;
	}

	if (!net_pkt_write_new(pkt, data, len)) {
		LOG_WRN("Incomplete buffer copy");
	}

	socket->recv_cb(socket->context, pkt,
			NULL, NULL, 0, socket->user_data);
	k_sem_give(&socket->read_sem);
	k_yield();

done:
	err = k_delayed_work_submit_to_queue(&eswifi->work_q,
					     &socket->read_work,
					     500);
	if (err) {
		LOG_ERR("Rescheduling socket read error");
	}

	eswifi_unlock(eswifi);
}

static int eswifi_off_bind(struct net_context *context,
			   const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;

	if (addr->sa_family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	LOG_DBG("");

	eswifi_lock(eswifi);

	__select_socket(eswifi, socket->index);

	/* Set Local Port */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P2=%d\r",
		 (u16_t)sys_be16_to_cpu(net_sin(addr)->sin_port));
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set local port");
		eswifi_unlock(eswifi);
		return -EIO;
	}

	eswifi_unlock(eswifi);

	return 0;
}

static int eswifi_off_listen(struct net_context *context, int backlog)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	char cmd[] = "P5=1\r";
	int err;

	/* TODO */
	LOG_ERR("");

	eswifi_lock(eswifi);

	__select_socket(eswifi, socket->index);

	/* Start TCP Server */
	err = eswifi_at_cmd(eswifi, cmd);
	if (err < 0) {
		LOG_ERR("Unable to start TCP server");
	}

	eswifi_unlock(eswifi);

	return err;
}

static int __eswifi_off_connect(struct eswifi_dev *eswifi,
				struct eswifi_off_socket *socket)
{
	struct sockaddr *addr = &socket->peer_addr;
	struct in_addr *sin_addr = &net_sin(addr)->sin_addr;
	int err;

	LOG_DBG("");

	__select_socket(eswifi, socket->index);

	/* Set Remote IP */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P3=%u.%u.%u.%u\r",
		 sin_addr->s4_addr[0], sin_addr->s4_addr[1],
		 sin_addr->s4_addr[2], sin_addr->s4_addr[3]);

	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set remote ip");
		return -EIO;
	}

	/* Set Remote Port */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P4=%d\r",
		(u16_t)sys_be16_to_cpu(net_sin(addr)->sin_port));
		err = eswifi_at_cmd(eswifi, eswifi->buf);
		if (err < 0) {
		LOG_ERR("Unable to set remote port");
		return -EIO;
	}

	/* Start TCP client */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P6=1\r");
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to connect");
		return -EIO;
	}

	return 0;
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
	user_data = socket->user_data;

	err = __eswifi_off_connect(eswifi, socket);
	if (!err) {
		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
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
			      s32_t timeout,
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
	socket->user_data = user_data;
	socket->state = ESWIFI_SOCKET_STATE_CONNECTING;

	if (timeout == K_NO_WAIT) {
		/* async */
		k_work_submit_to_queue(&eswifi->work_q, &socket->connect_work);
		eswifi_unlock(eswifi);
		return 0;
	}

	err = __eswifi_off_connect(eswifi, socket);
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
			     net_tcp_accept_cb_t cb, s32_t timeout,
			     void *user_data)
{
	/* TODO */
	LOG_DBG("");
	return -ENOTSUP;
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
	snprintf(eswifi->buf, sizeof(eswifi->buf), "S3=%u\r", bytes);
	offset = strlen(eswifi->buf);

	/* copy payload */
	if (net_pkt_read_new(pkt, &eswifi->buf[offset], bytes)) {
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
	void *user_data, *token;
	int err;

	socket = CONTAINER_OF(work, struct eswifi_off_socket, connect_work);
	eswifi = eswifi_socket_to_dev(socket);

	eswifi_lock(eswifi);

	user_data = socket->user_data;
	token = socket->tx_token;
	cb = socket->send_cb;
	context = socket->context;

	err = __eswifi_off_send_pkt(eswifi, socket);
	socket->tx_pkt = NULL;

	eswifi_unlock(eswifi);

	if (cb) {
		cb(context, err, token, user_data);
	}
}

static int eswifi_off_send(struct net_pkt *pkt,
			   net_context_send_cb_t cb,
			   s32_t timeout,
			   void *token,
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

	if (timeout == K_NO_WAIT) {
		socket->tx_token = token;
		socket->user_data = user_data;
		socket->send_cb = cb;

		k_work_submit_to_queue(&eswifi->work_q, &socket->send_work);

		eswifi_unlock(eswifi);

		return 0;
	}

	err = __eswifi_off_send_pkt(eswifi, socket);
	socket->tx_pkt = NULL;

	eswifi_unlock(eswifi);

	if (cb) {
		cb(socket->context, err, token, user_data);
	}

	return err;
}

static int eswifi_off_sendto(struct net_pkt *pkt,
			     const struct sockaddr *dst_addr,
			     socklen_t addrlen,
			     net_context_send_cb_t cb,
			     s32_t timeout,
			     void *token,
			     void *user_data)
{
	/* TODO */
	LOG_DBG("");
	return -ENOTSUP;
}

static int eswifi_off_recv(struct net_context *context,
			   net_context_recv_cb_t cb,
			   s32_t timeout,
			   void *user_data)
{
	struct eswifi_off_socket *socket = context->offload_context;
	struct eswifi_dev *eswifi = eswifi_by_iface_idx(context->iface);
	int err;


	LOG_DBG("");

	eswifi_lock(eswifi);
	socket->recv_cb = cb;
	socket->user_data = user_data;
	k_sem_reset(&socket->read_sem);
	eswifi_unlock(eswifi);

	if (timeout == K_NO_WAIT)
		return 0;

	err = k_sem_take(&socket->read_sem, timeout);

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
	int err;

	LOG_DBG("");

	eswifi_lock(eswifi);

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		eswifi_unlock(eswifi);
		return -ENOTCONN;
	}

	__select_socket(eswifi, socket->index);

	k_delayed_work_cancel(&socket->read_work);

	socket->context = NULL;
	socket->state = ESWIFI_SOCKET_STATE_NONE;

	/* Stop TCP client */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P6=0\r");
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		err = -EIO;
		LOG_ERR("Unable to disconnect");
	}

	eswifi_unlock(eswifi);

	return err;
}

static int eswifi_off_get(sa_family_t family,
			  enum net_sock_type type,
			  enum net_ip_protocol ip_proto,
			  struct net_context **context)
{
	struct eswifi_dev *eswifi = eswifi_by_iface_idx((*context)->iface);
	struct eswifi_off_socket *socket = NULL;
	int err, i;

	LOG_DBG("");

	if (family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	if (ip_proto != IPPROTO_TCP) {
		/* TODO: add UDP */
		LOG_ERR("Only TCP supported");
		return -EPROTONOSUPPORT;
	}

	eswifi_lock(eswifi);

	/* pickup available socket */
	for (i = 0; i < ESWIFI_OFFLOAD_MAX_SOCKETS; i++) {
		if (!eswifi->socket[i].context) {
			socket = &eswifi->socket[i];
			socket->index = i;
			socket->context = *context;
			(*context)->offload_context = socket;
			break;
		}
	}

	if (!socket) {
		LOG_ERR("No socket resource available");
		eswifi_unlock(eswifi);
		return -ENOMEM;
	}

	k_work_init(&socket->connect_work, eswifi_off_connect_work);
	k_work_init(&socket->send_work, eswifi_off_send_work);
	k_delayed_work_init(&socket->read_work, eswifi_off_read_work);
	k_sem_init(&socket->read_sem, 1, 1);

	err = __select_socket(eswifi, socket->index);
	if (err < 0) {
		LOG_ERR("Unable to select socket %u", socket->index);
		eswifi_unlock(eswifi);
		return -EIO;
	}

	/* Set Transport Protocol */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "P1=%d\r",
		ESWIFI_TRANSPORT_TCP);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set transport protocol");
		eswifi_unlock(eswifi);
		return -EIO;
	}

	k_delayed_work_submit_to_queue(&eswifi->work_q, &socket->read_work,
				       500);

	eswifi_unlock(eswifi);

	return 0;
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
