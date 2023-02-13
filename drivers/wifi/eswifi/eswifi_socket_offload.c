/*
 * Copyright (c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eswifi_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/net/socket_offload.h>
#include <zephyr/net/tls_credentials.h>

#include "sockets_internal.h"
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#endif
#include "eswifi.h"
#include <zephyr/net/net_pkt.h>

/* Increment by 1 to make sure we do not store the value of 0, which has
 * a special meaning in the fdtable subsys.
 */
#define SD_TO_OBJ(sd) ((void *)(sd + 1))
#define OBJ_TO_SD(obj) (((int)obj) - 1)
/* Default socket context (50CE) */
#define ESWIFI_INIT_CONTEXT	INT_TO_POINTER(0x50CE)

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

	if (!pkt) {
		k_fifo_cancel_wait(&socket->fifo);
		return;
	}

	k_fifo_put(&socket->fifo, pkt);
}

static int eswifi_socket_connect(void *obj, const struct sockaddr *addr,
				 socklen_t addrlen)
{
	int sock = OBJ_TO_SD(obj);
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == 0) || (addr == NULL) ||
	    (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS)) {
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

static int eswifi_socket_listen(void *obj, int backlog)
{
	struct eswifi_off_socket *socket;
	int sock = OBJ_TO_SD(obj);
	int ret;

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	ret = __eswifi_listen(eswifi, socket, backlog);
	eswifi_unlock(eswifi);

	return ret;
}

void __eswifi_socket_accept_cb(struct net_context *context, struct sockaddr *addr,
			       unsigned int len, int val, void *data)
{
	struct sockaddr *addr_target = data;

	memcpy(addr_target, addr, len);
}

static int __eswifi_socket_accept(void *obj, struct sockaddr *addr,
				  socklen_t *addrlen)
{
	int sock = OBJ_TO_SD(obj);
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == NULL) || (addr == NULL) ||
	    (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	ret = __eswifi_accept(eswifi, socket);
	socket->accept_cb = __eswifi_socket_accept_cb;
	socket->accept_data = addr;
	k_sem_reset(&socket->accept_sem);
	eswifi_unlock(eswifi);

	*addrlen = sizeof(struct sockaddr_in);

	k_sem_take(&socket->accept_sem, K_FOREVER);

	return 0;
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

			snprintk(eswifi->buf, sizeof(eswifi->buf),
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

			snprintk(eswifi->buf, sizeof(eswifi->buf), "PF=0,0\r");
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
	snprintk(eswifi->buf, sizeof(eswifi->buf), "S3=%u\r", len);
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
	    (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS)) {
		return -EINVAL;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	if (socket->prev_pkt_rem) {
		pkt = socket->prev_pkt_rem;
		goto skip_wait;
	}

	ret = k_work_reschedule_for_queue(&eswifi->work_q, &socket->read_work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Rescheduling socket read error");
		errno = -ret;
		len = -1;
	}

	if (flags & ZSOCK_MSG_DONTWAIT) {
		pkt = k_fifo_get(&socket->fifo, K_NO_WAIT);
		if (!pkt) {
			errno = EAGAIN;
			len = -1;
			goto done;
		}
	} else {
		eswifi_unlock(eswifi);
		pkt = k_fifo_get(&socket->fifo, K_FOREVER);
		if (!pkt) {
			return 0; /* EOF */
		}
		eswifi_lock(eswifi);
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

static int eswifi_socket_close(void *obj)
{
	int sock = OBJ_TO_SD(obj);
	struct eswifi_off_socket *socket;
	struct net_pkt *pkt;
	int ret;

	if (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS) {
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
		socket->context = NULL;
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

	idx = __eswifi_socket_new(eswifi, family, type, proto, ESWIFI_INIT_CONTEXT);
	if (idx < 0) {
		goto unlock;
	}

	socket = &eswifi->socket[idx];
	k_fifo_init(&socket->fifo);
	k_sem_init(&socket->read_sem, 0, 200);
	k_sem_init(&socket->accept_sem, 1, 1);
	socket->prev_pkt_rem = NULL;
	socket->recv_cb = __process_received;
	socket->recv_data = socket;

	k_work_reschedule_for_queue(&eswifi->work_q, &socket->read_work,
				    K_MSEC(500));

unlock:
	eswifi_unlock(eswifi);
	return idx;
}

static int eswifi_socket_poll(struct zsock_pollfd *fds, int nfds, int msecs)
{
	struct eswifi_off_socket *socket;
	int sock, ret;
	void *obj;

	if (nfds != 1) {
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

	if (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS) {
		errno = EINVAL;
		return -1;
	}

	if (!(fds[0].events & ZSOCK_POLLIN)) {
		errno = ENOTSUP;
		return -1;
	}

	eswifi_lock(eswifi);
	socket = &eswifi->socket[sock];

	ret = k_work_reschedule_for_queue(&eswifi->work_q, &socket->read_work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Rescheduling socket read error");
		errno = -ret;
		eswifi_unlock(eswifi);
		return -1;
	}

	eswifi_unlock(eswifi);

	if (socket->state != ESWIFI_SOCKET_STATE_CONNECTED) {
		errno = EINVAL;
		return -1;
	}

	ret = k_sem_take(&socket->read_sem, K_MSEC(msecs));
	if (ret) {
		errno = ETIMEDOUT;
		return -1;
	}

	fds[0].revents = ZSOCK_POLLIN;

	/* Report one event */
	return 1;
}

static int eswifi_socket_bind(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	int sock = OBJ_TO_SD(obj);
	struct eswifi_off_socket *socket;
	int ret;

	if ((addrlen == 0) || (addr == NULL) ||
	    (sock >= ESWIFI_OFFLOAD_MAX_SOCKETS)) {
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
	enum eswifi_transport_type eswifi_socket_type;
	int err;

	if (family != AF_INET) {
		return false;
	}

	if (type != SOCK_DGRAM &&
	    type != SOCK_STREAM) {
		return false;
	}

	err = eswifi_socket_type_from_zephyr(proto, &eswifi_socket_type);
	if (err) {
		return false;
	}

	return true;
}

int eswifi_socket_create(int family, int type, int proto)
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
	switch (request) {
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
		.close = eswifi_socket_close,
		.ioctl = eswifi_socket_ioctl,
	},
	.bind = eswifi_socket_bind,
	.connect = eswifi_socket_connect,
	.listen = eswifi_socket_listen,
	.accept = eswifi_socket_accept,
	.sendto = eswifi_socket_sendto,
	.recvfrom = eswifi_socket_recvfrom,
	.setsockopt = eswifi_socket_setsockopt,
};

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
NET_SOCKET_OFFLOAD_REGISTER(eswifi, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,
			    eswifi_socket_is_supported, eswifi_socket_create);
#endif

static int eswifi_off_getaddrinfo(const char *node, const char *service,
				  const struct zsock_addrinfo *hints,
				  struct zsock_addrinfo **res)
{
	struct sockaddr_in *ai_addr;
	struct zsock_addrinfo *ai;
	unsigned long port = 0;
	char *rsp;
	int err;

	if (!node) {
		return DNS_EAI_NONAME;
	}

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (!res) {
		return DNS_EAI_NONAME;
	}

	if (hints && hints->ai_family != AF_INET) {
		return DNS_EAI_FAIL;
	}

	eswifi_lock(eswifi);

	/* DNS lookup */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "D0=%s\r", node);
	err = eswifi_at_cmd_rsp(eswifi, eswifi->buf, &rsp);
	if (err < 0) {
		err = DNS_EAI_FAIL;
		goto done_unlock;
	}

	/* Allocate out res (addrinfo) struct.	Just one. */
	*res = calloc(1, sizeof(struct zsock_addrinfo));
	ai = *res;
	if (!ai) {
		err = DNS_EAI_MEMORY;
		goto done_unlock;
	}

	/* Now, alloc the embedded sockaddr struct: */
	ai_addr = calloc(1, sizeof(*ai_addr));
	if (!ai_addr) {
		free(*res);
		err = DNS_EAI_MEMORY;
		goto done_unlock;
	}

	ai->ai_family = AF_INET;
	ai->ai_socktype = hints ? hints->ai_socktype : SOCK_STREAM;
	ai->ai_protocol = ai->ai_socktype == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP;

	ai_addr->sin_family = ai->ai_family;
	ai_addr->sin_port = htons(port);

	if (!net_ipaddr_parse(rsp, strlen(rsp), (struct sockaddr *)ai_addr)) {
		free(ai_addr);
		free(*res);
		err = DNS_EAI_FAIL;
		goto done_unlock;
	}

	ai->ai_addrlen = sizeof(*ai_addr);
	ai->ai_addr = (struct sockaddr *)ai_addr;
	err = 0;

done_unlock:
	eswifi_unlock(eswifi);
	return err;
}

static void eswifi_off_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(res);

	free(res->ai_addr);
	free(res);
}

const struct socket_dns_offload eswifi_dns_ops = {
	.getaddrinfo = eswifi_off_getaddrinfo,
	.freeaddrinfo = eswifi_off_freeaddrinfo,
};

int eswifi_socket_offload_init(struct eswifi_dev *leswifi)
{
	eswifi = leswifi;

	socket_offload_dns_register(&eswifi_dns_ops);

	return 0;
}
