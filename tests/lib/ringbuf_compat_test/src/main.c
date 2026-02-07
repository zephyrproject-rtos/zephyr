/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/irq.h>
#include <zephyr/types.h>
#include <stdint.h>

struct queue_item_ser {
	uintptr_t ptr;
	uint32_t size;
} __packed;

#define ENTRY_SIZE sizeof(struct queue_item_ser)
#define SLOTS      4

static uint8_t rb_buf[SLOTS * ENTRY_SIZE];
static struct ring_buf rb;

static void *tb_init(void)
{
	ring_buf_init(&rb, sizeof(rb_buf), rb_buf);
	return NULL;
}

static int tb_put(void *mem_block, size_t size)
{
	struct queue_item_ser s = {(uintptr_t)mem_block, (uint32_t)size};
	size_t written;
	unsigned int key = irq_lock();

	written = ring_buf_put(&rb, (const uint8_t *)&s, ENTRY_SIZE);
	irq_unlock(key);
	return (written == ENTRY_SIZE) ? 0 : -ENOMEM;
}

static int tb_get(void **mem_block, size_t *size)
{
	struct queue_item_ser s;
	size_t read;
	unsigned int key = irq_lock();

	read = ring_buf_get(&rb, (uint8_t *)&s, ENTRY_SIZE);
	irq_unlock(key);
	if (read != ENTRY_SIZE) {
		return -ENOMEM;
	}
	*mem_block = (void *)(uintptr_t)s.ptr;
	*size = (size_t)s.size;
	return 0;
}

ZTEST(ringbuf_tests, test_put_get)
{
	void *p1 = (void *)0x1000;
	void *p2 = (void *)0x2000;
	void *out;
	size_t outsz;
	int rc;

	rc = tb_put(p1, 16);
	zassert_equal(rc, 0, "put p1 failed");

	rc = tb_put(p2, 32);
	zassert_equal(rc, 0, "put p2 failed");

	rc = tb_get(&out, &outsz);
	zassert_equal(rc, 0, "get1 failed");
	zassert_equal((uintptr_t)out, (uintptr_t)p1, "ptr mismatch 1");
	zassert_equal(outsz, (size_t)16, "size mismatch 1");

	rc = tb_get(&out, &outsz);
	zassert_equal(rc, 0, "get2 failed");
	zassert_equal((uintptr_t)out, (uintptr_t)p2, "ptr mismatch 2");
	zassert_equal(outsz, (size_t)32, "size mismatch 2");

	rc = tb_get(&out, &outsz);
	zassert_equal(rc, -ENOMEM, "expected empty");
}

ZTEST_SUITE(ringbuf_tests, NULL, tb_init, NULL, NULL, NULL);
