/* buf.c - Buffer management */

/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_buf
#define LOG_LEVEL CONFIG_NET_BUF_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>

#if defined(CONFIG_NET_BUF_LOG)
#define NET_BUF_DBG(fmt, ...) LOG_DBG("(%p) " fmt, k_current_get(), \
				      ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#endif /* CONFIG_NET_BUF_LOG */

#define NET_BUF_ASSERT(cond, ...) __ASSERT(cond, "" __VA_ARGS__)

#if CONFIG_NET_BUF_WARN_ALLOC_INTERVAL > 0
#define WARN_ALLOC_INTERVAL K_SECONDS(CONFIG_NET_BUF_WARN_ALLOC_INTERVAL)
#else
#define WARN_ALLOC_INTERVAL K_FOREVER
#endif

/* Linker-defined symbol bound to the static pool structs */
STRUCT_SECTION_START_EXTERN(net_buf_pool);

struct net_buf_pool *net_buf_pool_get(int id)
{
	struct net_buf_pool *pool;

	STRUCT_SECTION_GET(net_buf_pool, id, &pool);

	return pool;
}

static int pool_id(struct net_buf_pool *pool)
{
	return pool - TYPE_SECTION_START(net_buf_pool);
}

int net_buf_id(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	size_t struct_size = ROUND_UP(sizeof(struct net_buf) + pool->user_data_size,
				__alignof__(struct net_buf));
	ptrdiff_t offset = (uint8_t *)buf - (uint8_t *)pool->__bufs;

	return offset / struct_size;
}

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
					      uint16_t uninit_count)
{
	size_t struct_size = ROUND_UP(sizeof(struct net_buf) + pool->user_data_size,
				__alignof__(struct net_buf));
	size_t byte_offset = (pool->buf_count - uninit_count) * struct_size;
	struct net_buf *buf;

	buf = (struct net_buf *)(((uint8_t *)pool->__bufs) + byte_offset);

	buf->pool_id = pool_id(pool);
	buf->user_data_size = pool->user_data_size;

	return buf;
}

void net_buf_reset(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf->flags == 0U);
	__ASSERT_NO_MSG(buf->frags == NULL);

	net_buf_simple_reset(&buf->b);
}

static uint8_t *generic_data_ref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - sizeof(void *);
	(*ref_count)++;

	return data;
}

static uint8_t *mem_pool_data_alloc(struct net_buf *buf, size_t *size,
				 k_timeout_t timeout)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;

	/* Reserve extra space for a ref-count (uint8_t) */
	void *b = k_heap_alloc(pool, sizeof(void *) + *size, timeout);

	if (b == NULL) {
		return NULL;
	}

	ref_count = (uint8_t *)b;
	*ref_count = 1U;

	/* Return pointer to the byte following the ref count */
	return ref_count + sizeof(void *);
}

static void mem_pool_data_unref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;

	ref_count = data - sizeof(void *);
	if (--(*ref_count)) {
		return;
	}

	/* Need to copy to local variable due to alignment */
	k_heap_free(pool, ref_count);
}

const struct net_buf_data_cb net_buf_var_cb = {
	.alloc = mem_pool_data_alloc,
	.ref   = generic_data_ref,
	.unref = mem_pool_data_unref,
};

static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size,
			      k_timeout_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	*size = pool->alloc->max_alloc_size;

	return fixed->data_pool + *size * net_buf_id(buf);
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

#if (K_HEAP_MEM_POOL_SIZE > 0)

static uint8_t *heap_data_alloc(struct net_buf *buf, size_t *size,
			     k_timeout_t timeout)
{
	uint8_t *ref_count;

	ref_count = k_malloc(sizeof(void *) + *size);
	if (!ref_count) {
		return NULL;
	}

	*ref_count = 1U;

	return ref_count + sizeof(void *);
}

static void heap_data_unref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - sizeof(void *);
	if (--(*ref_count)) {
		return;
	}

	k_free(ref_count);
}

static const struct net_buf_data_cb net_buf_heap_cb = {
	.alloc = heap_data_alloc,
	.ref   = generic_data_ref,
	.unref = heap_data_unref,
};

const struct net_buf_data_alloc net_buf_heap_alloc = {
	.cb = &net_buf_heap_cb,
	.max_alloc_size = 0,
};

#endif /* K_HEAP_MEM_POOL_SIZE > 0 */

static uint8_t *data_alloc(struct net_buf *buf, size_t *size, k_timeout_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->alloc(buf, size, timeout);
}

static uint8_t *data_ref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->ref(buf, data);
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_len_debug(struct net_buf_pool *pool, size_t size,
					k_timeout_t timeout, const char *func,
					int line)
#else
struct net_buf *net_buf_alloc_len(struct net_buf_pool *pool, size_t size,
				  k_timeout_t timeout)
#endif
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	struct net_buf *buf;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(pool);

	NET_BUF_DBG("%s():%d: pool %p size %zu", func, line, pool, size);

	/* We need to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = k_spin_lock(&pool->lock);

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
				k_spin_unlock(&pool->lock, key);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		k_spin_unlock(&pool->lock, key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	k_spin_unlock(&pool->lock, key);

#if defined(CONFIG_NET_BUF_LOG) && (CONFIG_NET_BUF_LOG_LEVEL >= LOG_LEVEL_WRN)
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		uint32_t ref = k_uptime_get_32();
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		while (!buf) {
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s low on buffers.",
				     func, line, pool->name);
#else
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
				     func, line, pool);
#endif
			buf = k_lifo_get(&pool->free, WARN_ALLOC_INTERVAL);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s blocked for %u secs",
				     func, line, pool->name,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#else
			NET_BUF_WARN("%s():%d: Pool %p blocked for %u secs",
				     func, line, pool,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#endif
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

	if (size) {
#if __ASSERT_ON
		size_t req_size = size;
#endif
		timeout = sys_timepoint_timeout(end);
		buf->__buf = data_alloc(buf, &size, timeout);
		if (!buf->__buf) {
			NET_BUF_ERR("%s():%d: Failed to allocate data",
				    func, line);
			net_buf_destroy(buf);
			return NULL;
		}

#if __ASSERT_ON
		NET_BUF_ASSERT(req_size <= size);
#endif
	} else {
		buf->__buf = NULL;
	}

	buf->ref   = 1U;
	buf->flags = 0U;
	buf->frags = NULL;
	buf->size  = size;
	net_buf_reset(buf);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	atomic_dec(&pool->avail_count);
	__ASSERT_NO_MSG(atomic_get(&pool->avail_count) >= 0);
#endif
	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_fixed_debug(struct net_buf_pool *pool,
					  k_timeout_t timeout, const char *func,
					  int line)
{
	return net_buf_alloc_len_debug(pool, pool->alloc->max_alloc_size, timeout, func,
				       line);
}
#else
struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool,
				    k_timeout_t timeout)
{
	return net_buf_alloc_len(pool, pool->alloc->max_alloc_size, timeout);
}
#endif

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_with_data_debug(struct net_buf_pool *pool,
					      void *data, size_t size,
					      k_timeout_t timeout,
					      const char *func, int line)
#else
struct net_buf *net_buf_alloc_with_data(struct net_buf_pool *pool,
					void *data, size_t size,
					k_timeout_t timeout)
#endif
{
	struct net_buf *buf;

#if defined(CONFIG_NET_BUF_LOG)
	buf = net_buf_alloc_len_debug(pool, 0, timeout, func, line);
#else
	buf = net_buf_alloc_len(pool, 0, timeout);
#endif
	if (!buf) {
		return NULL;
	}

	net_buf_simple_init_with_data(&buf->b, data, size);
	buf->flags = NET_BUF_EXTERNAL_DATA;

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_get_debug(struct k_fifo *fifo, k_timeout_t timeout,
				  const char *func, int line)
#else
struct net_buf *net_buf_get(struct k_fifo *fifo, k_timeout_t timeout)
#endif
{
	struct net_buf *buf;

	NET_BUF_DBG("%s():%d: fifo %p", func, line, fifo);

	buf = k_fifo_get(fifo, timeout);
	if (!buf) {
		return NULL;
	}

	NET_BUF_DBG("%s():%d: buf %p fifo %p", func, line, buf, fifo);

	return buf;
}

static struct k_spinlock net_buf_slist_lock;

void net_buf_slist_put(sys_slist_t *list, struct net_buf *buf)
{
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(list);
	__ASSERT_NO_MSG(buf);

	key = k_spin_lock(&net_buf_slist_lock);
	sys_slist_append(list, &buf->node);
	k_spin_unlock(&net_buf_slist_lock, key);
}

struct net_buf *net_buf_slist_get(sys_slist_t *list)
{
	struct net_buf *buf;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(list);

	key = k_spin_lock(&net_buf_slist_lock);

	buf = (void *)sys_slist_get(list);

	k_spin_unlock(&net_buf_slist_lock, key);

	return buf;
}

void net_buf_put(struct k_fifo *fifo, struct net_buf *buf)
{
	__ASSERT_NO_MSG(fifo);
	__ASSERT_NO_MSG(buf);

	k_fifo_put(fifo, buf);
}

#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
	__ASSERT_NO_MSG(buf);

	while (buf) {
		struct net_buf *frags = buf->frags;
		struct net_buf_pool *pool;

#if defined(CONFIG_NET_BUF_LOG)
		if (!buf->ref) {
			NET_BUF_ERR("%s():%d: buf %p double free", func, line,
				    buf);
			return;
		}
#endif
		NET_BUF_DBG("buf %p ref %u pool_id %u frags %p", buf, buf->ref,
			    buf->pool_id, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		buf->data = NULL;
		buf->frags = NULL;

		pool = net_buf_pool_get(buf->pool_id);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		atomic_inc(&pool->avail_count);
		__ASSERT_NO_MSG(atomic_get(&pool->avail_count) <= pool->buf_count);
#endif

		if (pool->destroy) {
			pool->destroy(buf);
		} else {
			net_buf_destroy(buf);
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf);

	NET_BUF_DBG("buf %p (old) ref %u pool_id %u",
		    buf, buf->ref, buf->pool_id);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	struct net_buf_pool *pool;
	struct net_buf *clone;

	__ASSERT_NO_MSG(buf);

	pool = net_buf_pool_get(buf->pool_id);

	clone = net_buf_alloc_len(pool, 0, timeout);
	if (!clone) {
		return NULL;
	}

	/* If the pool supports data referencing use that. Otherwise
	 * we need to allocate new data and make a copy.
	 */
	if (pool->alloc->cb->ref && !(buf->flags & NET_BUF_EXTERNAL_DATA)) {
		clone->__buf = data_ref(buf, buf->__buf);
		clone->data = buf->data;
		clone->len = buf->len;
		clone->size = buf->size;
	} else {
		size_t size = buf->size;

		timeout = sys_timepoint_timeout(end);

		clone->__buf = data_alloc(clone, &size, timeout);
		if (!clone->__buf || size < buf->size) {
			net_buf_destroy(clone);
			return NULL;
		}

		clone->size = size;
		clone->data = clone->__buf + net_buf_headroom(buf);
		net_buf_add_mem(clone, buf->data, buf->len);
	}

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	__ASSERT_NO_MSG(parent);
	__ASSERT_NO_MSG(frag);

	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct net_buf *net_buf_frag_add(struct net_buf *head, struct net_buf *frag)
{
	__ASSERT_NO_MSG(frag);

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

	__ASSERT_NO_MSG(frag);

	if (parent) {
		__ASSERT_NO_MSG(parent->frags);
		__ASSERT_NO_MSG(parent->frags == frag);
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

size_t net_buf_linearize(void *dst, size_t dst_len, struct net_buf *src,
			 size_t offset, size_t len)
{
	struct net_buf *frag;
	size_t to_copy;
	size_t copied;

	len = MIN(len, dst_len);

	frag = src;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		memcpy((uint8_t *)dst + copied, frag->data + offset, to_copy);

		copied += to_copy;

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	return copied;
}

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
size_t net_buf_append_bytes(struct net_buf *buf, size_t len,
			    const void *value, k_timeout_t timeout,
			    net_buf_allocator_cb allocate_cb, void *user_data)
{
	struct net_buf *frag = net_buf_frag_last(buf);
	size_t added_len = 0;
	const uint8_t *value8 = value;
	size_t max_size;

	do {
		uint16_t count = MIN(len, net_buf_tailroom(frag));

		net_buf_add_mem(frag, value8, count);
		len -= count;
		added_len += count;
		value8 += count;

		if (len == 0) {
			return added_len;
		}

		if (allocate_cb) {
			frag = allocate_cb(timeout, user_data);
		} else {
			struct net_buf_pool *pool;

			/* Allocate from the original pool if no callback has
			 * been provided.
			 */
			pool = net_buf_pool_get(buf->pool_id);
			max_size = pool->alloc->max_alloc_size;
			frag = net_buf_alloc_len(pool,
						 max_size ? MIN(len, max_size) : len,
						 timeout);
		}

		if (!frag) {
			return added_len;
		}

		net_buf_frag_add(buf, frag);
	} while (1);

	/* Unreachable */
	return 0;
}

size_t net_buf_data_match(const struct net_buf *buf, size_t offset, const void *data, size_t len)
{
	const uint8_t *dptr = data;
	const uint8_t *bptr;
	size_t compared = 0;
	size_t to_compare;

	if (!buf || !data) {
		return compared;
	}

	/* find the right fragment to start comparison */
	while (buf && offset >= buf->len) {
		offset -= buf->len;
		buf = buf->frags;
	}

	while (buf && len > 0) {
		bptr = buf->data + offset;
		to_compare = MIN(len, buf->len - offset);

		for (size_t i = 0; i < to_compare; ++i) {
			if (dptr[compared] != bptr[i]) {
				return compared;
			}
			compared++;
		}

		len -= to_compare;
		buf = buf->frags;
		offset = 0;
	}

	return compared;
}
