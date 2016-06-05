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
		buf->data  = buf->__buf + reserve_head;
		buf->len   = 0;
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

void net_buf_put(struct nano_fifo *fifo, struct net_buf *buf)
{
	int mask;

	mask = irq_lock();

	while (buf) {
		struct net_buf *frag = buf->frags;

		if (frag) {
			buf->flags |= NET_BUF_FRAGS;
		}

		nano_fifo_put(fifo, buf);
		buf = frag;
	}

	irq_unlock(mask);
}

void net_buf_unref(struct net_buf *buf)
{
	NET_BUF_DBG("buf %p ref %u fifo %p\n", buf, buf->ref, buf->free);
	NET_BUF_ASSERT(buf->ref > 0);

	if (--buf->ref) {
		return;
	}

	if (buf->frags) {
		net_buf_unref(buf->frags);
		buf->frags = NULL;
	}

	if (buf->destroy) {
		buf->destroy(buf);
	} else {
		nano_fifo_put(buf->free, buf);
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

void *net_buf_add(struct net_buf *buf, size_t len)
{
	uint8_t *tail = net_buf_tail(buf);

	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(net_buf_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

uint8_t *net_buf_add_u8(struct net_buf *buf, uint8_t value)
{
	uint8_t *u8;

	NET_BUF_DBG("buf %p value 0x%02x\n", buf, value);

	u8 = net_buf_add(buf, 1);
	*u8 = value;

	return u8;
}

void net_buf_add_le16(struct net_buf *buf, uint16_t value)
{
	NET_BUF_DBG("buf %p value %u\n", buf, value);

	value = sys_cpu_to_le16(value);
	memcpy(net_buf_add(buf, sizeof(value)), &value, sizeof(value));
}

void net_buf_add_le32(struct net_buf *buf, uint32_t value)
{
	NET_BUF_DBG("buf %p value %u\n", buf, value);

	value = sys_cpu_to_le32(value);
	memcpy(net_buf_add(buf, sizeof(value)), &value, sizeof(value));
}

void *net_buf_push(struct net_buf *buf, size_t len)
{
	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(net_buf_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void net_buf_push_le16(struct net_buf *buf, uint16_t value)
{
	NET_BUF_DBG("buf %p value %u\n", buf, value);

	value = sys_cpu_to_le16(value);
	memcpy(net_buf_push(buf, sizeof(value)), &value, sizeof(value));
}

void *net_buf_pull(struct net_buf *buf, size_t len)
{
	NET_BUF_DBG("buf %p len %u\n", buf, len);

	NET_BUF_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

uint8_t net_buf_pull_u8(struct net_buf *buf)
{
	uint8_t val;

	val = buf->data[0];
	net_buf_pull(buf, 1);

	return val;
}

uint16_t net_buf_pull_le16(struct net_buf *buf)
{
	uint16_t value;

	value = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_pull(buf, sizeof(value));

	return sys_le16_to_cpu(value);
}

uint32_t net_buf_pull_le32(struct net_buf *buf)
{
	uint32_t value;

	value = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_pull(buf, sizeof(value));

	return sys_le32_to_cpu(value);
}

size_t net_buf_headroom(struct net_buf *buf)
{
	return buf->data - buf->__buf;
}

size_t net_buf_tailroom(struct net_buf *buf)
{
	return buf->size - net_buf_headroom(buf) - buf->len;
}
