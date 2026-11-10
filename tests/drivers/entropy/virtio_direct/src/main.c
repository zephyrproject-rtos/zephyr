/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "fake_device.h"

static void entropy_notify(const struct device *dev, uint16_t index);

/* Include the private driver without instantiating a hardware device. */
#define virtio_notify_virtqueue entropy_notify
#include "entropy_virtio.c"
#undef virtio_notify_virtqueue

enum response_mode {
	RESPONSE_VALID,
	RESPONSE_EMPTY,
	RESPONSE_OVERSIZED,
};

static struct entropy_virtio_data data;
static struct entropy_virtio_config config;
static struct device entropy_dev = {
	.data = &data,
	.config = &config,
	.api = &entropy_virtio_api,
};
static struct virtq vq;

static struct {
	uint8_t *buffer;
	uint16_t length;
	uint32_t completed;
	uint32_t notifications;
	uint32_t limit;
	enum response_mode mode;
	bool direct;
	bool defer;
	uint16_t pending_head;
	struct k_sem pending;
} transfer;

static uint8_t entropy_byte(size_t offset)
{
	return (uint8_t)(offset * 17U + 0x2dU);
}

static void complete(uint16_t head)
{
	uint32_t size = sys_le32_to_cpu(vq.desc[head].len);
	uint32_t received = MIN(size, transfer.limit);
	uint8_t *target = k_mem_virt_addr(sys_le64_to_cpu(vq.desc[head].addr));

	if (transfer.mode == RESPONSE_EMPTY) {
		received = 0U;
	} else if (transfer.mode == RESPONSE_OVERSIZED) {
		received = size + 1U;
	} else {
		for (size_t i = 0; i < received; i++) {
			target[i] = entropy_byte(transfer.completed + i);
		}
		transfer.completed += received;
	}

	fake_device_complete_request(config.vdev, head, received);
}

static void entropy_notify(const struct device *dev, uint16_t index)
{
	uint16_t head = fake_device_last_submitted_head(&vq);
	uint32_t remaining = transfer.length - transfer.completed;
	uint8_t *expected = transfer.direct ? transfer.buffer + transfer.completed : data.buf;

	zassert_equal(dev, config.vdev);
	zassert_equal(index, VIRTIO_ENTROPY_QUEUE_IDX);
	zassert_equal(sys_le16_to_cpu(vq.desc[head].flags), VIRTQ_DESC_F_WRITE);
	zassert_equal(sys_le64_to_cpu(vq.desc[head].addr), k_mem_phys_addr(expected));
	zassert_equal(sys_le32_to_cpu(vq.desc[head].len),
		      transfer.direct ? remaining : MIN(remaining, ENTROPY_BUFFER_MAX_LEN));
	transfer.notifications++;

	if (transfer.defer) {
		transfer.pending_head = head;
		k_sem_give(&transfer.pending);
	} else {
		complete(head);
	}
}

static void prepare(uint8_t *buffer, uint16_t length, bool direct)
{
	memset(&transfer, 0, sizeof(transfer));
	transfer.buffer = buffer;
	transfer.length = length;
	transfer.direct = direct;
	transfer.limit = UINT32_MAX;
	k_sem_init(&transfer.pending, 0, 1);
	memset(data.buf, 0xa5, sizeof(data.buf));
	memset(buffer, 0xa5, length);
	zassert_equal(virtio_buf_is_in_linear_mapping(buffer, length), direct);
}

static void check_payload(const uint8_t *buffer, uint16_t length)
{
	for (size_t i = 0; i < length; i++) {
		zassert_equal(buffer[i], entropy_byte(i), "incorrect entropy byte at %zu", i);
	}
	zassert_equal(transfer.completed, length);
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
	if (transfer.direct) {
		for (size_t i = 0; i < sizeof(data.buf); i++) {
			zassert_equal(data.buf[i], 0xa5, "direct request used bounce storage");
		}
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&data, 0, sizeof(data));
	config.vdev = fake_device_a();
	zassert_ok(virtq_create(&vq, 1));
	fake_device_attach_queue(config.vdev, &vq);
	zassert_ok(entropy_virtio_init(&entropy_dev));
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
	fake_device_attach_queue(config.vdev, NULL);
	virtq_free(&vq);
}

ZTEST(virtio_entropy_direct, test_static_buffer)
{
	static uint8_t buffer[1026];
	bool direct = !IS_ENABLED(CONFIG_DEMAND_PAGING);

	buffer[0] = 0x37;
	buffer[sizeof(buffer) - 1] = 0x73;
	prepare(buffer + 1, sizeof(buffer) - 2, direct);
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer + 1, sizeof(buffer) - 2));
	check_payload(buffer + 1, sizeof(buffer) - 2);
	zassert_equal(transfer.notifications, direct ? 1U : 4U);
	zassert_equal(buffer[0], 0x37);
	zassert_equal(buffer[sizeof(buffer) - 1], 0x73);
}

ZTEST(virtio_entropy_direct, test_maximum_length)
{
	static uint8_t buffer[UINT16_MAX + 2U];
	bool direct = !IS_ENABLED(CONFIG_DEMAND_PAGING);

	buffer[0] = 0x37;
	buffer[sizeof(buffer) - 1] = 0x73;
	prepare(buffer + 1, UINT16_MAX, direct);
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer + 1, UINT16_MAX));
	check_payload(buffer + 1, UINT16_MAX);
	zassert_equal(transfer.notifications, direct ? 1U : 256U);
	zassert_equal(buffer[0], 0x37);
	zassert_equal(buffer[sizeof(buffer) - 1], 0x73);
}

ZTEST(virtio_entropy_direct, test_zero_length)
{
	uint8_t buffer = 0xa5;

	prepare(&buffer, 0, virtio_buf_is_in_linear_mapping(&buffer, 0));
	zassert_ok(entropy_get_entropy(&entropy_dev, &buffer, 0));
	zassert_equal(transfer.notifications, 0);
	zassert_equal(buffer, 0xa5);
}

ZTEST(virtio_entropy_direct, test_stack_buffer)
{
	uint8_t buffer[770];
	bool direct =
		!IS_ENABLED(CONFIG_THREAD_STACK_MEM_MAPPED) && !IS_ENABLED(CONFIG_DEMAND_PAGING);

	buffer[0] = 0x37;
	buffer[sizeof(buffer) - 1] = 0x73;
	prepare(buffer + 1, sizeof(buffer) - 2, direct);
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer + 1, sizeof(buffer) - 2));
	check_payload(buffer + 1, sizeof(buffer) - 2);
	zassert_equal(transfer.notifications, direct ? 1U : 3U);
	zassert_equal(buffer[0], 0x37);
	zassert_equal(buffer[sizeof(buffer) - 1], 0x73);
}

ZTEST(virtio_entropy_direct, test_runtime_mapping)
{
#if defined(CONFIG_MMU)
	uint8_t *buffer = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(buffer);
	buffer[0] = 0x37;
	buffer[1025] = 0x73;
	prepare(buffer + 1, 1024, false);
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer + 1, 1024));
	check_payload(buffer + 1, 1024);
	zassert_equal(transfer.notifications, 4U);
	zassert_equal(buffer[0], 0x37);
	zassert_equal(buffer[1025], 0x73);
	k_mem_unmap(buffer, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(virtio_entropy_direct, test_partial_direct_completion)
{
	static uint8_t buffer[768];

	prepare(buffer, sizeof(buffer), !IS_ENABLED(CONFIG_DEMAND_PAGING));
	transfer.limit = 113;
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer, sizeof(buffer)));
	check_payload(buffer, sizeof(buffer));
	zassert_equal(transfer.notifications, 7U);
}

ZTEST(virtio_entropy_direct, test_partial_bounce_completion)
{
#if defined(CONFIG_MMU)
	uint8_t *buffer = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(buffer);
	prepare(buffer, 768, false);
	transfer.limit = 113;
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer, 768));
	check_payload(buffer, 768);
	zassert_equal(transfer.notifications, 7U);
	k_mem_unmap(buffer, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

static void check_invalid_completions(uint8_t *buffer, bool direct)
{
	for (enum response_mode mode = RESPONSE_EMPTY; mode <= RESPONSE_OVERSIZED; mode++) {
		prepare(buffer, 32, direct);
		transfer.mode = mode;
		zassert_equal(entropy_get_entropy(&entropy_dev, buffer, 32), -EIO);
		zassert_equal(transfer.notifications, 1U);
		for (size_t i = 0; i < 32; i++) {
			zassert_equal(buffer[i], 0xa5);
		}

		prepare(buffer, 32, direct);
		zassert_ok(entropy_get_entropy(&entropy_dev, buffer, 32));
		check_payload(buffer, 32);
	}
}

ZTEST(virtio_entropy_direct, test_invalid_direct_completions)
{
	static uint8_t buffer[32];

	check_invalid_completions(buffer, !IS_ENABLED(CONFIG_DEMAND_PAGING));
}

ZTEST(virtio_entropy_direct, test_invalid_bounce_completions)
{
#if defined(CONFIG_MMU)
	uint8_t *buffer = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(buffer);
	check_invalid_completions(buffer, false);
	k_mem_unmap(buffer, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(virtio_entropy_direct, test_enqueue_failure)
{
	static uint8_t buffer[32];
	bool direct = !IS_ENABLED(CONFIG_DEMAND_PAGING);

	prepare(buffer, sizeof(buffer), direct);
	/* A zero-size queue rejects the chain before allocating descriptors. */
	vq.num = 0U;
	zassert_equal(entropy_get_entropy(&entropy_dev, buffer, sizeof(buffer)), -EIO);
	zassert_equal(transfer.notifications, 0U);
	vq.num = 1U;

	prepare(buffer, sizeof(buffer), direct);
	zassert_ok(entropy_get_entropy(&entropy_dev, buffer, sizeof(buffer)));
	check_payload(buffer, sizeof(buffer));
}

struct request {
	uint8_t *buffer;
	int result;
	struct k_sem done;
};

static void request_thread(void *arg, void *unused2, void *unused3)
{
	struct request *request = arg;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);
	request->result = entropy_get_entropy(&entropy_dev, request->buffer, 64);
	k_sem_give(&request->done);
}

ZTEST(virtio_entropy_direct, test_waits_for_completion)
{
	static K_THREAD_STACK_DEFINE(stack, 4096);

	static uint8_t buffer[64];
	struct request request = {.buffer = buffer};
	struct k_thread thread;

	prepare(buffer, sizeof(buffer), !IS_ENABLED(CONFIG_DEMAND_PAGING));
	transfer.defer = true;
	k_sem_init(&request.done, 0, 1);
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), request_thread, &request,
			NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&transfer.pending, K_SECONDS(1)));
	zassert_equal(k_sem_take(&request.done, K_NO_WAIT), -EBUSY);
	zassert_equal(buffer[0], 0xa5);
	zassert_equal(atomic_get(&vq.free_desc_n), 0);

	complete(transfer.pending_head);
	zassert_ok(k_sem_take(&request.done, K_SECONDS(1)));
	zassert_ok(k_thread_join(&thread, K_SECONDS(1)));
	zassert_ok(request.result);
	check_payload(buffer, sizeof(buffer));
}

ZTEST_SUITE(virtio_entropy_direct, NULL, NULL, before, after, NULL);
