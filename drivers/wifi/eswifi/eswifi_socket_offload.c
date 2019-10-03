/*
 * Copyright (c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_eswifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <net/socket_offload.h>
#include <net/tls_credentials.h>
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#endif
#include "eswifi.h"
#include <net/net_pkt.h>

static struct eswifi_dev *eswifi;

static void __process_received(struct net_context *context,
			      struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      union net_proto_header *proto_hdr,
			      int status,
			      void *user_data)
{
	struct eswifi_off_socket *socket = user_data;

	eswifi_lock(eswifi);
	k_fifo_put(&socket->fifo, pkt);
	eswifi_unlock(eswifi);
}

static int eswifi_socket_connect(int sock, const struct sockaddr *addr,
				 socklen_t addrlen)
{
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == 0) || (addr == NULL) ||
	    (sock > ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	if (addr->sa_family != AF_INET) {
		LOG_ERR("Only AF_INET is supported!");
		return -EPFNOSUPPORT;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	if (socket->state != ESWIFI_SOCKET_STATE_NONE) {
		eswifi_unlock(eswifi);
		return -EBUSY;
	}

	socket->peer_addr = *addr;
	socket->state = ESWIFI_SOCKET_STATE_CONNECTING;

	ret = __eswifi_off_start_client(eswifi, socket);
	if (!ret) {
		socket->state = ESWIFI_SOCKET_STATE_CONNECTED;
	} else {
		socket->state = ESWIFI_SOCKET_STATE_NONE;
	}

	eswifi_unlock(eswifi);
	return ret;
}

static int eswifi_socket_accept(int sock, struct sockaddr *addr,
				socklen_t *addrlen)
{
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == NULL) || (addr == NULL) ||
	    (sock > ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	ret = __eswifi_accept(eswifi, socket);
	eswifi_unlock(eswifi);

	return ret;
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
static int map_credentials(int sd, const void *optval, socklen_t optlen)
{
	sec_tag_t *sec_tags = (sec_tag_t *)optval;
	int ret = 0;
	int tags_len;
	sec_tag_t tag;
	int id;
	int i, bytes;
	struct tls_credential *cert;

	if ((optlen % sizeof(sec_tag_t)) != 0 || (optlen == 0)) {
		return -EINVAL;
	}

	tags_len = optlen / sizeof(sec_tag_t);
	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
		while (cert != NULL) {
			/* Map Zephyr cert types to Simplelink cert options: */
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				id = 0;
				break;
			case TLS_CREDENTIAL_SERVER_CERTIFICATE:
				id = 1;
				break;
			case TLS_CREDENTIAL_PRIVATE_KEY:
				id = 2;
				break;
			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				/* Not handled */
				return -EINVAL;
			}

			snprintf(eswifi->buf, sizeof(eswifi->buf),
				 "PG=%d,%d,%d\r", 0, id, cert->len);
			bytes = strlen(eswifi->buf);
			memcpy(&eswifi->buf[bytes], cert->buf, cert->len);
			bytes += cert->len;
			LOG_DBG("cert write len %d\n", cert->len);
			ret = eswifi_request(eswifi, eswifi->buf, bytes + 1,
				     eswifi->buf, sizeof(eswifi->buf));
			LOG_DBG("cert write err %d\n", ret);
			if (ret < 0) {
				return ret;
			}

			snprintf(eswifi->buf, sizeof(eswifi->buf), "PF=0,0\r");
			ret = eswifi_at_cmd(eswifi, eswifi->buf);
			if (ret < 0) {
				return ret;
			}

			cert = credential_next_get(tag, cert);
		}
	}

	return 0;
}
#else
static int map_credentials(int sd, const void *optval, socklen_t optlen)
{
	return 0;
}
#endif

static int eswifi_socket_setsockopt(int sd, int level, int optname,
				    const void *optval, socklen_t optlen)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			ret = map_credentials(sd, optval, optlen);
			break;
		case TLS_HOSTNAME:
		case TLS_PEER_VERIFY:
			ret = 0;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return ret;
}

static ssize_t eswifi_socket_send(int sock, const void *buf, size_t len,
				  int flags)
{
	struct eswifi_off_socket *socket;
	int ret;
	int offset;

	if (!buf) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		eswifi_unlock(eswifi);
		return -ENOTCONN;
	}

	__select_socket(eswifi, socket->index);

	/* header */
	snprintf(eswifi->buf, sizeof(eswifi->buf), "S3=%u\r", len);
	offset = strlen(eswifi->buf);

	/* copy payload */
	memcpy(&eswifi->buf[offset], buf, len);
	offset += len;

	ret = eswifi_request(eswifi, eswifi->buf, offset + 1, eswifi->buf,
				sizeof(eswifi->buf));
	if (ret < 0) {
		LOG_DBG("Unable to send data");
		ret = -EIO;
	} else {
		ret = len;
	}

	eswifi_unlock(eswifi);
	return ret;
}

static ssize_t eswifi_socket_recv(int sock, void *buf, size_t max_len,
				  int flags)
{
	struct eswifi_off_socket *socket;
	int len = 0, ret = 0;
	struct net_pkt *pkt;

	if ((max_len == 0) || (buf == NULL) ||
	    (sock > ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	if (socket->prev_pkt_rem) {
		pkt = socket->prev_pkt_rem;
		goto skip_wait;
	}

	pkt = k_fifo_get(&socket->fifo, K_NO_WAIT);
	if (!pkt) {
		errno = EAGAIN;
		len = -EAGAIN;
		goto done;
	}

skip_wait:
	len = net_pkt_remaining_data(pkt);
	if (len > max_len) {
		len = max_len;
		socket->prev_pkt_rem = pkt;
	} else {
		socket->prev_pkt_rem = NULL;
	}

	ret = net_pkt_read(pkt, buf, len);

	if (!socket->prev_pkt_rem) {
		net_pkt_unref(pkt);
	}

done:
	LOG_DBG("read %d %d %p", len, ret, pkt);
	eswifi_unlock(eswifi);
	if (ret) {
		len = 0;
	}

	return len;
}

static int eswifi_socket_close(int sock)
{
	struct eswifi_off_socket *socket;
	struct net_pkt *pkt;
	int ret;

	if (sock > ESWIFI_OFFLOAD_MAX_SOCKETS) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);

	socket = &eswifi->socket[sock];
	ret = __eswifi_socket_free(eswifi, socket);
	if (ret) {
		goto done;
	}

	/* consume all net pkt */
	while (1) {
		pkt = k_fifo_get(&socket->fifo, K_NO_WAIT);
		if (!pkt) {
			break;
		}
		net_pkt_unref(pkt);
	}

	if (--socket->usage <= 0) {
		memset(socket, 0, sizeof(*socket));
	}

done:
	eswifi_unlock(eswifi);
	return ret;
}

static int eswifi_socket_open(int family, int type, int proto)
{
	struct eswifi_off_socket *socket = NULL;
	int idx;

	eswifi_lock(eswifi);

	/* Assign dummy context SOCkEt(50CE) */
	idx = __eswifi_socket_new(eswifi, family, type, proto, 0x50CE);
	if (idx < 0) {
		goto unlock;
	}

	socket = &eswifi->socket[idx];
	k_fifo_init(&socket->fifo);
	k_sem_init(&socket->read_sem, 0, 200);
	socket->prev_pkt_rem = NULL;
	socket->recv_cb = __process_received;
	socket->recv_data = socket;

	k_delayed_work_submit_to_queue(&eswifi->work_q, &socket->read_work,
					K_MSEC(500));

unlock:
	eswifi_unlock(eswifi);
	return idx;
}

static int eswifi_socket_poll(struct pollfd *fds, int nfds, int msecs)
{
	struct eswifi_off_socket *socket;
	int sock, ret;

	if (nfds > 1 ||
	    fds[0].fd  > ESWIFI_OFFLOAD_MAX_SOCKETS) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	sock = fds[0].fd;
	socket = &eswifi->socket[sock];
	eswifi_unlock(eswifi);
	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		return -EINVAL;
	}

	ret = k_sem_take(&socket->read_sem, msecs);
	return ret;
}

static int ewifi_socket_bind(int sock, const struct sockaddr *addr,
			     socklen_t addrlen)
{
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == NULL) || (addr == NULL) ||
	    (sock > ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];
	ret = __eswifi_bind(eswifi, socket, addr, addrlen);
	eswifi_unlock(eswifi);

	return ret;
}

const struct socket_offload eswifi_socket_ops = {
	/* POSIX Socket Functions: */
	.socket = eswifi_socket_open,
	.close = eswifi_socket_close,
	.accept = eswifi_socket_accept,
	.bind = ewifi_socket_bind,
	.connect = eswifi_socket_connect,
	.setsockopt = eswifi_socket_setsockopt,
	.recv = eswifi_socket_recv,
	.send = eswifi_socket_send,
	.poll = eswifi_socket_poll,
};

int eswifi_socket_offload_init(struct eswifi_dev *leswifi)
{
	eswifi = leswifi;
	socket_offload_register(&eswifi_socket_ops);
	return 0;
}
