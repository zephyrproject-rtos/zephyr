/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <kernel.h>
#include <net/net_context.h>

/* Zephyr headers */
#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <kernel.h>
#include <net/socket.h>
#include <syscall_handler.h>
#include <sys/fdtable.h>

#include "sockets_internal.h"

#define SPAIR_MAGIC 0x9a12

#define D(fmt, args...) printk("D: %s(): %d: " fmt "\n", __func__, __LINE__, ##args)

enum {
	SPAIR_CANCEL,
	SPAIR_CAN_READ,
	SPAIR_CAN_WRITE,
};



struct spair {
	u32_t magic;
	int remote;
	struct k_pipe recv_q;
	struct k_poll_signal signal;
	struct k_poll_event events[1];
	bool blocking :1;
	u8_t __aligned(4) buf[CONFIG_NET_SOCKETPAIR_BUFFER_SIZE];
};

static const struct socket_op_vtable spair_fd_op_vtable;

static void spair_init(struct spair *spair) {

	spair->magic = SPAIR_MAGIC;
	spair->remote = -1;

	k_pipe_init(&spair->recv_q, spair->buf, sizeof(spair->buf));

	k_poll_signal_init(&spair->signal);

	k_poll_event_init(&spair->events[0],
		K_POLL_TYPE_SIGNAL,
		K_POLL_MODE_NOTIFY_ONLY,
		&spair->signal);

	spair->blocking = true;
}

static void spair_fini(struct spair *spair)
{
	memset(spair, 0, sizeof(*spair));
	k_free(spair);
}

static size_t k_pipe_write_avail(struct k_pipe *pipe)
{
	if (pipe->write_index >= pipe->read_index) {
		return pipe->size - (pipe->write_index - pipe->read_index);
	}

	return pipe->read_index - pipe->write_index;
}

static size_t k_pipe_read_avail(struct k_pipe *pipe)
{
	if (pipe->read_index >= pipe->write_index) {
		return pipe->size - (pipe->read_index - pipe->write_index);
	}

	return pipe->write_index - pipe->read_index;
}

int z_impl_zsock_socketpair(int family, int type, int proto, int *sv)
{
	int res;
	int tmp[2] = {-1, -1};
	struct spair *obj[2] = {};

	if (family != AF_UNIX) {
		errno = EAFNOSUPPORT;
		res = -1;
		goto out;
	}

	if (!(type == SOCK_STREAM || type == SOCK_RAW)) {
		errno = EPROTOTYPE;
		res = -1;
		goto out;
	}

	if (proto != 0) {
		errno = EPROTONOSUPPORT;
		res = -1;
		goto out;
	}

	if (sv == NULL) {
		/* not listed in the normative standard, but probably safe */
		errno = EINVAL;
		res = -1;
		goto out;
	}

	for(size_t i = 0; i < 2; ++i) {

		tmp[i] = z_reserve_fd();
		if (tmp[i] == -1) {
			errno = ENFILE;
			res = -1;
			goto cleanup;
		}
		D("reserved fd %d", tmp[i]);

		obj[i] = k_malloc(sizeof(*obj));
		if (obj[i] == NULL) {
			errno = ENOMEM;
			res = -1;
			goto cleanup;
		}
		memset(obj[i], 0, sizeof(*(obj[i])));
		D("allocated spair[%u] at %p", i, obj[i]);

		spair_init(obj[i]);
		D("initialized spair[%u] at %p", i, obj[i]);

		z_finalize_fd(tmp[i], obj[i], (const struct fd_op_vtable *) &spair_fd_op_vtable);
		D("finalized fd: %d spair: %p vtable: %p &recv_q: %p", tmp[i], obj[i], &spair_fd_op_vtable, &obj[i]->recv_q);
	}

	for(size_t i = 0; i < 2; ++i) {
		obj[i]->remote = tmp[(!i) & 1];
		sv[i] = tmp[i];
	}

	res = 0;

	goto out;

cleanup:
	for(size_t i = 0; i < 2; ++i) {
		if (obj[i] != NULL) {
			spair_fini(obj[i]);
			obj[i] = NULL;
		}
	}

	for(size_t i = 0; i < 2; ++i) {
		if (tmp[i] != -1) {
			z_free_fd(tmp[i]);
			tmp[i] = -1;
		}
	}

out:
	return res;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_socketpair(int family, int type, int proto, int *sv)
{
	int ret;
	int tmp[2];

	ret = z_impl_zsock_socketpair(family, type, proto, tmp);
	Z_OOPS(z_user_to_copy(sv, tmp, sizeof(tmp));
	return ret;
}
#include <syscalls/zsock_spair_mrsh.c>
#endif /* CONFIG_USERSPACE */


static ssize_t spair_read(void *obj, void *buffer, size_t count)
{
	struct spair *const spair = (struct spair *)obj;

	if ( spair->magic != SPAIR_MAGIC ) {
		D(
			"invalid magic for struct spair * at %p:\n"
			"actual:   %u\n"
			"expected: %u",
			spair,
			spair->magic,
			SPAIR_MAGIC
		);
	}
	__ASSERT(spair->magic == SPAIR_MAGIC, "");

	int res;

	size_t bytes_read;

	if (count == 0) {
		return 0;
	}

	D("calling k_pipe_get(%p, %p, %u)", &spair->recv_q, buffer, count);
	D("recv_q: buffer: %p size: %u bytes_used: %u read_index: %u write_index: %u flags: %u",
		spair->recv_q.buffer, spair->recv_q.size, spair->recv_q.bytes_used,
		spair->recv_q.read_index, spair->recv_q.write_index, spair->recv_q.flags);
	res = k_pipe_get(&spair->recv_q, (void *)buffer, count, & bytes_read, 0, K_NO_WAIT);
	D("k_pipe_get() returned %d", res);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	D("read %u bytes", bytes_read);

	return bytes_read;
}

static ssize_t spair_write(void *obj, const void *buffer, size_t count)
{
	struct spair *const spair = (struct spair *)obj;
	struct spair *const remote = z_get_fd_obj(spair->remote, (const struct fd_op_vtable *) &spair_fd_op_vtable, 0);

	int res;
	size_t bytes_written;

	if ( spair->magic != SPAIR_MAGIC ) {
		D(
			"invalid magic for struct spair * at %p:\n"
			"actual:   %u\n"
			"expected: %u",
			spair,
			spair->magic,
			SPAIR_MAGIC
		);
	}
	__ASSERT(spair->magic == SPAIR_MAGIC, "");

	if (remote == NULL) {
		errno = EPIPE;
		return -1;
	}

	if (count == 0) {
		return 0;
	}

	D("calling k_pipe_put(%p, %p, %u)", &remote->recv_q, buffer, count);
	D("recv_q: buffer: %p size: %u bytes_used: %u read_index: %u write_index: %u flags: %u",
		remote->recv_q.buffer, remote->recv_q.size, remote->recv_q.bytes_used,
		remote->recv_q.read_index, remote->recv_q.write_index, remote->recv_q.flags);
	res = k_pipe_put(&remote->recv_q, (void *)buffer, count, & bytes_written, 0, K_NO_WAIT);
	D("k_pipe_put() returned %d", res);
	if (res < 0) {
		errno = -res;
		return -1;
	}
	D("wrote %u bytes", bytes_written);

	return bytes_written;
}

static int spair_ioctl(void *obj, unsigned int request, va_list args)
{

	struct spair *const spair = (struct spair *)obj;

	if ( spair->magic != SPAIR_MAGIC ) {
		D(
			"invalid magic for struct spair * at %p:\n"
			"actual:   %u\n"
			"expected: %u",
			spair,
			spair->magic,
			SPAIR_MAGIC
		);
	}
	__ASSERT(spair->magic == SPAIR_MAGIC, "");

	switch (request) {
	case ZFD_IOCTL_CLOSE:
		spair_fini((struct spair *)obj);
		return 0;

	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static int spair_bind(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EISCONN;
	return -1;
}

static int spair_connect(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EISCONN;
	return -1;
}

static int spair_listen(void *obj, int backlog)
{
	(void) obj;
	(void) backlog;

	errno = EINVAL;
	return -1;
}

static int spair_accept(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	(void) obj;
	(void) addr;
	(void) addrlen;

	errno = EOPNOTSUPP;
	return -1;
}

static ssize_t spair_sendto(void *obj, const void *buf, size_t len,
				 int flags, const struct sockaddr *dest_addr,
				 socklen_t addrlen)
{
	(void) flags;
	(void) dest_addr;
	(void) addrlen;

	return spair_write(obj, buf, len);
}

static ssize_t spair_sendmsg(void *obj, const struct msghdr *msg,
				  int flags)
{
	(void) obj;
	(void) msg;
	(void) flags;

	errno = ENOSYS;
	return -1;
}

static ssize_t spair_recvfrom(void *obj, void *buf, size_t max_len,
				   int flags, struct sockaddr *src_addr,
				   socklen_t *addrlen)
{
	(void) flags;
	(void) src_addr;
	(void) addrlen;

	return spair_read(obj, buf, max_len);
}

static int spair_getsockopt(void *obj, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	(void) obj;
	(void) level;
	(void) optname;
	(void) optval;
	(void) optlen;

	errno = ENOSYS;
	return -1;
}

static int spair_setsockopt(void *obj, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	(void) obj;
	(void) level;
	(void) optname;
	(void) optval;
	(void) optlen;

	errno = ENOSYS;
	return -1;
}

static const struct socket_op_vtable spair_fd_op_vtable = {
	.fd_vtable = {
		.read = spair_read,
		.write = spair_write,
		.ioctl = spair_ioctl,
	},
	.bind = spair_bind,
	.connect = spair_connect,
	.listen = spair_listen,
	.accept = spair_accept,
	.sendto = spair_sendto,
	.sendmsg = spair_sendmsg,
	.recvfrom = spair_recvfrom,
	.getsockopt = spair_getsockopt,
	.setsockopt = spair_setsockopt,
};
