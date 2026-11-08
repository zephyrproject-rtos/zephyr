/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exercise the request pool through the real virtqueue and interrupt path.
 * Include the driver to inspect its private request state without a disk device.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include <string.h>

#include "fake_device.h"

/* The driver is included below; enabling its Kconfig menu would build it twice. */
#define CONFIG_DISK_VIRTIO_BLK_MAX_SEGMENTS 4
#define CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE  512
#define CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT 3
#define CONFIG_DISK_VIRTIO_BLK_LOG_LEVEL    0

#include "virtio_blk.c"

static const void *const kept_symbols[] __unused = {
	(void *)virtio_blk_dev_init,
	(void *)&virtio_blk_ops,
};

#define POOL_DEPTH       CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT
#define REUSE_ITERATIONS (2 * POOL_DEPTH)

K_THREAD_STACK_ARRAY_DEFINE(submit_stacks, POOL_DEPTH, 4096);
static struct k_thread submit_threads[POOL_DEPTH];
static uint8_t submit_buffers[POOL_DEPTH][VIRTIO_BLK_MAX_DATA_SEGS][512];
static struct virtio_blk_data data;
static struct virtq vq;

struct submitter {
	uint16_t segments;
	uint16_t iterations;
	int result;
	atomic_t done;
	bool active;
};

static struct submitter submitters[POOL_DEPTH];

static void reset_pool(uint16_t queue_size)
{
	memset(&data, 0, sizeof(data));
	memset(submitters, 0, sizeof(submitters));
	zassert_ok(virtq_create(&vq, queue_size));
	fake_device_attach_queue(fake_device_a(), &vq);
	data.vdev = fake_device_a();
	data.max_data_segs = VIRTIO_BLK_MAX_DATA_SEGS;
	data.sector_size = 512;
	data.sectors_per_block = 1;

	k_stack_init(&data.free, data.free_idx, ARRAY_SIZE(data.free_idx));
	for (size_t i = 0; i < ARRAY_SIZE(data.free_idx); i++) {
		k_sem_init(&data.requests[i].sem, 0, 1);
		zassert_ok(k_stack_push(&data.free, (stack_data_t)i));
	}
}

static void submit_thread(void *p1, void *p2, void *p3)
{
	uint32_t idx = POINTER_TO_UINT(p1);
	struct submitter *s = &submitters[idx];
	struct virtq_buf segs[VIRTIO_BLK_MAX_DATA_SEGS];
	bool write = (idx % 2U) != 0U;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (uint16_t i = 0; i < s->segments; i++) {
		segs[i].addr = submit_buffers[idx][i];
		segs[i].len = sizeof(submit_buffers[idx][i]);
	}

	for (uint16_t i = 0; i < s->iterations; i++) {
		s->result = virtio_blk_submit(&data, write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN,
					      idx, segs, s->segments, write);
		if (s->result != 0) {
			break;
		}
	}
	atomic_set(&s->done, 1);
}

static void start_submitter(uint32_t idx, uint16_t segments, uint16_t iterations)
{
	submitters[idx].segments = segments;
	submitters[idx].iterations = iterations;
	submitters[idx].active = true;
	k_thread_create(&submit_threads[idx], submit_stacks[idx],
			K_THREAD_STACK_SIZEOF(submit_stacks[idx]), submit_thread,
			UINT_TO_POINTER(idx), NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
}

static void join_submitter(uint32_t idx)
{
	zassert_ok(k_thread_join(&submit_threads[idx], K_SECONDS(1)), "worker %u did not exit",
		   idx);
	submitters[idx].active = false;
}

static uint16_t published_count(void)
{
	k_spinlock_key_t key = k_spin_lock(&vq.lock);
	uint16_t count = sys_le16_to_cpu(vq.avail->idx);

	k_spin_unlock(&vq.lock, key);
	return count;
}

static void wait_for_published(uint16_t count)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_SECONDS(1));

	while (published_count() < count && !sys_timepoint_expired(deadline)) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(published_count(), count, "requests did not reach the queue");
}

static uint16_t published_head(uint16_t index)
{
	return sys_le16_to_cpu(vq.avail->ring[index % vq.num]);
}

static struct virtio_blk_request *check_chain(uint16_t head)
{
	struct virtio_blk_request *req = vq.recv_cbs[head].opaque;
	uint32_t idx;
	uint16_t desc = head;
	uint16_t flags;

	zassert_not_null(req);
	zassert_true(vq.recv_cbs[head].cb == virtio_blk_complete);
	zassert_equal(sys_le64_to_cpu(vq.desc[head].addr), k_mem_phys_addr(&req->hdr));
	zassert_equal(sys_le32_to_cpu(vq.desc[head].len), sizeof(req->hdr));
	zassert_equal(sys_le16_to_cpu(vq.desc[head].flags), VIRTQ_DESC_F_NEXT);
	zassert_equal(req->hdr.reserved, 0);
	zassert_equal(req->status, 0xff);
	idx = (uint32_t)sys_le64_to_cpu(req->hdr.sector);
	zassert_true(idx < POOL_DEPTH);
	zassert_equal(sys_le32_to_cpu(req->hdr.type),
		      (idx % 2U) != 0U ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN);

	for (uint16_t i = 0; i < submitters[idx].segments; i++) {
		desc = sys_le16_to_cpu(vq.desc[desc].next);
		zassert_true(desc < vq.num);
		flags = VIRTQ_DESC_F_NEXT | ((idx % 2U) != 0U ? 0U : VIRTQ_DESC_F_WRITE);
		zassert_equal(sys_le16_to_cpu(vq.desc[desc].flags), flags);
		zassert_equal(sys_le64_to_cpu(vq.desc[desc].addr),
			      k_mem_phys_addr(submit_buffers[idx][i]));
		zassert_equal(sys_le32_to_cpu(vq.desc[desc].len), sizeof(submit_buffers[idx][i]));
	}

	desc = sys_le16_to_cpu(vq.desc[desc].next);
	zassert_true(desc < vq.num);
	zassert_equal(sys_le16_to_cpu(vq.desc[desc].flags), VIRTQ_DESC_F_WRITE);
	zassert_equal(sys_le32_to_cpu(vq.desc[desc].len), sizeof(req->status));
	zassert_equal(sys_le64_to_cpu(vq.desc[desc].addr), k_mem_phys_addr(&req->status));

	return req;
}

static void complete_chain(uint16_t head, uint8_t status)
{
	uint16_t desc = head;

	for (uint16_t i = 0; i < vq.num; i++) {
		if ((sys_le16_to_cpu(vq.desc[desc].flags) & VIRTQ_DESC_F_NEXT) == 0U) {
			uint8_t *addr = k_mem_virt_addr(sys_le64_to_cpu(vq.desc[desc].addr));

			/* Write the status through the descriptor visible to the device. */
			*addr = status;
			fake_device_complete_request(fake_device_a(), head, 1);
			return;
		}
		desc = sys_le16_to_cpu(vq.desc[desc].next);
		zassert_true(desc < vq.num);
	}
	zassert_unreachable("unterminated request chain");
}

ZTEST(virtio_blk_pool, test_concurrent_submit)
{
	struct virtio_blk_request *requests[POOL_DEPTH];
	const uint8_t statuses[] = {VIRTIO_BLK_S_UNSUPP, VIRTIO_BLK_S_IOERR, VIRTIO_BLK_S_OK};
	const int results[] = {-ENOTSUP, -EIO, 0};

	reset_pool(VIRTIO_BLK_REQUEST_QUEUE_SIZE);
	for (uint32_t i = 0; i < POOL_DEPTH; i++) {
		start_submitter(i, i == 1U ? VIRTIO_BLK_MAX_DATA_SEGS : 1U, 1);
	}

	/* Every request must be in flight before the first completion. */
	wait_for_published(POOL_DEPTH);
	for (uint16_t i = 0; i < POOL_DEPTH; i++) {
		requests[i] = check_chain(published_head(i));
		for (uint16_t j = 0; j < i; j++) {
			zassert_not_equal(requests[i], requests[j], "request state was shared");
		}
	}

	/* Out-of-order completions must wake only the matching caller. */
	for (uint16_t i = POOL_DEPTH; i > 0U; i--) {
		uint32_t idx = (uint32_t)sys_le64_to_cpu(requests[i - 1U]->hdr.sector);

		complete_chain(published_head(i - 1U), statuses[idx]);
		join_submitter(idx);
		zassert_equal(submitters[idx].result, results[idx]);
		for (uint16_t j = 0; j < i - 1U; j++) {
			idx = (uint32_t)sys_le64_to_cpu(requests[j]->hdr.sector);
			zassert_equal(atomic_get(&submitters[idx].done), 0,
				      "completion woke another caller");
		}
	}
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
}

ZTEST(virtio_blk_pool, test_capacity_limited_no_deadlock)
{
	k_timepoint_t deadline;

	/* One three-descriptor request fits; two cannot fit together. */
	reset_pool(4);
	for (uint32_t i = 0; i < POOL_DEPTH; i++) {
		start_submitter(i, 1, 1);
	}
	wait_for_published(1);
	deadline = sys_timepoint_calc(K_SECONDS(1));
	while (atomic_get(&vq.waiters) < POOL_DEPTH - 1 && !sys_timepoint_expired(deadline)) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(atomic_get(&vq.waiters), POOL_DEPTH - 1,
		      "queue waiters were serialized before reservation");
	zassert_equal(atomic_get(&vq.free_desc_n), 1, "waiters held partial chains");

	for (uint16_t i = 0; i < POOL_DEPTH; i++) {
		wait_for_published(i + 1U);
		check_chain(published_head(i));
		complete_chain(published_head(i), VIRTIO_BLK_S_OK);
	}
	for (uint32_t i = 0; i < POOL_DEPTH; i++) {
		join_submitter(i);
		zassert_ok(submitters[i].result);
	}
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
}

ZTEST(virtio_blk_pool, test_slot_reuse)
{
	reset_pool(VIRTIO_BLK_REQUEST_QUEUE_SIZE);
	start_submitter(0, 1, REUSE_ITERATIONS);

	/* More submissions than slots must progress after each completion. */
	for (uint16_t i = 0; i < REUSE_ITERATIONS; i++) {
		wait_for_published(i + 1U);
		check_chain(published_head(i));
		complete_chain(published_head(i), VIRTIO_BLK_S_OK);
	}
	join_submitter(0);
	zassert_ok(submitters[0].result);
}

ZTEST(virtio_blk_pool, test_submission_error_returns_slot)
{
	struct virtq_buf seg = {.addr = submit_buffers[0][0], .len = 512};
	stack_data_t slots[POOL_DEPTH];

	reset_pool(2);
	zassert_equal(virtio_blk_submit(&data, VIRTIO_BLK_T_IN, 0, &seg, 1, false), -EINVAL);
	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		zassert_ok(k_stack_pop(&data.free, &slots[i], K_NO_WAIT), "request slot leaked");
	}
	zassert_equal(published_count(), 0);
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
}

ZTEST(virtio_blk_pool, test_queue_size)
{
	reset_pool(VIRTIO_BLK_REQUEST_QUEUE_SIZE);
	zassert_equal(virtio_blk_enum_queues_cb(0, 256, &data), VIRTIO_BLK_REQUEST_QUEUE_SIZE);
	zassert_equal(virtio_blk_enum_queues_cb(0, 4, &data), 4);
	zassert_equal(virtio_blk_enum_queues_cb(1, 256, &data), 0);
}

static void pool_after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (uint32_t i = 0; i < POOL_DEPTH; i++) {
		if (submitters[i].active) {
			k_thread_abort(&submit_threads[i]);
			submitters[i].active = false;
		}
	}
	fake_device_attach_queue(fake_device_a(), NULL);
	if (vq.desc != NULL) {
		virtq_free(&vq);
		vq.desc = NULL;
	}
}

ZTEST_SUITE(virtio_blk_pool, NULL, NULL, NULL, pool_after, NULL);
