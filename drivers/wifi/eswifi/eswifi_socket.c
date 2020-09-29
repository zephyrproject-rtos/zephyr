/*
 * Copyright (c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eswifi_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "eswifi.h"
#include <net/net_pkt.h>

static int __stop_socket(struct eswifi_dev *eswifi,
			 struct eswifi_off_socket *socket)
{
	char cmd_srv[] = "P5=0\r";
	char cmd_cli[] = "P6=0\r";

	LOG_DBG("Stopping socket %d", socket->index);
	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		return 0;
	}

	socket->state = ESWIFI_SOCKET_STATE_NONE;
	return eswifi_at_cmd(eswifi, socket->is_server ? cmd_srv : cmd_cli);
}

static int __read_data(struct eswifi_dev *eswifi, size_t len, char **data)
{
	char cmd[] = "R0\r";
	char size[] = "R1=9999\r";
	char timeout[] = "R2=30000\r";
	int ret;

	/* Set max read size */
	snprintk(size, sizeof(size), "R1=%u\r", len);
	ret = eswifi_at_cmd(eswifi, size);
	if (ret < 0) {
		LOG_ERR("Unable to set read size");
		return -EIO;
	}

	/* Set timeout */
	snprintk(timeout, sizeof(timeout), "R2=%u\r", 30); /* 30 ms */
	ret = eswifi_at_cmd(eswifi, timeout);
	if (ret < 0) {
		LOG_ERR("Unable to set timeout");
		return -EIO;
	}

	return eswifi_at_cmd_rsp(eswifi, cmd, data);
}

int __eswifi_bind(struct eswifi_dev *eswifi, struct eswifi_off_socket *socket,
		      const struct sockaddr *addr, socklen_t addrlen)
{
	int err;

	if (addr->sa_family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	__select_socket(eswifi, socket->index);
	socket->port = sys_be16_to_cpu(net_sin(addr)->sin_port);

	/* Set Local Port */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P2=%d\r", socket->port);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set local port");
		return -EIO;
	}

	return 0;
}

static void eswifi_off_read_work(struct k_work *work)
{
	struct eswifi_off_socket *socket;
	struct eswifi_dev *eswifi;
	struct net_pkt *pkt = NULL;
	int next_timeout_ms = 100;
	int err, len;
	char *data;

	LOG_DBG("");

	socket = CONTAINER_OF(work, struct eswifi_off_socket, read_work);
	eswifi = eswifi_socket_to_dev(socket);

	eswifi_lock(eswifi);

	if ((socket->type == ESWIFI_TRANSPORT_TCP ||
	     socket->type == ESWIFI_TRANSPORT_TCP_SSL) &&
	     socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		goto done;
	}

	__select_socket(eswifi, socket->index);

	len = __read_data(eswifi, 1460, &data); /* 1460 is max size */
	if (len < 0) {
		__stop_socket(eswifi, socket);

		if (socket->recv_cb) {
			/* send EOF (null pkt) */
			goto do_recv_cb;
		}
	}

	if (!len || !socket->recv_cb) {
		goto done;
	}

	LOG_DBG("payload sz = %d", len);

	pkt = net_pkt_rx_alloc_with_buffer(eswifi->iface, len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Cannot allocate rx packet");
		goto done;
	}

	if (net_pkt_write(pkt, data, len) < 0) {
		LOG_WRN("Incomplete buffer copy");
	}

	net_pkt_cursor_init(pkt);

do_recv_cb:
	socket->recv_cb(socket->context, pkt,
			NULL, NULL, 0, socket->recv_data);

	if (!socket->context) {
		/* something destroyed the socket in the recv path */
		eswifi_unlock(eswifi);
		return;
	}

	k_sem_give(&socket->read_sem);
	next_timeout_ms = 0;

done:
	err = k_delayed_work_submit_to_queue(&eswifi->work_q,
					     &socket->read_work,
					     K_MSEC(next_timeout_ms));
	if (err) {
		LOG_ERR("Rescheduling socket read error");
	}

	eswifi_unlock(eswifi);
}

int __eswifi_off_start_client(struct eswifi_dev *eswifi,
			      struct eswifi_off_socket *socket)
{
	struct sockaddr *addr = &socket->peer_addr;
	struct in_addr *sin_addr = &net_sin(addr)->sin_addr;
	int err;

	LOG_DBG("");

	__select_socket(eswifi, socket->index);

	/* Stop any running client */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P6=0\r");
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to stop running client");
		return -EIO;
	}

	/* Set Remote IP */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P3=%u.%u.%u.%u\r",
		 sin_addr->s4_addr[0], sin_addr->s4_addr[1],
		 sin_addr->s4_addr[2], sin_addr->s4_addr[3]);

	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set remote ip");
		return -EIO;
	}

	/* Set Remote Port */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P4=%d\r",
		(uint16_t)sys_be16_to_cpu(net_sin(addr)->sin_port));
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set remote port");
		return -EIO;
	}

	/* Start TCP/UDP client */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P6=1\r");
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to start TCP/UDP client");
		return -EIO;
	}

	return 0;
}

int __eswifi_accept(struct eswifi_dev *eswifi, struct eswifi_off_socket *socket)
{
	char cmd[] = "P5=1\r";

	if (socket->state != ESWIFI_SOCKET_STATE_NONE) {
		/* we can only handle one connection at a time */
		return -EBUSY;
	}

	__select_socket(eswifi, socket->index);

	/* Start TCP Server */
	if (eswifi_at_cmd(eswifi, cmd) < 0) {
		LOG_ERR("Unable to start TCP server");
		return -EIO;
	}

	LOG_DBG("TCP Server started");
	socket->state = ESWIFI_SOCKET_STATE_ACCEPTING;

	return 0;
}

int __eswifi_socket_free(struct eswifi_dev *eswifi,
			 struct eswifi_off_socket *socket)
{
	__select_socket(eswifi, socket->index);
	k_delayed_work_cancel(&socket->read_work);

	__select_socket(eswifi, socket->index);
	__stop_socket(eswifi, socket);

	return 0;
}

int __eswifi_socket_new(struct eswifi_dev *eswifi, int family, int type,
			int proto, void *context)
{
	struct eswifi_off_socket *socket = NULL;
	int err, i;

	LOG_DBG("");

	if (family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	/* pickup available socket */
	for (i = 0; i < ESWIFI_OFFLOAD_MAX_SOCKETS; i++) {
		if (!eswifi->socket[i].context) {
			socket = &eswifi->socket[i];
			socket->index = i;
			socket->context = context;
			break;
		}
	}

	if (!socket) {
		LOG_ERR("No socket resource available");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) &&
	    proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) {
		socket->type = ESWIFI_TRANSPORT_TCP_SSL;
	} else if (proto == IPPROTO_TCP) {
		socket->type = ESWIFI_TRANSPORT_TCP;
	} else if (proto == IPPROTO_UDP) {
		socket->type = ESWIFI_TRANSPORT_UDP;
	} else {
		LOG_ERR("Only TCP & UDP is supported");
		return -EPFNOSUPPORT;
	}

	err = __select_socket(eswifi, socket->index);
	if (err < 0) {
		LOG_ERR("Unable to select socket %u", socket->index);
		return -EIO;
	}

	snprintk(eswifi->buf, sizeof(eswifi->buf), "P1=%d\r", socket->type);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set transport protocol");
		return -EIO;
	}

	k_delayed_work_init(&socket->read_work, eswifi_off_read_work);
	socket->usage = 1;
	LOG_DBG("Socket index %d", socket->index);

	return socket->index;
}
