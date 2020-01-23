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

#include "sockets_internal.h"
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#endif
#include "eswifi.h"
#include <net/net_pkt.h>

/* Increment by 1 to make sure we do not store the value of 0, which has
 * a special meaning in the fdtable subsys.
 */
#define SD_TO_OBJ(sd) ((void *)(sd + 1))
#define OBJ_TO_SD(obj) (((int)obj) - 1)

static struct eswifi_dev *eswifi;
static const struct socket_op_vtable eswifi_socket_fd_op_vtable;

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

static int eswifi_socket_connect(void *obj, const struct sockaddr *addr,
				 socklen_t addrlen)
{
	int sock = OBJ_TO_SD(obj);
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

static int __eswifi_socket_accept(void *obj, struct sockaddr *addr,
				  socklen_t *addrlen)
{
	int sock = OBJ_TO_SD(obj);
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

static int eswifi_socket_accept(void *obj, struct sockaddr *addr,
				socklen_t *addrlen)
{
	int fd = z_reserve_fd();
	int sock;

	if (fd < 0) {
		return -1;
	}

	sock = __eswifi_socket_accept(obj, addr, addrlen);
	if (sock < 0) {
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(fd, SD_TO_OBJ(sock),
		      (const struct fd_op_vtable *)
					&eswifi_socket_fd_op_vtable);

	return fd;
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

static int eswifi_socket_setsockopt(void *obj, int level, int optname,
				    const void *optval, socklen_t optlen)
{
	int sd = OBJ_TO_SD(obj);
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

static ssize_t eswifi_socket_send(void *obj, const void *buf, size_t len,
				  int flags)
{
	int sock = OBJ_TO_SD(obj);
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

static ssize_t eswifi_socket_sendto(void *obj, const void *buf, size_t len,
				    int flags, const struct sockaddr *to,
				    socklen_t tolen)
{
	if (to != NULL) {
		errno = EOPNOTSUPP;
		return -1;
	}

	return eswifi_socket_send(obj, buf, len, flags);
}

static ssize_t eswifi_socket_recv(void *obj, void *buf, size_t max_len,
				  int flags)
{
	int sock = OBJ_TO_SD(obj);
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

static ssize_t eswifi_socket_recvfrom(void *obj, void *buf, size_t len,
				      int flags, struct sockaddr *from,
				      socklen_t *fromlen)
{
	if (fromlen != NULL) {
		errno = EOPNOTSUPP;
		return -1;
	}

	return eswifi_socket_recv(obj, buf, len, flags);
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
	void *obj;

	if (nfds > 1) {
		errno = EINVAL;
		return -1;
	}

	obj = z_get_fd_obj(fds[0].fd,
			   (const struct fd_op_vtable *)
						&eswifi_socket_fd_op_vtable,
			   0);
	if (obj != NULL) {
		sock = OBJ_TO_SD(obj);
	} else {
		errno = EINVAL;
		return -1;
	}

	if (sock > ESWIFI_OFFLOAD_MAX_SOCKETS) {
		errno = EINVAL;
		return -1;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];
	eswifi_unlock(eswifi);
	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		errno = EINVAL;
		return -1;
	}

	ret = k_sem_take(&socket->read_sem, msecs);
	return ret;
}

static int eswifi_socket_bind(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	int sock = OBJ_TO_SD(obj);
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

static bool eswifi_socket_is_supported(int family, int type, int proto)
{
	return true;
}

static int eswifi_socket_create(int family, int type, int proto)
{
	int fd = z_reserve_fd();
	int sock;

	if (fd < 0) {
		return -1;
	}

	sock = eswifi_socket_open(family, type, proto);
	if (sock < 0) {
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(fd, SD_TO_OBJ(sock),
		      (const struct fd_op_vtable *)
					&eswifi_socket_fd_op_vtable);

	return fd;
}

static int eswifi_socket_ioctl(void *obj, unsigned int request, va_list args)
{
	int sd = OBJ_TO_SD(obj);

	switch (request) {
	/* Handle close specifically. */
	case ZFD_IOCTL_CLOSE:
		return eswifi_socket_close(sd);

	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return eswifi_socket_poll(fds, nfds, timeout);
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

static ssize_t eswifi_socket_read(void *obj, void *buffer, size_t count)
{
	return eswifi_socket_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t eswifi_socket_write(void *obj, const void *buffer,
				   size_t count)
{
	return eswifi_socket_sendto(obj, buffer, count, 0, NULL, 0);
}

static const struct socket_op_vtable eswifi_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = eswifi_socket_read,
		.write = eswifi_socket_write,
		.ioctl = eswifi_socket_ioctl,
	},
	.bind = eswifi_socket_bind,
	.connect = eswifi_socket_connect,
	.accept = eswifi_socket_accept,
	.sendto = eswifi_socket_sendto,
	.recvfrom = eswifi_socket_recvfrom,
	.setsockopt = eswifi_socket_setsockopt,
};

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
NET_SOCKET_REGISTER(eswifi, AF_UNSPEC, eswifi_socket_is_supported,
		    eswifi_socket_create);
#endif

int eswifi_socket_offload_init(struct eswifi_dev *leswifi)
{
	eswifi = leswifi;
	return 0;
}
