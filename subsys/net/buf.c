/* buf.c - Buffer management */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <misc/byteorder.h>

#include <net/buf.h>

#if defined(CONFIG_NET_BUF_LOG)
#define SYS_LOG_DOMAIN "net/buf"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_BUF_LEVEL
#include <logging/sys_log.h>

#define NET_BUF_DBG(fmt, ...) SYS_LOG_DBG("(%p) " fmt, k_current_get(), \
					  ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) SYS_LOG_WRN(fmt,	##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) SYS_LOG_INF(fmt,  ##__VA_ARGS__)
#define NET_BUF_ASSERT(cond) do { if (!(cond)) {			  \
			NET_BUF_ERR("assert: '" #cond "' failed"); \
		} } while (0)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#define NET_BUF_ASSERT(cond)
#endif /* CONFIG_NET_BUF_LOG */

/* Helpers to access the storage array, since we don't have access to its
 * type at this point anymore.
 */
#define BUF_SIZE(pool) (sizeof(struct net_buf) + \
			ROUND_UP(pool->buf_size, 4) + \
			ROUND_UP(pool->user_data_size, 4))
#define UNINIT_BUF(pool, n) (struct net_buf *)(((uint8_t *)(pool->__bufs)) + \
					       ((n) * BUF_SIZE(pool)))

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
					      uint16_t uninit_count)
{
	struct net_buf *buf;

	buf = UNINIT_BUF(pool, pool->buf_count - uninit_count);

	buf->pool = pool;
	buf->size = pool->buf_size;

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_debug(struct net_buf_pool *pool, int32_t timeout,
				    const char *func, int line)
#else
struct net_buf *net_buf_alloc(struct net_buf_pool *pool, int32_t timeout)
#endif
{
	struct net_buf *buf;
	unsigned int key;

	NET_BUF_ASSERT(pool);

	NET_BUF_DBG("%s():%d: pool %p timeout %d", func, line, pool, timeout);

	/* We need to lock interrupts temporarily to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = irq_lock();

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	if (pool->uninit_count) {
		uint16_t uninit_count;

		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the LIFO with K_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = k_lifo_get(&pool->free, K_NO_WAIT);
			if (buf) {
				irq_unlock(key);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		irq_unlock(key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	irq_unlock(key);

#if defined(CONFIG_NET_BUF_LOG) && SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING
	if (timeout == K_FOREVER) {
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		if (!buf) {
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
				     func, line, pool);
			buf = k_lifo_get(&pool->free, timeout);
		}
	} else {
		buf = k_lifo_get(&pool->free, timeout);
	}
#else
	buf = k_lifo_get(&pool->free, timeout);
#endif
	if (!buf) {
		NET_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	NET_BUF_DBG("allocated buf %p", buf);

	buf->ref   = 1;
	buf->len   = 0;
	buf->data  = buf->__buf;
	buf->flags = 0;
	buf->frags = NULL;

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_get_debug(struct k_fifo *fifo, int32_t timeout,
				  const char *func, int line)
#else
struct net_buf *net_buf_get(struct k_fifo *fifo, int32_t timeout)
#endif
{
	struct net_buf *buf, *frag;

	NET_BUF_DBG("%s():%d: fifo %p timeout %d", func, line, fifo, timeout);

	buf = k_fifo_get(fifo, timeout);
	if (!buf) {
		NET_BUF_ERR("Failed to get free buffer");
		return NULL;
	}

	NET_BUF_DBG("%s():%d: buf %p fifo %p", func, line, buf, fifo);

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		frag->frags = k_fifo_get(fifo, K_NO_WAIT);
		NET_BUF_ASSERT(frag->frags);

		/* The fragments flag is only for FIFO-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_reserve(struct net_buf *buf, size_t reserve)
{
	NET_BUF_ASSERT(buf);
	NET_BUF_ASSERT(buf->len == 0);
	NET_BUF_DBG("buf %p reserve %zu", buf, reserve);

	buf->data = buf->__buf + reserve;
}

void net_buf_put(struct k_fifo *fifo, struct net_buf *buf)
{
	struct net_buf *tail;

	NET_BUF_ASSERT(fifo);
	NET_BUF_ASSERT(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

	k_fifo_put_list(fifo, buf, tail);
}

#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
	NET_BUF_ASSERT(buf);

	while (buf) {
		struct net_buf *frags = buf->frags;

#if defined(CONFIG_NET_BUF_LOG)
		if (!buf->ref) {
			NET_BUF_ERR("%s():%d: buf %p double free", func, line,
				    buf);
			return;
		}
#endif
		NET_BUF_DBG("buf %p ref %u pool %p frags %p", buf, buf->ref,
			    buf->pool, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		buf->frags = NULL;

		if (buf->pool->destroy) {
			buf->pool->destroy(buf);
		} else {
			net_buf_destroy(buf);
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	NET_BUF_DBG("buf %p (old) ref %u pool %p",
		    buf, buf->ref, buf->pool);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf, int32_t timeout)
{
	struct net_buf *clone;

	NET_BUF_ASSERT(buf);

	clone = net_buf_alloc(buf->pool, timeout);
	if (!clone) {
		return NULL;
	}

	net_buf_reserve(clone, net_buf_headroom(buf));

	/* TODO: Add reference to the original buffer instead of copying it. */
	memcpy(net_buf_add(clone, buf->len), buf->data, buf->len);

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	NET_BUF_ASSERT(parent);
	NET_BUF_ASSERT(frag);

	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct net_buf *net_buf_frag_add(struct net_buf *head, struct net_buf *frag)
{
	NET_BUF_ASSERT(frag);

	if (!head) {
		return net_buf_ref(frag);
	}

	net_buf_frag_insert(net_buf_frag_last(head), frag);

	return head;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_frag_del_debug(struct net_buf *parent,
				       struct net_buf *frag,
				       const char *func, int line)
#else
struct net_buf *net_buf_frag_del(struct net_buf *parent, struct net_buf *frag)
#endif
{
	struct net_buf *next_frag;

	NET_BUF_ASSERT(frag);

	if (parent) {
		NET_BUF_ASSERT(parent->frags);
		NET_BUF_ASSERT(parent->frags == frag);
		parent->frags = frag->frags;
	}

	next_frag = frag->frags;

	frag->frags = NULL;

#if defined(CONFIG_NET_BUF_LOG)
	net_buf_unref_debug(frag, func, line);
#else
	net_buf_unref(frag);
#endif

	return next_frag;
}

#if defined(CONFIG_NET_BUF_SIMPLE_LOG)
#define NET_BUF_SIMPLE_DBG(fmt, ...) NET_BUF_DBG(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ERR(fmt, ...) NET_BUF_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_WARN(fmt, ...) NET_BUF_WARN(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_INFO(fmt, ...) NET_BUF_INFO(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ASSERT(cond) NET_BUF_ASSERT(cond)
#else
#define NET_BUF_SIMPLE_DBG(fmt, ...)
#define NET_BUF_SIMPLE_ERR(fmt, ...)
#define NET_BUF_SIMPLE_WARN(fmt, ...)
#define NET_BUF_SIMPLE_INFO(fmt, ...)
#define NET_BUF_SIMPLE_ASSERT(cond)
#endif /* CONFIG_NET_BUF_SIMPLE_LOG */

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	uint8_t *tail = net_buf_simple_tail(buf);

	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem,
			     size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	return memcpy(net_buf_simple_add(buf, len), mem, len);
}

uint8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *u8;

	NET_BUF_SIMPLE_DBG("buf %p val 0x%02x", buf, val);

	u8 = net_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	val = sys_cpu_to_be16(val);
	memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *data = net_buf_simple_push(buf, 1);

	*data = val;
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	NET_BUF_SIMPLE_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

uint8_t net_buf_simple_pull_u8(struct net_buf_simple *buf)
{
	uint8_t val;

	val = buf->data[0];
	net_buf_simple_pull(buf, 1);

	return val;
}

uint16_t net_buf_simple_pull_le16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le16_to_cpu(val);
}

uint16_t net_buf_simple_pull_be16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be16_to_cpu(val);
}

uint32_t net_buf_simple_pull_le32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le32_to_cpu(val);
}

uint32_t net_buf_simple_pull_be32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be32_to_cpu(val);
}

size_t net_buf_simple_headroom(struct net_buf_simple *buf)
{
	return buf->data - buf->__buf;
}

size_t net_buf_simple_tailroom(struct net_buf_simple *buf)
{
	return buf->size - net_buf_simple_headroom(buf) - buf->len;
}
