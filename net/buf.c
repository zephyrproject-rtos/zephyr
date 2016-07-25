/* buf.c - Buffer management */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <misc/byteorder.h>

#include <net/buf.h>

#if defined(CONFIG_NET_BUF_DEBUG)
#define NET_BUF_DBG(fmt, ...) printf("buf: %s (%p): " fmt, __func__, \
				sys_thread_self_get(), ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) printf("buf: %s: " fmt, __func__, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) printf("buf: %s: " fmt, __func__, ##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) printf("buf: " fmt,  ##__VA_ARGS__)
#define NET_BUF_ASSERT(cond) do { if (!(cond)) {			  \
			NET_BUF_ERR("buf: assert: '" #cond "' failed\n"); \
		}} while(0)
#else
#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#define NET_BUF_ASSERT(cond)
#endif /* CONFIG_NET_BUF_DEBUG */

struct net_buf *net_buf_get_timeout(struct nano_fifo *fifo,
				    size_t reserve_head, int32_t timeout)
{
	struct net_buf *buf, *frag;

	NET_BUF_DBG("fifo %p reserve %u timeout %d\n", fifo, reserve_head,
		    timeout);

	buf = nano_fifo_get(fifo, timeout);
	if (!buf) {
		NET_BUF_ERR("Failed to get free buffer\n");
		return NULL;
	}

	NET_BUF_DBG("buf %p fifo %p reserve %u\n", buf, fifo, reserve_head);

	/* If this buffer is from the free buffers FIFO there wont be
	 * any fragments and we can directly proceed with initializing
	 * and returning it.
	 */
	if (buf->free == fifo) {
		buf->ref   = 1;
		buf->len   = 0;
		net_buf_reserve(buf, reserve_head);
		buf->flags = 0;
		buf->frags = NULL;

		return buf;
	}

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		frag->frags = nano_fifo_get(fifo, TICKS_NONE);
		NET_BUF_ASSERT(frag->frags);

		/* The fragments flag is only for FIFO-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

struct net_buf *net_buf_get(struct nano_fifo *fifo, size_t reserve_head)
{
	struct net_buf *buf;

	NET_BUF_DBG("fifo %p reserve %u\n", fifo, reserve_head);

	buf = net_buf_get_timeout(fifo, reserve_head, TICKS_NONE);
	if (buf || sys_execution_context_type_get() == NANO_CTX_ISR) {
		return buf;
	}

	NET_BUF_WARN("Low on buffers. Waiting (fifo %p)\n", fifo);

	return net_buf_get_timeout(fifo, reserve_head, TICKS_UNLIMITED);
}

void net_buf_reserve(struct net_buf *buf, size_t reserve)
{
	NET_BUF_ASSERT(buf->len == 0);
	NET_BUF_DBG("buf %p reserve %u", buf, reserve);

	buf->data = buf->__buf + reserve;
}

void net_buf_put(struct nano_fifo *fifo, struct net_buf *buf)
{
	struct net_buf *tail;

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

	nano_fifo_put_list(fifo, buf, tail);
}

void net_buf_unref(struct net_buf *buf)
{
	NET_BUF_DBG("buf %p ref %u fifo %p frags %p\n", buf, buf->ref,
		    buf->free, buf->frags);
	NET_BUF_ASSERT(buf->ref > 0);

	while (buf && --buf->ref == 0) {
		struct net_buf *frags = buf->frags;

		buf->frags = NULL;

		if (buf->destroy) {
			buf->destroy(buf);
		} else {
			nano_fifo_put(buf->free, buf);
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	NET_BUF_DBG("buf %p (old) ref %u fifo %p\n", buf, buf->ref, buf->free);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf)
{
	struct net_buf *clone;

	clone = net_buf_get(buf->free, net_buf_headroom(buf));
	if (!clone) {
		return NULL;
	}

	/* TODO: Add reference to the original buffer instead of copying it. */
	memcpy(net_buf_add(clone, buf->len), buf->data, buf->len);

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	parent->frags = net_buf_ref(frag);
}

void net_buf_frag_del(struct net_buf *parent, struct net_buf *frag)
{
	NET_BUF_ASSERT(parent->frags);
	NET_BUF_ASSERT(parent->frags == frag);

	parent->frags = frag->frags;
	frag->frags = NULL;
	net_buf_unref(frag);
}

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	uint8_t *tail = net_buf_simple_tail(buf);

	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(net_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

uint8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *u8;

	NET_BUF_DBG("buf %p val 0x%02x\n", buf, val);

	u8 = net_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

	val = sys_cpu_to_be16(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

	val = sys_cpu_to_le32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

	val = sys_cpu_to_be32(val);
	memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(net_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

	val = sys_cpu_to_le16(val);
	memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_DBG("buf %p val %u\n", buf, val);

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
	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(buf->len >= len);

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
