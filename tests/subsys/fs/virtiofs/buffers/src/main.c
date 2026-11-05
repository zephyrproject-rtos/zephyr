/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/ztest.h>

static struct {
	const struct virtq_buf *original;
	uint16_t count;
	uint16_t readable;
	uint32_t bounced;
	size_t allocation_size;
	uint32_t allocations;
	uint32_t frees;
	uint32_t submissions;
	uint32_t notifications;
	bool fail_allocation;
	int enqueue_error;
} transfer;

static void *test_malloc(size_t size)
{
	transfer.allocations++;
	transfer.allocation_size = size;
	return transfer.fail_allocation ? NULL : k_malloc(size);
}

static void test_free(void *ptr)
{
	if (ptr != NULL) {
		transfer.frees++;
	}
	k_free(ptr);
}

#define k_malloc test_malloc
#define k_free   test_free
#include "virtiofs.c"
#undef k_malloc
#undef k_free

int virtq_add_buffer_chain(struct virtq *vq, struct virtq_buf *bufs, uint16_t count,
			   uint16_t readable, virtq_receive_callback cb, void *opaque,
			   k_timeout_t timeout)
{
	uint32_t used_len = 0;

	ARG_UNUSED(vq);
	ARG_UNUSED(timeout);
	transfer.submissions++;
	zassert_equal(count, transfer.count);
	zassert_equal(readable, transfer.readable);

	if (transfer.enqueue_error != 0) {
		return transfer.enqueue_error;
	}

	for (uint16_t i = 0; i < count; i++) {
		zassert_equal(bufs[i].len, transfer.original[i].len);
		zassert_true(virtiofs_buffer_is_direct(&bufs[i]), "invalid descriptor address");
		if (bufs[i].addr != transfer.original[i].addr) {
			transfer.bounced |= BIT(i);
		}
		if (bufs[i].len == 0U) {
			continue;
		}
		if (i < readable) {
			zassert_mem_equal(bufs[i].addr, transfer.original[i].addr, bufs[i].len);
		} else {
			memset(bufs[i].addr, 0x5a, bufs[i].len);
			used_len += bufs[i].len;
		}
	}

	cb(opaque, used_len);
	return 0;
}

static struct virtq *get_virtqueue(const struct device *dev, uint16_t queue)
{
	static struct virtq vq;

	ARG_UNUSED(dev);
	ARG_UNUSED(queue);
	return &vq;
}

static void notify_virtqueue(const struct device *dev, uint16_t queue)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(queue);
	transfer.notifications++;
}

static DEVICE_API(virtio, test_api) = {
	.get_virtqueue = get_virtqueue,
	.notify_virtqueue = notify_virtqueue,
};

static int send_buffers(struct virtq_buf *bufs, uint16_t count, uint16_t readable,
			uint32_t *used_len)
{
	const struct device dev = {.api = &test_api};

	transfer.original = bufs;
	transfer.count = count;
	transfer.readable = readable;
	return virtiofs_send_receive_checked(&dev, 1U, bufs, count, readable, used_len);
}

ZTEST(virtiofs_buffers, test_direct_buffers_do_not_allocate)
{
	static uint8_t data[2][32];
	struct virtq_buf bufs[] = {
		{.addr = data[0], .len = sizeof(data[0])},
		{.addr = data[1], .len = sizeof(data[1])},
	};
	uint32_t used_len;

	memset(data[0], 0x39, sizeof(data[0]));
	transfer.fail_allocation = true;
	zassert_ok(send_buffers(bufs, ARRAY_SIZE(bufs), 1U, &used_len));
	zassert_equal(used_len, sizeof(data[1]));
	zassert_equal(transfer.bounced, 0U);
	zassert_equal(transfer.allocations, 0U);
	zassert_equal(transfer.frees, 0U);
	zassert_equal(data[0][0], 0x39);
	zassert_equal(data[1][0], 0x5a);
}

ZTEST(virtiofs_buffers, test_mixed_buffers_only_allocate_for_runtime_mappings)
{
#ifdef CONFIG_MMU
	static uint8_t data[2][4096];
	uint8_t *mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(mapped);

	struct virtq_buf bufs[] = {
		{.addr = mapped, .len = 7U},
		{.addr = data[0], .len = sizeof(data[0])},
		{.addr = mapped + 16, .len = 9U},
		{.addr = data[1], .len = sizeof(data[1])},
	};
	uint32_t used_len;

	memset(mapped, 0x37, 7U);
	zassert_ok(send_buffers(bufs, ARRAY_SIZE(bufs), 2U, &used_len));
	zassert_equal(used_len, 9U + sizeof(data[1]));
	zassert_equal(transfer.bounced, BIT(0) | BIT(2));
	zassert_equal(transfer.allocation_size, 16U);
	zassert_equal(transfer.allocations, 1U);
	zassert_equal(transfer.frees, 1U);
	zassert_equal(mapped[0], 0x37);
	zassert_equal(mapped[16], 0x5a);
	zassert_equal(data[1][0], 0x5a);
	k_mem_unmap(mapped, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(virtiofs_buffers, test_bounced_buffers_preserve_both_directions)
{
#ifdef CONFIG_MMU
	uint8_t *mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(mapped);

	struct virtq_buf bufs[] = {
		{.addr = mapped, .len = 5U},
		{.addr = mapped + 16, .len = 13U},
	};
	uint32_t used_len;

	memset(mapped, 0x47, 5U);
	zassert_ok(send_buffers(bufs, ARRAY_SIZE(bufs), 1U, &used_len));
	zassert_equal(used_len, 13U);
	zassert_equal(transfer.bounced, BIT(0) | BIT(1));
	zassert_equal(transfer.allocation_size, 18U);
	zassert_equal(transfer.frees, 1U);
	zassert_equal(mapped[0], 0x47);
	zassert_equal(mapped[16], 0x5a);
	k_mem_unmap(mapped, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(virtiofs_buffers, test_empty_unmapped_buffer_has_valid_descriptor)
{
	static uint8_t data[8];
	struct virtq_buf bufs[] = {
		{.addr = data, .len = sizeof(data)},
		{.addr = NULL, .len = 0U},
	};
	uint32_t used_len;

	zassert_ok(send_buffers(bufs, ARRAY_SIZE(bufs), 1U, &used_len));
	zassert_equal(used_len, 0U);
	zassert_equal(transfer.bounced, BIT(1));
	zassert_equal(transfer.allocation_size, 1U);
	zassert_equal(transfer.frees, 1U);
}

ZTEST(virtiofs_buffers, test_allocation_failure_does_not_enqueue)
{
	struct virtq_buf buf = {.addr = NULL, .len = 0U};
	uint32_t used_len = 42U;

	transfer.fail_allocation = true;
	zassert_equal(send_buffers(&buf, 1U, 0U, &used_len), -ENOMEM);
	zassert_equal(used_len, 42U);
	zassert_equal(transfer.submissions, 0U);
	zassert_equal(transfer.notifications, 0U);
	zassert_equal(transfer.frees, 0U);
}

ZTEST(virtiofs_buffers, test_enqueue_failure_frees_bounce)
{
	struct virtq_buf buf = {.addr = NULL, .len = 0U};
	uint32_t used_len = 42U;

	transfer.enqueue_error = -EIO;
	zassert_equal(send_buffers(&buf, 1U, 0U, &used_len), -EIO);
	zassert_equal(used_len, 42U);
	zassert_equal(transfer.submissions, 1U);
	zassert_equal(transfer.notifications, 0U);
	zassert_equal(transfer.frees, 1U);
}

ZTEST(virtiofs_buffers, test_direct_range_contains_entire_buffer)
{
#ifdef CONFIG_MMU
	uintptr_t end = POINTER_TO_UINT(z_mapped_end);
#else
	uintptr_t end = DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE;
#endif
	struct virtq_buf buf = {.addr = UINT_TO_POINTER(end - 1U), .len = 1U};

	zassert_true(virtiofs_buffer_is_direct(&buf));
	buf.len = 2U;
	zassert_false(virtiofs_buffer_is_direct(&buf));
	buf.addr = UINT_TO_POINTER(UINTPTR_MAX - 1U);
	buf.len = 4U;
	zassert_false(virtiofs_buffer_is_direct(&buf));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&transfer, 0, sizeof(transfer));
}

ZTEST_SUITE(virtiofs_buffers, NULL, NULL, before, NULL, NULL);
