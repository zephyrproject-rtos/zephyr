/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/spsc_lockfree.h>
#include <zephyr/sys/sys_heap.h>
#include <libmctp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp, CONFIG_MCTP_LOG_LEVEL);

#define MCTP_INVALID_SOCKET_ID 0xFF

struct mctp_buf {
	sys_snode_t node;
	size_t len;
	size_t offs;
	uint8_t buf[0];
};

struct mctp_sock {
	uint8_t endpoint_id;
	sys_slist_t buf_list;
	struct k_sem bufs_avail;
};

struct mctp_listen {
	uint8_t endpoint_id;
	sys_slist_t accept_list;
	struct k_sem accepts_avail;
};

static int _mctp_listen_sock;
SPSC_DEFINE(_mctp_accept_socks, uint8_t, 64);
K_SEM_DEFINE(_mctp_accept_sem, 0, K_SEM_MAX_LIMIT);

static struct {
	struct k_spinlock lock;
	struct mctp_sock sockets[CONFIG_MCTP_SOCKETS];
	uint8_t endpoint_to_socket[UINT8_MAX];
} mctp_sockets;

static uint8_t MCTP_MEM[CONFIG_MCTP_HEAP_SIZE];

static struct {
	struct k_spinlock lock;
	struct sys_heap heap;
} mctp_heap;

static int mctp_sock_alloc(struct mctp_sock **sock, uint8_t endpoint_id)
{
	int sock_id;

	for (sock_id = 0; sock_id < CONFIG_MCTP_SOCKETS; sock_id++) {
		if (mctp_sockets.sockets[sock_id].endpoint_id == 0) {
			*sock = &mctp_sockets.sockets[sock_id];
			break;
		}
	}

	if (*sock == NULL) {
		return -ENOMEM;
	}

	mctp_sockets.endpoint_to_socket[endpoint_id] = sock_id;
	(*sock)->endpoint_id = endpoint_id;
	k_sem_init(&((*sock)->bufs_avail), 0, K_SEM_MAX_LIMIT);
	sys_slist_init(&((*sock)->buf_list));

	return sock_id;
}

static void *mctp_heap_alloc(size_t bytes)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	void *ptr = sys_heap_alloc(&mctp_heap.heap, bytes);

	k_spin_unlock(&mctp_heap.lock, key);

	return ptr;
}

static void mctp_heap_free(void *ptr)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	sys_heap_free(&mctp_heap.heap, ptr);

	k_spin_unlock(&mctp_heap.lock, key);
}

static void *mctp_heap_realloc(void *ptr, size_t bytes)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	void *new_ptr = sys_heap_realloc(&mctp_heap.heap, ptr, bytes);

	k_spin_unlock(&mctp_heap.lock, key);

	return new_ptr;
}

static void mctp_rx_message(uint8_t source_eid, bool tag_owner, uint8_t msg_tag, void *data,
			    void *msg, size_t len)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);
	uint8_t sock_id = mctp_sockets.endpoint_to_socket[source_eid];

	if (sock_id == MCTP_INVALID_SOCKET_ID) {
		if (_mctp_listen_sock) {
			/* Acquire accept sock from the ring */
			uint8_t *accept_sock = spsc_acquire(&_mctp_accept_socks);

			if (accept_sock == NULL) {
				LOG_WRN("Out of accept sockets");
				goto out;
			}

			/* Setup a new socket for this source */
			struct mctp_sock *sock = NULL;

			sock_id = (uint8_t)mctp_sock_alloc(&sock, source_eid);
			if (sock == NULL) {
				spsc_drop_all(&_mctp_accept_socks);
				LOG_WRN("Out of free sockets to accept new connections");
				goto out;
			}

			*accept_sock = sock_id;
			spsc_produce(&_mctp_accept_socks);
			k_sem_give(&_mctp_accept_sem);

		} else {
			LOG_DBG("received message for unopened peer %u", source_eid);
			goto out;
		}
	}

	struct mctp_sock *sock = &mctp_sockets.sockets[sock_id];
	struct mctp_buf *buf = mctp_heap_alloc(sizeof(struct mctp_buf) + len);

	if (buf == NULL) {
		LOG_ERR("Out of memory allocating mctp buffer len %zu", len);
		goto out;
	}

	memcpy(buf->buf, msg, len);
	buf->len = len;
	buf->offs = 0;
	sys_slist_append(&sock->buf_list, &buf->node);
	k_sem_give(&sock->bufs_avail);

out:
	k_spin_unlock(&mctp_sockets.lock, key);
}

static struct mctp *mctp_ctx;

static int zephyr_mctp_init(void)
{
	sys_heap_init(&mctp_heap.heap, MCTP_MEM, sizeof(MCTP_MEM));
	mctp_set_alloc_ops(mctp_heap_alloc, mctp_heap_free, mctp_heap_realloc);

	for (int i = 0; i < UINT8_MAX; i++) {
		mctp_sockets.endpoint_to_socket[i] = MCTP_INVALID_SOCKET_ID;
	}

	mctp_ctx = mctp_init();
	mctp_set_rx_all(mctp_ctx, mctp_rx_message, NULL);

	return 0;
}

int zephyr_mctp_register_bus(struct mctp_binding *binding)
{
	mctp_register_bus(mctp_ctx, binding, CONFIG_MCTP_ENDPOINT_ID);

	return 0;
}

int zephyr_mctp_listen(void)
{
	LOG_DBG("mctp listening");
	_mctp_listen_sock = k_uptime_seconds() + 1;
	return _mctp_listen_sock;
}

int zephyr_mctp_accept(int listen_sock)
{
	int sock_id = -EINVAL;

	LOG_INF("accept called with sock %d, listen sock %d", listen_sock, _mctp_listen_sock);

	if (_mctp_listen_sock == listen_sock) {
		k_sem_take(&_mctp_accept_sem, K_FOREVER);
		uint8_t *accept_sock = spsc_consume(&_mctp_accept_socks);

		sock_id = *accept_sock;
		spsc_release(&_mctp_accept_socks);
	}

	LOG_INF("accepting socket %u", sock_id);
	return sock_id;
}

int zephyr_mctp_open(uint8_t endpoint_id)
{
	int sock_id = 0;

	if (mctp_sockets.endpoint_to_socket[endpoint_id] != MCTP_INVALID_SOCKET_ID) {
		LOG_WRN("Attempted to open already existing endpoint %u", endpoint_id);
		sock_id = -EINVAL;
		goto out;
	}

	struct mctp_sock *sock = NULL;
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);

	sock_id = mctp_sock_alloc(&sock, endpoint_id);

	k_spin_unlock(&mctp_sockets.lock, key);
out:
	return sock_id;
}

static struct mctp_sock *get_socket(int sock_id)
{
	if (sock_id < 0 || sock_id > CONFIG_MCTP_SOCKETS) {
		return NULL;
	}

	return &mctp_sockets.sockets[sock_id];
}

int zephyr_mctp_poll_event_init(int sock_id, struct k_poll_event *evt)
{
	int ret;
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);
	struct mctp_sock *sock = get_socket(sock_id);

	if (sock == NULL) {
		ret = -EINVAL;
		goto out;
	}

	k_poll_event_init(evt, K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			  &sock->bufs_avail);

out:
	k_spin_unlock(&mctp_sockets.lock, key);
	return ret;
}

int zephyr_mctp_endpoint(int sock_id, uint8_t *endpoint)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);

	struct mctp_sock *sock = get_socket(sock_id);

	if (sock == NULL || sock->endpoint_id == 0) {
		return -EINVAL;
	}

	*endpoint = sock->endpoint_id;

	k_spin_unlock(&mctp_sockets.lock, key);

	return 0;
}

int zephyr_mctp_write(int sock_id, uint8_t *msg, size_t len)
{

	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);
	struct mctp_sock *sock = get_socket(sock_id);

	k_spin_unlock(&mctp_sockets.lock, key);

	if (sock == NULL || sock->endpoint_id == 0) {
		return -EINVAL;
	}

	int rc = mctp_message_tx(mctp_ctx, sock->endpoint_id, false, 0, msg, len);

	return rc;
}

int zephyr_mctp_read(int sock_id, uint8_t *msg, size_t *len)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);
	struct mctp_sock *sock = get_socket(sock_id);

	k_spin_unlock(&mctp_sockets.lock, key);

	if (sock == NULL || sock->endpoint_id == 0) {
		return -EINVAL;
	}

	size_t read_len = 0;

	while (k_sem_take(&sock->bufs_avail, K_NO_WAIT) == 0 && read_len < *len) {
		sys_snode_t *head = sys_slist_peek_head(&sock->buf_list);
		struct mctp_buf *buf = CONTAINER_OF(head, struct mctp_buf, node);
		size_t copy_len = MIN(buf->len - buf->offs, *len - read_len);

		memcpy(&msg[read_len], &buf->buf[buf->offs], copy_len);

		read_len += copy_len;

		if (copy_len < (buf->len - buf->offs)) {
			buf->offs = copy_len;
			k_sem_give(&sock->bufs_avail);
		} else {
			sys_slist_get(&sock->buf_list);
			mctp_heap_free(buf);
		}
	}

	*len = read_len;

	return 0;
}

int zephyr_mctp_read_exact(int sock_id, uint8_t *msg, size_t len)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_sockets.lock);
	struct mctp_sock *sock = get_socket(sock_id);

	k_spin_unlock(&mctp_sockets.lock, key);

	if (sock == NULL || sock->endpoint_id == 0) {
		return -EINVAL;
	}

	size_t read_len = 0;

	while (read_len < len) {
		k_sem_take(&sock->bufs_avail, K_FOREVER);

		sys_snode_t *head = sys_slist_peek_head(&sock->buf_list);
		struct mctp_buf *buf = CONTAINER_OF(head, struct mctp_buf, node);
		size_t copy_len = MIN(buf->len - buf->offs, len - read_len);

		memcpy(&msg[read_len], &buf->buf[buf->offs], copy_len);

		read_len += copy_len;

		if (copy_len < (buf->len - buf->offs)) {
			buf->offs = copy_len;
			k_sem_give(&sock->bufs_avail);
		} else {
			sys_slist_get(&sock->buf_list);
		}
		LOG_DBG("copy len %u, read len %u, buf offs %u", copy_len, read_len, buf->offs);
	}

	return 0;
}

SYS_INIT_NAMED(mctp, zephyr_mctp_init, POST_KERNEL, 0);
