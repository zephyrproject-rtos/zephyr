/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/fdtable.h>

#include "sockets_internal.h"

enum {
	SPAIR_SIG_CANCEL, /**< operation has been canceled */
	SPAIR_SIG_DATA,   /**< @ref spair.recv_q has been updated */
};

enum {
	SPAIR_FLAG_NONBLOCK = (1 << 0), /**< socket is non-blocking */
};

#define SPAIR_FLAGS_DEFAULT 0

/**
 * Socketpair endpoint structure
 *
 * This structure represents one half of a socketpair (an 'endpoint').
 *
 * The implementation strives for compatibility with socketpair(2).
 *
 * Resources contained within this structure are said to be 'local', while
 * resources contained within the other half of the socketpair (or other
 * endpoint) are said to be 'remote'.
 *
 * Theory of operation:
 * - each end of a socketpair owns a @a recv_q
 * - since there is no write queue, data is either written or not
 * - read and write operations may return partial transfers
 * - read operations may block if the local @a recv_q is empty
 * - write operations may block if the remote @a recv_q is full
 * - each endpoint may be blocking or non-blocking
 */
__net_socket struct spair {
	int remote; /**< the remote endpoint file descriptor */
	uint32_t flags; /**< status and option bits */
	struct k_sem sem; /**< semaphore for exclusive structure access */
	struct k_pipe recv_q; /**< receive queue of local endpoint */
	/** indicates local @a recv_q isn't empty */
	struct k_poll_signal readable;
	/** indicates local @a recv_q isn't full */
	struct k_poll_signal writeable;
	/** buffer for @a recv_q recv_q */
	uint8_t buf[CONFIG_NET_SOCKETPAIR_BUFFER_SIZE];
};

#ifdef CONFIG_NET_SOCKETPAIR_STATIC
K_MEM_SLAB_DEFINE_STATIC(spair_slab, sizeof(struct spair), CONFIG_NET_SOCKETPAIR_MAX * 2,
			 __alignof__(struct spair));
#endif /* CONFIG_NET_SOCKETPAIR_STATIC */

/* forward declaration */
static const struct socket_op_vtable spair_fd_op_vtable;

#undef sock_is_nonblock
/** Determine if a @ref spair is in non-blocking mode */
static inline bool sock_is_nonblock(const struct spair *spair)
{
	return !!(spair->flags & SPAIR_FLAG_NONBLOCK);
}

/** Determine if a @ref spair is connected */
static inline bool sock_is_connected(const struct spair *spair)
{
	const struct spair *remote = zvfs_get_fd_obj(spair->remote,
		(const struct fd_op_vtable *)&spair_fd_op_vtable, 0);

	if (remote == NULL) {
		return false;
	}

	return true;
}

#undef sock_is_eof
/** Determine if a @ref spair has encountered end-of-file */
static inline bool sock_is_eof(const struct spair *spair)
{
	return !sock_is_connected(spair);
}

/**
 * Determine bytes available to write
 *
 * Specifically, this function calculates the number of bytes that may be
 * written to a given @ref spair without blocking.
 */
static inline size_t spair_write_avail(struct spair *spair)
{
	struct spair *const remote = zvfs_get_fd_obj(spair->remote,
		(const struct fd_op_vtable *)&spair_fd_op_vtable, 0);

	if (remote == NULL) {
		return 0;
	}

	return k_pipe_write_avail(&remote->recv_q);
}

/**
 * Determine bytes available to read
 *
 * Specifically, this function calculates the number of bytes that may be
 * read from a given @ref spair without blocking.
 */
static inline size_t spair_read_avail(struct spair *spair)
{
	return k_pipe_read_avail(&spair->recv_q);
}

/** Swap two 32-bit integers */
static inline void swap32(uint32_t *a, uint32_t *b)
{
	uint32_t c;

	c = *b;
	*b = *a;
	*a = c;
}

/**
 * Delete @param spair
 *
 * This function deletes one endpoint of a socketpair.
 *
 * Theory of operation:
 * - we have a socketpair with two endpoints: A and B
 * - we have two threads: T1 and T2
 * - T1 operates on endpoint A
 * - T2 operates on endpoint B
 *
 * There are two possible cases where a blocking operation must be notified
 * when one endpoint is closed:
 * -# T1 is blocked reading from A and T2 closes B
 *    T1 waits on A's write signal. T2 triggers the remote
 *    @ref spair.readable
 * -# T1 is blocked writing to A and T2 closes B
 *    T1 is waits on B's read signal. T2 triggers the local
 *    @ref spair.writeable.
 *
 * If the remote endpoint is already closed, the former operation does not
 * take place. Otherwise, the @ref spair.remote of the local endpoint is
 * set to -1.
 *
 * If no threads are blocking on A, then the signals have no effect.
 *
 * The memory associated with the local endpoint is cleared and freed.
 */
static void spair_delete(struct spair *spair)
{
	int res;
	struct spair *remote = NULL;
	bool have_remote_sem = false;

	if (spair == NULL) {
		return;
	}

	if (spair->remote != -1) {
		remote = zvfs_get_fd_obj(spair->remote,
			(const struct fd_op_vtable *)&spair_fd_op_vtable, 0);

		if (remote != NULL) {
			res = k_sem_take(&remote->sem, K_FOREVER);
			if (res == 0) {
				have_remote_sem = true;
				remote->remote = -1;
				res = k_poll_signal_raise(&remote->readable,
					SPAIR_SIG_CANCEL);
				__ASSERT(res == 0,
					"k_poll_signal_raise() failed: %d",
					res);
			}
		}
	}

	spair->remote = -1;

	res = k_poll_signal_raise(&spair->writeable, SPAIR_SIG_CANCEL);
	__ASSERT(res == 0, "k_poll_signal_raise() failed: %d", res);

	if (remote != NULL && have_remote_sem) {
		k_sem_give(&remote->sem);
	}

	/* ensure no private information is released to the memory pool */
	memset(spair, 0, sizeof(*spair));
#ifdef CONFIG_NET_SOCKETPAIR_STATIC
	k_mem_slab_free(&spair_slab, (void *)spair);
#elif CONFIG_USERSPACE
	k_object_free(spair);
#else
	k_free(spair);
#endif
}

/**
 * Create a @ref spair (1/2 of a socketpair)
 *
 * The idea is to call this twice, but store the "local" side in the
 * @ref spair.remote field initially.
 *
 * If both allocations are successful, then swap the @ref spair.remote
 * fields in the two @ref spair instances.
 */
static struct spair *spair_new(void)
{
	struct spair *spair;
	int res;

#ifdef CONFIG_NET_SOCKETPAIR_STATIC

	res = k_mem_slab_alloc(&spair_slab, (void **) &spair, K_NO_WAIT);
	if (res != 0) {
		spair = NULL;
	}

#elif CONFIG_USERSPACE
	struct k_object *zo = k_object_create_dynamic(sizeof(*spair));

	if (zo == NULL) {
		spair = NULL;
	} else {
		spair = zo->name;
		zo->type = K_OBJ_NET_SOCKET;
	}
#else
	spair = k_malloc(sizeof(*spair));
#endif
	if (spair == NULL) {
		errno = ENOMEM;
		goto out;
	}
	memset(spair, 0, sizeof(*spair));

	/* initialize any non-zero default values */
	spair->remote = -1;
	spair->flags = SPAIR_FLAGS_DEFAULT;

	k_sem_init(&spair->sem, 1, 1);
	k_pipe_init(&spair->recv_q, spair->buf, sizeof(spair->buf));
	k_poll_signal_init(&spair->readable);
	k_poll_signal_init(&spair->writeable);

	/* A new socket is always writeable after creation */
	res = k_poll_signal_raise(&spair->writeable, SPAIR_SIG_DATA);
	__ASSERT(res == 0, "k_poll_signal_raise() failed: %d", res);

	spair->remote = zvfs_reserve_fd();
	if (spair->remote == -1) {
		errno = ENFILE;
		goto cleanup;
	}

	zvfs_finalize_typed_fd(spair->remote, spair,
			       (const struct fd_op_vtable *)&spair_fd_op_vtable, ZVFS_MODE_IFSOCK);

	goto out;

cleanup:
	spair_delete(spair);
	spair = NULL;

out:
	return spair;
}

int z_impl_zsock_socketpair(int family, int type, int proto, int *sv)
{
	int res;
	size_t i;
	struct spair *obj[2] = {};

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, socketpair, family, type, proto, sv);

	if (family != AF_UNIX) {
		errno = EAFNOSUPPORT;
		res = -1;
		goto errout;
	}

	if (type != SOCK_STREAM) {
		errno = EPROTOTYPE;
		res = -1;
		goto errout;
	}

	if (proto != 0) {
		errno = EPROTONOSUPPORT;
		res = -1;
		goto errout;
	}

	if (sv == NULL) {
		/* not listed in normative spec, but mimics Linux behaviour */
		errno = EFAULT;
		res = -1;
		goto errout;
	}

	for (i = 0; i < 2; ++i) {
		obj[i] = spair_new();
		if (!obj[i]) {
			res = -1;
			goto cleanup;
		}
	}

	/* connect the two endpoints */
	swap32(&obj[0]->remote, &obj[1]->remote);

	for (i = 0; i < 2; ++i) {
		sv[i] = obj[i]->remote;
		k_sem_give(&obj[0]->sem);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, socketpair, sv[0], sv[1], 0);

	return 0;

cleanup:
	for (i = 0; i < 2; ++i) {
		spair_delete(obj[i]);
	}

errout:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, socketpair, -1, -1, -errno);

	return res;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_zsock_socketpair(int family, int type, int proto, int *sv)
{
	int ret;
	int tmp[2];

	if (!sv || K_SYSCALL_MEMORY_WRITE(sv, sizeof(tmp)) != 0) {
		/* not listed in normative spec, but mimics linux behaviour */
		errno = EFAULT;
		ret = -1;
		goto out;
	}

	ret = z_impl_zsock_socketpair(family, type, proto, tmp);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(sv, tmp, sizeof(tmp)));
	}

out:
	return ret;
}

#include <zephyr/syscalls/zsock_socketpair_mrsh.c>
#endif /* CONFIG_USERSPACE */

/**
 * Write data to one end of a @ref spair
 *
 * Data written on one file descriptor of a socketpair can be read at the
 * other end using common POSIX calls such as read(2) or recv(2).
 *
 * If the underlying file descriptor has the @ref O_NONBLOCK flag set then
 * this function will return immediately. If no data was written on a
 * non-blocking file descriptor, then -1 will be returned and @ref errno will
 * be set to @ref EAGAIN.
 *
 * Blocking write operations occur when the @ref O_NONBLOCK flag is @em not
 * set and there is insufficient space in the @em remote @ref spair.pipe.
 *
 * Such a blocking write will suspend execution of the current thread until
 * one of two possible results is received on the @em remote
 * @ref spair.writeable:
 *
 * 1) @ref SPAIR_SIG_DATA - data has been read from the @em remote
 *    @ref spair.pipe. Thus, allowing more data to be written.
 *
 * 2) @ref SPAIR_SIG_CANCEL - the @em remote socketpair endpoint was closed
 *    Receipt of this result is analogous to SIGPIPE from POSIX
 *    ("Write on a pipe with no one to read it."). In this case, the function
 *    will return -1 and set @ref errno to @ref EPIPE.
 *
 * @param obj the address of an @ref spair object cast to `void *`
 * @param buffer the buffer to write
 * @param count the number of bytes to write from @p buffer
 *
 * @return on success, a number > 0 representing the number of bytes written
 * @return -1 on error, with @ref errno set appropriately.
 */
static ssize_t spair_write(void *obj, const void *buffer, size_t count)
{
	int res;
	size_t avail;
	bool is_nonblock;
	size_t bytes_written;
	bool have_local_sem = false;
	bool have_remote_sem = false;
	bool will_block = false;
	struct spair *const spair = (struct spair *)obj;
	struct spair *remote = NULL;

	if (obj == NULL || buffer == NULL || count == 0) {
		errno = EINVAL;
		res = -1;
		goto out;
	}

	res = k_sem_take(&spair->sem, K_NO_WAIT);
	is_nonblock = sock_is_nonblock(spair);
	if (res < 0) {
		if (is_nonblock) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}

		res = k_sem_take(&spair->sem, K_FOREVER);
		if (res < 0) {
			errno = -res;
			res = -1;
			goto out;
		}
		is_nonblock = sock_is_nonblock(spair);
	}

	have_local_sem = true;

	remote = zvfs_get_fd_obj(spair->remote,
		(const struct fd_op_vtable *)&spair_fd_op_vtable, 0);

	if (remote == NULL) {
		errno = EPIPE;
		res = -1;
		goto out;
	}

	res = k_sem_take(&remote->sem, K_NO_WAIT);
	if (res < 0) {
		if (is_nonblock) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}
		res = k_sem_take(&remote->sem, K_FOREVER);
		if (res < 0) {
			errno = -res;
			res = -1;
			goto out;
		}
	}

	have_remote_sem = true;

	avail = spair_write_avail(spair);

	if (avail == 0) {
		if (is_nonblock) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}
		will_block = true;
	}

	if (will_block) {
		if (k_is_in_isr()) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}

		for (int signaled = false, result = -1; !signaled;
			result = -1) {

			struct k_poll_event events[] = {
				K_POLL_EVENT_INITIALIZER(
					K_POLL_TYPE_SIGNAL,
					K_POLL_MODE_NOTIFY_ONLY,
					&remote->writeable),
			};

			k_sem_give(&remote->sem);
			have_remote_sem = false;

			res = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
			if (res < 0) {
				errno = -res;
				res = -1;
				goto out;
			}

			remote = zvfs_get_fd_obj(spair->remote,
				(const struct fd_op_vtable *)
				&spair_fd_op_vtable, 0);

			if (remote == NULL) {
				errno = EPIPE;
				res = -1;
				goto out;
			}

			res = k_sem_take(&remote->sem, K_FOREVER);
			if (res < 0) {
				errno = -res;
				res = -1;
				goto out;
			}

			have_remote_sem = true;

			k_poll_signal_check(&remote->writeable, &signaled,
					    &result);
			if (!signaled) {
				continue;
			}

			switch (result) {
				case SPAIR_SIG_DATA: {
					break;
				}

				case SPAIR_SIG_CANCEL: {
					errno = EPIPE;
					res = -1;
					goto out;
				}

				default: {
					__ASSERT(false,
						"unrecognized result: %d",
						result);
					continue;
				}
			}

			/* SPAIR_SIG_DATA was received */
			break;
		}
	}

	res = k_pipe_put(&remote->recv_q, (void *)buffer, count,
			 &bytes_written, 1, K_NO_WAIT);
	__ASSERT(res == 0, "k_pipe_put() failed: %d", res);

	if (spair_write_avail(spair) == 0) {
		k_poll_signal_reset(&remote->writeable);
	}

	res = k_poll_signal_raise(&remote->readable, SPAIR_SIG_DATA);
	__ASSERT(res == 0, "k_poll_signal_raise() failed: %d", res);

	res = bytes_written;

out:

	if (remote != NULL && have_remote_sem) {
		k_sem_give(&remote->sem);
	}
	if (spair != NULL && have_local_sem) {
		k_sem_give(&spair->sem);
	}

	return res;
}

/**
 * Read data from one end of a @ref spair
 *
 * Data written on one file descriptor of a socketpair (with e.g. write(2) or
 * send(2)) can be read at the other end using common POSIX calls such as
 * read(2) or recv(2).
 *
 * If the underlying file descriptor has the @ref O_NONBLOCK flag set then
 * this function will return immediately. If no data was read from a
 * non-blocking file descriptor, then -1 will be returned and @ref errno will
 * be set to @ref EAGAIN.
 *
 * Blocking read operations occur when the @ref O_NONBLOCK flag is @em not set
 * and there are no bytes to read in the @em local @ref spair.pipe.
 *
 * Such a blocking read will suspend execution of the current thread until
 * one of two possible results is received on the @em local
 * @ref spair.readable:
 *
 * -# @ref SPAIR_SIG_DATA - data has been written to the @em local
 *    @ref spair.pipe. Thus, allowing more data to be read.
 *
 * -# @ref SPAIR_SIG_CANCEL - read of the @em local @spair.pipe
 *    must be cancelled for some reason (e.g. the file descriptor will be
 *    closed imminently). In this case, the function will return -1 and set
 *    @ref errno to @ref EINTR.
 *
 * @param obj the address of an @ref spair object cast to `void *`
 * @param buffer the buffer in which to read
 * @param count the number of bytes to read
 *
 * @return on success, a number > 0 representing the number of bytes written
 * @return -1 on error, with @ref errno set appropriately.
 */
static ssize_t spair_read(void *obj, void *buffer, size_t count)
{
	int res;
	bool is_connected;
	size_t avail;
	bool is_nonblock;
	size_t bytes_read;
	bool have_local_sem = false;
	bool will_block = false;
	struct spair *const spair = (struct spair *)obj;

	if (obj == NULL || buffer == NULL || count == 0) {
		errno = EINVAL;
		res = -1;
		goto out;
	}

	res = k_sem_take(&spair->sem, K_NO_WAIT);
	is_nonblock = sock_is_nonblock(spair);
	if (res < 0) {
		if (is_nonblock) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}

		res = k_sem_take(&spair->sem, K_FOREVER);
		if (res < 0) {
			errno = -res;
			res = -1;
			goto out;
		}
		is_nonblock = sock_is_nonblock(spair);
	}

	have_local_sem = true;

	is_connected = sock_is_connected(spair);
	avail = spair_read_avail(spair);

	if (avail == 0) {
		if (!is_connected) {
			/* signal EOF */
			res = 0;
			goto out;
		}

		if (is_nonblock) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}

		will_block = true;
	}

	if (will_block) {
		if (k_is_in_isr()) {
			errno = EAGAIN;
			res = -1;
			goto out;
		}

		for (int signaled = false, result = -1; !signaled;
			result = -1) {

			struct k_poll_event events[] = {
				K_POLL_EVENT_INITIALIZER(
					K_POLL_TYPE_SIGNAL,
					K_POLL_MODE_NOTIFY_ONLY,
					&spair->readable
				),
			};

			k_sem_give(&spair->sem);
			have_local_sem = false;

			res = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
			__ASSERT(res == 0, "k_poll() failed: %d", res);

			res = k_sem_take(&spair->sem, K_FOREVER);
			__ASSERT(res == 0, "failed to take local sem: %d", res);

			have_local_sem = true;

			k_poll_signal_check(&spair->readable, &signaled,
					    &result);
			if (!signaled) {
				continue;
			}

			switch (result) {
				case SPAIR_SIG_DATA: {
					break;
				}

				case SPAIR_SIG_CANCEL: {
					errno = EPIPE;
					res = -1;
					goto out;
				}

				default: {
					__ASSERT(false,
						"unrecognized result: %d",
						result);
					continue;
				}
			}

			/* SPAIR_SIG_DATA was received */
			break;
		}
	}

	res = k_pipe_get(&spair->recv_q, (void *)buffer, count, &bytes_read,
			 1, K_NO_WAIT);
	__ASSERT(res == 0, "k_pipe_get() failed: %d", res);

	if (spair_read_avail(spair) == 0 && !sock_is_eof(spair)) {
		k_poll_signal_reset(&spair->readable);
	}

	if (is_connected) {
		res = k_poll_signal_raise(&spair->writeable, SPAIR_SIG_DATA);
		__ASSERT(res == 0, "k_poll_signal_raise() failed: %d", res);
	}

	res = bytes_read;

out:

	if (spair != NULL && have_local_sem) {
		k_sem_give(&spair->sem);
	}

	return res;
}

static int zsock_poll_prepare_ctx(struct spair *const spair,
				  struct zsock_pollfd *const pfd,
				  struct k_poll_event **pev,
				  struct k_poll_event *pev_end)
{
	int res;

	struct spair *remote = NULL;
	bool have_remote_sem = false;

	if (pfd->events & ZSOCK_POLLIN) {

		/* Tell poll() to short-circuit wait */
		if (sock_is_eof(spair)) {
			res = -EALREADY;
			goto out;
		}

		if (*pev == pev_end) {
			res = -ENOMEM;
			goto out;
		}

		/* Wait until data has been written to the local end */
		(*pev)->obj = &spair->readable;
	}

	if (pfd->events & ZSOCK_POLLOUT) {

		/* Tell poll() to short-circuit wait */
		if (!sock_is_connected(spair)) {
			res = -EALREADY;
			goto out;
		}

		if (*pev == pev_end) {
			res = -ENOMEM;
			goto out;
		}

		remote = zvfs_get_fd_obj(spair->remote,
			(const struct fd_op_vtable *)
			&spair_fd_op_vtable, 0);

		__ASSERT(remote != NULL, "remote is NULL");

		res = k_sem_take(&remote->sem, K_FOREVER);
		if (res < 0) {
			goto out;
		}

		have_remote_sem = true;

		/* Wait until the recv queue on the remote end is no longer full */
		(*pev)->obj = &remote->writeable;
	}

	(*pev)->type = K_POLL_TYPE_SIGNAL;
	(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
	(*pev)->state = K_POLL_STATE_NOT_READY;

	(*pev)++;

	res = 0;

out:

	if (remote != NULL && have_remote_sem) {
		k_sem_give(&remote->sem);
	}

	return res;
}

static int zsock_poll_update_ctx(struct spair *const spair,
				 struct zsock_pollfd *const pfd,
				 struct k_poll_event **pev)
{
	int res;
	int signaled;
	int result;
	struct spair *remote = NULL;
	bool have_remote_sem = false;

	if (pfd->events & ZSOCK_POLLOUT) {
		if (!sock_is_connected(spair)) {
			pfd->revents |= ZSOCK_POLLHUP;
			goto pollout_done;
		}

		remote = zvfs_get_fd_obj(spair->remote,
			(const struct fd_op_vtable *) &spair_fd_op_vtable, 0);

		__ASSERT(remote != NULL, "remote is NULL");

		res = k_sem_take(&remote->sem, K_FOREVER);
		if (res < 0) {
			/* if other end is deleted, this might occur */
			goto pollout_done;
		}

		have_remote_sem = true;

		if (spair_write_avail(spair) > 0) {
			pfd->revents |= ZSOCK_POLLOUT;
			goto pollout_done;
		}

		/* check to see if op was canceled */
		signaled = false;
		k_poll_signal_check(&remote->writeable, &signaled, &result);
		if (signaled) {
			/* Cannot be SPAIR_SIG_DATA, because
			 * spair_write_avail() would have
			 * returned 0
			 */
			__ASSERT(result == SPAIR_SIG_CANCEL,
				"invalid result %d", result);
			pfd->revents |= ZSOCK_POLLHUP;
		}
	}

pollout_done:

	if (pfd->events & ZSOCK_POLLIN) {
		if (sock_is_eof(spair)) {
			pfd->revents |= ZSOCK_POLLIN;
			goto pollin_done;
		}

		if (spair_read_avail(spair) > 0) {
			pfd->revents |= ZSOCK_POLLIN;
			goto pollin_done;
		}

		/* check to see if op was canceled */
		signaled = false;
		k_poll_signal_check(&spair->readable, &signaled, &result);
		if (signaled) {
			/* Cannot be SPAIR_SIG_DATA, because
			 * spair_read_avail() would have
			 * returned 0
			 */
			__ASSERT(result == SPAIR_SIG_CANCEL,
					 "invalid result %d", result);
			pfd->revents |= ZSOCK_POLLIN;
		}
	}

pollin_done:
	res = 0;

	(*pev)++;

	if (remote != NULL && have_remote_sem) {
		k_sem_give(&remote->sem);
	}

	return res;
}

static int spair_ioctl(void *obj, unsigned int request, va_list args)
{
	int res;
	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;
	struct k_poll_event *pev_end;
	int flags = 0;
	bool have_local_sem = false;
	struct spair *const spair = (struct spair *)obj;

	if (spair == NULL) {
		errno = EINVAL;
		res = -1;
		goto out;
	}

	/* The local sem is always taken in this function. If a subsequent
	 * function call requires the remote sem, it must acquire and free the
	 * remote sem.
	 */
	res = k_sem_take(&spair->sem, K_FOREVER);
	__ASSERT(res == 0, "failed to take local sem: %d", res);

	have_local_sem = true;

	switch (request) {
		case F_GETFL: {
			if (sock_is_nonblock(spair)) {
				flags |= O_NONBLOCK;
			}

			res = flags;
			goto out;
		}

		case F_SETFL: {
			flags = va_arg(args, int);

			if (flags & O_NONBLOCK) {
				spair->flags |= SPAIR_FLAG_NONBLOCK;
			} else {
				spair->flags &= ~SPAIR_FLAG_NONBLOCK;
			}

			res = 0;
			goto out;
		}

		case ZFD_IOCTL_FIONBIO: {
			spair->flags |= SPAIR_FLAG_NONBLOCK;
			res = 0;
			goto out;
		}

		case ZFD_IOCTL_FIONREAD: {
			int *nbytes;

			nbytes = va_arg(args, int *);
			*nbytes = spair_read_avail(spair);

			res = 0;
			goto out;
		}

		case ZFD_IOCTL_POLL_PREPARE: {
			pfd = va_arg(args, struct zsock_pollfd *);
			pev = va_arg(args, struct k_poll_event **);
			pev_end = va_arg(args, struct k_poll_event *);

			res = zsock_poll_prepare_ctx(obj, pfd, pev, pev_end);
			goto out;
		}

		case ZFD_IOCTL_POLL_UPDATE: {
			pfd = va_arg(args, struct zsock_pollfd *);
			pev = va_arg(args, struct k_poll_event **);

			res = zsock_poll_update_ctx(obj, pfd, pev);
			goto out;
		}

		default: {
			errno = EOPNOTSUPP;
			res = -1;
			goto out;
		}
	}

out:
	if (spair != NULL && have_local_sem) {
		k_sem_give(&spair->sem);
	}

	return res;
}

static int spair_bind(void *obj, const struct sockaddr *addr,
		      socklen_t addrlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	errno = EISCONN;
	return -1;
}

static int spair_connect(void *obj, const struct sockaddr *addr,
			 socklen_t addrlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	errno = EISCONN;
	return -1;
}

static int spair_listen(void *obj, int backlog)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(backlog);

	errno = EINVAL;
	return -1;
}

static int spair_accept(void *obj, struct sockaddr *addr,
			socklen_t *addrlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	errno = EOPNOTSUPP;
	return -1;
}

static ssize_t spair_sendto(void *obj, const void *buf, size_t len,
			    int flags, const struct sockaddr *dest_addr,
				 socklen_t addrlen)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(dest_addr);
	ARG_UNUSED(addrlen);

	return spair_write(obj, buf, len);
}

static ssize_t spair_sendmsg(void *obj, const struct msghdr *msg,
			     int flags)
{
	ARG_UNUSED(flags);

	int res;
	size_t len = 0;
	bool is_connected;
	size_t avail;
	bool is_nonblock;
	struct spair *const spair = (struct spair *)obj;

	if (spair == NULL || msg == NULL) {
		errno = EINVAL;
		res = -1;
		goto out;
	}

	is_connected = sock_is_connected(spair);
	avail = is_connected ? spair_write_avail(spair) : 0;
	is_nonblock = sock_is_nonblock(spair);

	for (size_t i = 0; i < msg->msg_iovlen; ++i) {
		/* check & msg->msg_iov[i]? */
		/* check & msg->msg_iov[i].iov_base? */
		len += msg->msg_iov[i].iov_len;
	}

	if (!is_connected) {
		errno = EPIPE;
		res = -1;
		goto out;
	}

	if (len == 0) {
		res = 0;
		goto out;
	}

	if (len > avail && is_nonblock) {
		errno = EMSGSIZE;
		res = -1;
		goto out;
	}

	for (size_t i = 0; i < msg->msg_iovlen; ++i) {
		res = spair_write(spair, msg->msg_iov[i].iov_base,
			msg->msg_iov[i].iov_len);
		if (res == -1) {
			goto out;
		}
	}

	res = len;

out:
	return res;
}

static ssize_t spair_recvfrom(void *obj, void *buf, size_t max_len,
			      int flags, struct sockaddr *src_addr,
				   socklen_t *addrlen)
{
	(void)flags;
	(void)src_addr;
	(void)addrlen;

	if (addrlen != NULL) {
		/* Protocol (PF_UNIX) does not support addressing with connected
		 * sockets and, therefore, it is unspecified behaviour to modify
		 * src_addr. However, it would be ambiguous to leave addrlen
		 * untouched if the user expects it to be updated. It is not
		 * mentioned that modifying addrlen is unspecified. Therefore
		 * we choose to eliminate ambiguity.
		 *
		 * Setting it to zero mimics Linux's behaviour.
		 */
		*addrlen = 0;
	}

	return spair_read(obj, buf, max_len);
}

static int spair_getsockopt(void *obj, int level, int optname,
			    void *optval, socklen_t *optlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	errno = ENOPROTOOPT;
	return -1;
}

static int spair_setsockopt(void *obj, int level, int optname,
			    const void *optval, socklen_t optlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	errno = ENOPROTOOPT;
	return -1;
}

static int spair_close(void *obj)
{
	struct spair *const spair = (struct spair *)obj;
	int res;

	res = k_sem_take(&spair->sem, K_FOREVER);
	__ASSERT(res == 0, "failed to take local sem: %d", res);

	/* disconnect the remote endpoint */
	spair_delete(spair);

	/* Note that the semaphore released already so need to do it here */

	return 0;
}

static const struct socket_op_vtable spair_fd_op_vtable = {
	.fd_vtable = {
		.read = spair_read,
		.write = spair_write,
		.close = spair_close,
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
