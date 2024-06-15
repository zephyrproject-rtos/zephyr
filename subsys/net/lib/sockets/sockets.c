/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Zephyr headers */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/net/socket.h>
#include <zephyr/internal/syscall_handler.h>

#include "sockets_internal.h"

#define VTABLE_CALL(fn, sock, ...)			     \
	({						     \
		const struct socket_op_vtable *vtable;	     \
		struct k_mutex *lock;			     \
		void *obj;				     \
		int retval;				     \
							     \
		obj = get_sock_vtable(sock, &vtable, &lock); \
		if (obj == NULL) {			     \
			errno = EBADF;			     \
			return -1;			     \
		}					     \
							     \
		if (vtable->fn == NULL) {		     \
			errno = EOPNOTSUPP;		     \
			return -1;			     \
		}					     \
							     \
		(void)k_mutex_lock(lock, K_FOREVER);         \
							     \
		retval = vtable->fn(obj, __VA_ARGS__);	     \
							     \
		k_mutex_unlock(lock);                        \
							     \
		retval;					     \
	})

static inline void *get_sock_vtable(int sock,
				    const struct socket_op_vtable **vtable,
				    struct k_mutex **lock)
{
	void *ctx;

	ctx = zvfs_get_fd_obj_and_vtable(sock,
				      (const struct fd_op_vtable **)vtable,
				      lock);

#ifdef CONFIG_USERSPACE
	if (ctx != NULL && k_is_in_user_syscall()) {
		if (!k_object_is_valid(ctx, K_OBJ_NET_SOCKET)) {
			/* Invalidate the context, the caller doesn't have
			 * sufficient permission or there was some other
			 * problem with the net socket object
			 */
			ctx = NULL;
		}
	}
#endif /* CONFIG_USERSPACE */

	if (ctx == NULL) {
		NET_DBG("Invalid access on sock %d by thread %p (%s)", sock,
			_current, k_thread_name_get(_current));
	}

	return ctx;
}

size_t msghdr_non_empty_iov_count(const struct msghdr *msg)
{
	size_t non_empty_iov_count = 0;

	for (size_t i = 0; i < msg->msg_iovlen; i++) {
		if (msg->msg_iov[i].iov_len) {
			non_empty_iov_count++;
		}
	}

	return non_empty_iov_count;
}

void *z_impl_zsock_get_context_object(int sock)
{
	const struct socket_op_vtable *ignored;

	return get_sock_vtable(sock, &ignored, NULL);
}

#ifdef CONFIG_USERSPACE
void *z_vrfy_zsock_get_context_object(int sock)
{
	/* All checking done in implementation */
	return z_impl_zsock_get_context_object(sock);
}

#include <zephyr/syscalls/zsock_get_context_object_mrsh.c>
#endif

int z_impl_zsock_socket(int family, int type, int proto)
{
	STRUCT_SECTION_FOREACH(net_socket_register, sock_family) {
		int ret;

		if (sock_family->family != family &&
		    sock_family->family != AF_UNSPEC) {
			continue;
		}

		NET_ASSERT(sock_family->is_supported);

		if (!sock_family->is_supported(family, type, proto)) {
			continue;
		}

		errno = 0;
		ret = sock_family->handler(family, type, proto);

		SYS_PORT_TRACING_OBJ_INIT(socket, ret < 0 ? -errno : ret,
					  family, type, proto);

		(void)sock_obj_core_alloc(ret, sock_family, family, type, proto);

		return ret;
	}

	errno = EAFNOSUPPORT;
	SYS_PORT_TRACING_OBJ_INIT(socket, -errno, family, type, proto);
	return -1;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_socket(int family, int type, int proto)
{
	/* implementation call to net_context_get() should do all necessary
	 * checking
	 */
	return z_impl_zsock_socket(family, type, proto);
}
#include <zephyr/syscalls/zsock_socket_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_close(int sock)
{
	const struct socket_op_vtable *vtable;
	struct k_mutex *lock;
	void *ctx;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, close, sock);

	ctx = get_sock_vtable(sock, &vtable, &lock);
	if (ctx == NULL) {
		errno = EBADF;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, close, sock, -errno);
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	NET_DBG("close: ctx=%p, fd=%d", ctx, sock);

	ret = vtable->fd_vtable.close(ctx);

	k_mutex_unlock(lock);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, close, sock, ret < 0 ? -errno : ret);

	zvfs_free_fd(sock);

	(void)sock_obj_core_dealloc(sock);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_close(int sock)
{
	return z_impl_zsock_close(sock);
}
#include <zephyr/syscalls/zsock_close_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_shutdown(int sock, int how)
{
	const struct socket_op_vtable *vtable;
	struct k_mutex *lock;
	void *ctx;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, shutdown, sock, how);

	ctx = get_sock_vtable(sock, &vtable, &lock);
	if (ctx == NULL) {
		errno = EBADF;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, shutdown, sock, -errno);
		return -1;
	}

	if (!vtable->shutdown) {
		errno = ENOTSUP;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, shutdown, sock, -errno);
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	NET_DBG("shutdown: ctx=%p, fd=%d, how=%d", ctx, sock, how);

	ret = vtable->shutdown(ctx, how);

	k_mutex_unlock(lock);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, shutdown, sock, ret < 0 ? -errno : ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_shutdown(int sock, int how)
{
	return z_impl_zsock_shutdown(sock, how);
}
#include <zephyr/syscalls/zsock_shutdown_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, bind, sock, addr, addrlen);

	ret = VTABLE_CALL(bind, sock, addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, bind, sock, ret < 0 ? -errno : ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_bind(int sock, const struct sockaddr *addr,
				    socklen_t addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	K_OOPS(K_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
	K_OOPS(k_usermode_from_copy(&dest_addr_copy, (void *)addr, addrlen));

	return z_impl_zsock_bind(sock, (struct sockaddr *)&dest_addr_copy,
				addrlen);
}
#include <zephyr/syscalls/zsock_bind_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, connect, sock, addr, addrlen);

	ret = VTABLE_CALL(connect, sock, addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, connect, sock,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_zsock_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	K_OOPS(K_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
	K_OOPS(k_usermode_from_copy(&dest_addr_copy, (void *)addr, addrlen));

	return z_impl_zsock_connect(sock, (struct sockaddr *)&dest_addr_copy,
				   addrlen);
}
#include <zephyr/syscalls/zsock_connect_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_listen(int sock, int backlog)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, listen, sock, backlog);

	ret = VTABLE_CALL(listen, sock, backlog);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, listen, sock,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_listen(int sock, int backlog)
{
	return z_impl_zsock_listen(sock, backlog);
}
#include <zephyr/syscalls/zsock_listen_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	int new_sock;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, accept, sock);

	new_sock = VTABLE_CALL(accept, sock, addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, accept, new_sock, addr, addrlen,
				       new_sock < 0 ? -errno : 0);

	(void)sock_obj_core_alloc_find(sock, new_sock, SOCK_STREAM);

	return new_sock;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_accept(int sock, struct sockaddr *addr,
				      socklen_t *addrlen)
{
	socklen_t addrlen_copy;
	int ret;

	K_OOPS(addrlen && k_usermode_from_copy(&addrlen_copy, addrlen,
					   sizeof(socklen_t)));
	K_OOPS(addr && K_SYSCALL_MEMORY_WRITE(addr, addrlen ? addrlen_copy : 0));

	ret = z_impl_zsock_accept(sock, (struct sockaddr *)addr,
				  addrlen ? &addrlen_copy : NULL);

	K_OOPS(ret >= 0 && addrlen && k_usermode_to_copy(addrlen, &addrlen_copy,
						     sizeof(socklen_t)));

	return ret;
}
#include <zephyr/syscalls/zsock_accept_mrsh.c>
#endif /* CONFIG_USERSPACE */

ssize_t z_impl_zsock_sendto(int sock, const void *buf, size_t len, int flags,
			   const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int bytes_sent;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, sendto, sock, len, flags,
					dest_addr, addrlen);

	bytes_sent = VTABLE_CALL(sendto, sock, buf, len, flags, dest_addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, sendto, sock,
				       bytes_sent < 0 ? -errno : bytes_sent);

	sock_obj_core_update_send_stats(sock, bytes_sent);

	return bytes_sent;
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_zsock_sendto(int sock, const void *buf, size_t len, int flags,
			   const struct sockaddr *dest_addr, socklen_t addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	K_OOPS(K_SYSCALL_MEMORY_READ(buf, len));
	if (dest_addr) {
		K_OOPS(K_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
		K_OOPS(k_usermode_from_copy(&dest_addr_copy, (void *)dest_addr,
					addrlen));
	}

	return z_impl_zsock_sendto(sock, (const void *)buf, len, flags,
			dest_addr ? (struct sockaddr *)&dest_addr_copy : NULL,
			addrlen);
}
#include <zephyr/syscalls/zsock_sendto_mrsh.c>
#endif /* CONFIG_USERSPACE */

ssize_t z_impl_zsock_sendmsg(int sock, const struct msghdr *msg, int flags)
{
	int bytes_sent;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, sendmsg, sock, msg, flags);

	bytes_sent = VTABLE_CALL(sendmsg, sock, msg, flags);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, sendmsg, sock,
				       bytes_sent < 0 ? -errno : bytes_sent);

	sock_obj_core_update_send_stats(sock, bytes_sent);

	return bytes_sent;
}

#ifdef CONFIG_USERSPACE
static inline ssize_t z_vrfy_zsock_sendmsg(int sock,
					   const struct msghdr *msg,
					   int flags)
{
	struct msghdr msg_copy;
	size_t i;
	int ret;

	K_OOPS(k_usermode_from_copy(&msg_copy, (void *)msg, sizeof(msg_copy)));

	msg_copy.msg_name = NULL;
	msg_copy.msg_control = NULL;

	msg_copy.msg_iov = k_usermode_alloc_from_copy(msg->msg_iov,
				       msg->msg_iovlen * sizeof(struct iovec));
	if (!msg_copy.msg_iov) {
		errno = ENOMEM;
		goto fail;
	}

	for (i = 0; i < msg->msg_iovlen; i++) {
		msg_copy.msg_iov[i].iov_base =
			k_usermode_alloc_from_copy(msg->msg_iov[i].iov_base,
					       msg->msg_iov[i].iov_len);
		if (!msg_copy.msg_iov[i].iov_base) {
			errno = ENOMEM;
			goto fail;
		}

		msg_copy.msg_iov[i].iov_len = msg->msg_iov[i].iov_len;
	}

	if (msg->msg_namelen > 0) {
		msg_copy.msg_name = k_usermode_alloc_from_copy(msg->msg_name,
							   msg->msg_namelen);
		if (!msg_copy.msg_name) {
			errno = ENOMEM;
			goto fail;
		}
	}

	if (msg->msg_controllen > 0) {
		msg_copy.msg_control = k_usermode_alloc_from_copy(msg->msg_control,
							  msg->msg_controllen);
		if (!msg_copy.msg_control) {
			errno = ENOMEM;
			goto fail;
		}
	}

	ret = z_impl_zsock_sendmsg(sock, (const struct msghdr *)&msg_copy,
				   flags);

	k_free(msg_copy.msg_name);
	k_free(msg_copy.msg_control);

	for (i = 0; i < msg_copy.msg_iovlen; i++) {
		k_free(msg_copy.msg_iov[i].iov_base);
	}

	k_free(msg_copy.msg_iov);

	return ret;

fail:
	if (msg_copy.msg_name) {
		k_free(msg_copy.msg_name);
	}

	if (msg_copy.msg_control) {
		k_free(msg_copy.msg_control);
	}

	if (msg_copy.msg_iov) {
		for (i = 0; i < msg_copy.msg_iovlen; i++) {
			if (msg_copy.msg_iov[i].iov_base) {
				k_free(msg_copy.msg_iov[i].iov_base);
			}
		}

		k_free(msg_copy.msg_iov);
	}

	return -1;
}
#include <zephyr/syscalls/zsock_sendmsg_mrsh.c>
#endif /* CONFIG_USERSPACE */

ssize_t z_impl_zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
			     struct sockaddr *src_addr, socklen_t *addrlen)
{
	int bytes_received;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, recvfrom, sock, max_len, flags, src_addr, addrlen);

	bytes_received = VTABLE_CALL(recvfrom, sock, buf, max_len, flags, src_addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, recvfrom, sock,
				       src_addr, addrlen,
				       bytes_received < 0 ? -errno : bytes_received);

	sock_obj_core_update_recv_stats(sock, bytes_received);

	return bytes_received;
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
			      struct sockaddr *src_addr, socklen_t *addrlen)
{
	socklen_t addrlen_copy;
	ssize_t ret;

	if (K_SYSCALL_MEMORY_WRITE(buf, max_len)) {
		errno = EFAULT;
		return -1;
	}

	if (addrlen) {
		K_OOPS(k_usermode_from_copy(&addrlen_copy, addrlen,
					sizeof(socklen_t)));
	}
	K_OOPS(src_addr && K_SYSCALL_MEMORY_WRITE(src_addr, addrlen_copy));

	ret = z_impl_zsock_recvfrom(sock, (void *)buf, max_len, flags,
				   (struct sockaddr *)src_addr,
				   addrlen ? &addrlen_copy : NULL);

	if (addrlen) {
		K_OOPS(k_usermode_to_copy(addrlen, &addrlen_copy,
				      sizeof(socklen_t)));
	}

	return ret;
}
#include <zephyr/syscalls/zsock_recvfrom_mrsh.c>
#endif /* CONFIG_USERSPACE */

ssize_t z_impl_zsock_recvmsg(int sock, struct msghdr *msg, int flags)
{
	int bytes_received;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, recvmsg, sock, msg, flags);

	bytes_received = VTABLE_CALL(recvmsg, sock, msg, flags);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, recvmsg, sock, msg,
				       bytes_received < 0 ? -errno : bytes_received);

	sock_obj_core_update_recv_stats(sock, bytes_received);

	return bytes_received;
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_zsock_recvmsg(int sock, struct msghdr *msg, int flags)
{
	struct msghdr msg_copy;
	size_t iovlen;
	size_t i;
	int ret;

	if (msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (msg->msg_iov == NULL) {
		errno = ENOMEM;
		return -1;
	}

	K_OOPS(k_usermode_from_copy(&msg_copy, (void *)msg, sizeof(msg_copy)));

	k_usermode_from_copy(&iovlen, &msg->msg_iovlen, sizeof(iovlen));

	msg_copy.msg_name = NULL;
	msg_copy.msg_control = NULL;

	msg_copy.msg_iov = k_usermode_alloc_from_copy(msg->msg_iov,
				       msg->msg_iovlen * sizeof(struct iovec));
	if (!msg_copy.msg_iov) {
		errno = ENOMEM;
		goto fail;
	}

	/* Clear the pointers in the copy so that if the allocation in the
	 * next loop fails, we do not try to free non allocated memory
	 * in fail branch.
	 */
	memset(msg_copy.msg_iov, 0, msg->msg_iovlen * sizeof(struct iovec));

	for (i = 0; i < iovlen; i++) {
		/* TODO: In practice we do not need to copy the actual data
		 * in msghdr when receiving data but currently there is no
		 * ready made function to do just that (unless we want to call
		 * relevant malloc function here ourselves). So just use
		 * the copying variant for now.
		 */
		msg_copy.msg_iov[i].iov_base =
			k_usermode_alloc_from_copy(msg->msg_iov[i].iov_base,
						   msg->msg_iov[i].iov_len);
		if (!msg_copy.msg_iov[i].iov_base) {
			errno = ENOMEM;
			goto fail;
		}

		msg_copy.msg_iov[i].iov_len = msg->msg_iov[i].iov_len;
	}

	if (msg->msg_namelen > 0) {
		if (msg->msg_name == NULL) {
			errno = EINVAL;
			goto fail;
		}

		msg_copy.msg_name = k_usermode_alloc_from_copy(msg->msg_name,
							   msg->msg_namelen);
		if (msg_copy.msg_name == NULL) {
			errno = ENOMEM;
			goto fail;
		}
	}

	if (msg->msg_controllen > 0) {
		if (msg->msg_control == NULL) {
			errno = EINVAL;
			goto fail;
		}

		msg_copy.msg_control =
			k_usermode_alloc_from_copy(msg->msg_control,
						   msg->msg_controllen);
		if (msg_copy.msg_control == NULL) {
			errno = ENOMEM;
			goto fail;
		}
	}

	ret = z_impl_zsock_recvmsg(sock, &msg_copy, flags);

	/* Do not copy anything back if there was an error or nothing was
	 * received.
	 */
	if (ret > 0) {
		if (msg->msg_namelen > 0 && msg->msg_name != NULL) {
			K_OOPS(k_usermode_to_copy(msg->msg_name,
						  msg_copy.msg_name,
						  msg_copy.msg_namelen));
		}

		if (msg->msg_controllen > 0 &&
		    msg->msg_control != NULL) {
			K_OOPS(k_usermode_to_copy(msg->msg_control,
						  msg_copy.msg_control,
						  msg_copy.msg_controllen));

			msg->msg_controllen = msg_copy.msg_controllen;
		} else {
			msg->msg_controllen = 0U;
		}

		k_usermode_to_copy(&msg->msg_iovlen,
				   &msg_copy.msg_iovlen,
				   sizeof(msg->msg_iovlen));

		/* The new iovlen cannot be bigger than the original one */
		NET_ASSERT(msg_copy.msg_iovlen <= iovlen);

		for (i = 0; i < iovlen; i++) {
			if (i < msg_copy.msg_iovlen) {
				K_OOPS(k_usermode_to_copy(msg->msg_iov[i].iov_base,
							  msg_copy.msg_iov[i].iov_base,
							  msg_copy.msg_iov[i].iov_len));
				K_OOPS(k_usermode_to_copy(&msg->msg_iov[i].iov_len,
							  &msg_copy.msg_iov[i].iov_len,
							  sizeof(msg->msg_iov[i].iov_len)));
			} else {
				/* Clear out those vectors that we could not populate */
				msg->msg_iov[i].iov_len = 0;
			}
		}

		k_usermode_to_copy(&msg->msg_flags,
				   &msg_copy.msg_flags,
				   sizeof(msg->msg_flags));
	}

	k_free(msg_copy.msg_name);
	k_free(msg_copy.msg_control);

	/* Note that we need to free according to original iovlen */
	for (i = 0; i < iovlen; i++) {
		k_free(msg_copy.msg_iov[i].iov_base);
	}

	k_free(msg_copy.msg_iov);

	return ret;

fail:
	if (msg_copy.msg_name) {
		k_free(msg_copy.msg_name);
	}

	if (msg_copy.msg_control) {
		k_free(msg_copy.msg_control);
	}

	if (msg_copy.msg_iov) {
		for (i = 0; i < msg_copy.msg_iovlen; i++) {
			if (msg_copy.msg_iov[i].iov_base) {
				k_free(msg_copy.msg_iov[i].iov_base);
			}
		}

		k_free(msg_copy.msg_iov);
	}

	return -1;
}
#include <zephyr/syscalls/zsock_recvmsg_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* As this is limited function, we don't follow POSIX signature, with
 * "..." instead of last arg.
 */
int z_impl_zsock_fcntl_impl(int sock, int cmd, int flags)
{
	const struct socket_op_vtable *vtable;
	struct k_mutex *lock;
	void *obj;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, fcntl, sock, cmd, flags);

	obj = get_sock_vtable(sock, &vtable, &lock);
	if (obj == NULL) {
		errno = EBADF;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, fcntl, sock, -errno);
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	ret = zvfs_fdtable_call_ioctl((const struct fd_op_vtable *)vtable,
				   obj, cmd, flags);

	k_mutex_unlock(lock);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, fcntl, sock,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_fcntl_impl(int sock, int cmd, int flags)
{
	return z_impl_zsock_fcntl_impl(sock, cmd, flags);
}
#include <zephyr/syscalls/zsock_fcntl_impl_mrsh.c>
#endif

int z_impl_zsock_ioctl_impl(int sock, unsigned long request, va_list args)
{
	const struct socket_op_vtable *vtable;
	struct k_mutex *lock;
	void *ctx;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, ioctl, sock, request);

	ctx = get_sock_vtable(sock, &vtable, &lock);
	if (ctx == NULL) {
		errno = EBADF;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, ioctl, sock, -errno);
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	NET_DBG("ioctl: ctx=%p, fd=%d, request=%lu", ctx, sock, request);

	ret = vtable->fd_vtable.ioctl(ctx, request, args);

	k_mutex_unlock(lock);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, ioctl, sock,
				       ret < 0 ? -errno : ret);
	return ret;

}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_ioctl_impl(int sock, unsigned long request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_FIONBIO:
		break;

	case ZFD_IOCTL_FIONREAD: {
		int *avail;

		avail = va_arg(args, int *);
		K_OOPS(K_SYSCALL_MEMORY_WRITE(avail, sizeof(*avail)));

		break;
	}

	default:
		errno = EOPNOTSUPP;
		return -1;
	}

	return z_impl_zsock_ioctl_impl(sock, request, args);
}
#include <zephyr/syscalls/zsock_ioctl_impl_mrsh.c>
#endif

int z_impl_zsock_inet_pton(sa_family_t family, const char *src, void *dst)
{
	if (net_addr_pton(family, src, dst) == 0) {
		return 1;
	} else {
		return 0;
	}
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_inet_pton(sa_family_t family,
					 const char *src, void *dst)
{
	int dst_size;
	char src_copy[NET_IPV6_ADDR_LEN];
	char dst_copy[sizeof(struct in6_addr)];
	int ret;

	switch (family) {
	case AF_INET:
		dst_size = sizeof(struct in_addr);
		break;
	case AF_INET6:
		dst_size = sizeof(struct in6_addr);
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}

	K_OOPS(k_usermode_string_copy(src_copy, (char *)src, sizeof(src_copy)));
	ret = z_impl_zsock_inet_pton(family, src_copy, dst_copy);
	K_OOPS(k_usermode_to_copy(dst, dst_copy, dst_size));

	return ret;
}
#include <zephyr/syscalls/zsock_inet_pton_mrsh.c>
#endif

int z_impl_zsock_getsockopt(int sock, int level, int optname,
			    void *optval, socklen_t *optlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, getsockopt, sock, level, optname);

	ret = VTABLE_CALL(getsockopt, sock, level, optname, optval, optlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, getsockopt, sock, level, optname,
				       optval, *optlen, ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_zsock_getsockopt(int sock, int level, int optname,
			    void *optval, socklen_t *optlen)
{
	socklen_t kernel_optlen = *(socklen_t *)optlen;
	void *kernel_optval;
	int ret;

	if (K_SYSCALL_MEMORY_WRITE(optval, kernel_optlen)) {
		errno = -EPERM;
		return -1;
	}

	kernel_optval = k_usermode_alloc_from_copy((const void *)optval,
					       kernel_optlen);
	K_OOPS(!kernel_optval);

	ret = z_impl_zsock_getsockopt(sock, level, optname,
				      kernel_optval, &kernel_optlen);

	K_OOPS(k_usermode_to_copy((void *)optval, kernel_optval, kernel_optlen));
	K_OOPS(k_usermode_to_copy((void *)optlen, &kernel_optlen,
			      sizeof(socklen_t)));

	k_free(kernel_optval);

	return ret;
}
#include <zephyr/syscalls/zsock_getsockopt_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_setsockopt(int sock, int level, int optname,
			    const void *optval, socklen_t optlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, setsockopt, sock,
					level, optname, optval, optlen);

	ret = VTABLE_CALL(setsockopt, sock, level, optname, optval, optlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, setsockopt, sock,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_zsock_setsockopt(int sock, int level, int optname,
			    const void *optval, socklen_t optlen)
{
	void *kernel_optval;
	int ret;

	kernel_optval = k_usermode_alloc_from_copy((const void *)optval, optlen);
	K_OOPS(!kernel_optval);

	ret = z_impl_zsock_setsockopt(sock, level, optname,
				      kernel_optval, optlen);

	k_free(kernel_optval);

	return ret;
}
#include <zephyr/syscalls/zsock_setsockopt_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_getpeername(int sock, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, getpeername, sock);

	ret = VTABLE_CALL(getpeername, sock, addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, getpeername, sock,
				       addr, addrlen,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_getpeername(int sock, struct sockaddr *addr,
					   socklen_t *addrlen)
{
	socklen_t addrlen_copy;
	int ret;

	K_OOPS(k_usermode_from_copy(&addrlen_copy, (void *)addrlen,
				sizeof(socklen_t)));

	if (K_SYSCALL_MEMORY_WRITE(addr, addrlen_copy)) {
		errno = EFAULT;
		return -1;
	}

	ret = z_impl_zsock_getpeername(sock, (struct sockaddr *)addr,
				       &addrlen_copy);

	if (ret == 0 &&
	    k_usermode_to_copy((void *)addrlen, &addrlen_copy,
			   sizeof(socklen_t))) {
		errno = EINVAL;
		return -1;
	}

	return ret;
}
#include <zephyr/syscalls/zsock_getpeername_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zsock_getsockname(int sock, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, getsockname, sock);

	ret = VTABLE_CALL(getsockname, sock, addr, addrlen);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, getsockname, sock,
				       addr, addrlen,
				       ret < 0 ? -errno : ret);
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_getsockname(int sock, struct sockaddr *addr,
					   socklen_t *addrlen)
{
	socklen_t addrlen_copy;
	int ret;

	K_OOPS(k_usermode_from_copy(&addrlen_copy, (void *)addrlen,
				sizeof(socklen_t)));

	if (K_SYSCALL_MEMORY_WRITE(addr, addrlen_copy)) {
		errno = EFAULT;
		return -1;
	}

	ret = z_impl_zsock_getsockname(sock, (struct sockaddr *)addr,
				       &addrlen_copy);

	if (ret == 0 &&
	    k_usermode_to_copy((void *)addrlen, &addrlen_copy,
			   sizeof(socklen_t))) {
		errno = EINVAL;
		return -1;
	}

	return ret;
}
#include <zephyr/syscalls/zsock_getsockname_mrsh.c>
#endif /* CONFIG_USERSPACE */
