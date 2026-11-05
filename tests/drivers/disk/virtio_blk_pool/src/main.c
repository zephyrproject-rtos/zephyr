/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 *
 * White-box concurrency test for the virtio-blk request pool. The source is
 * included directly so the test can substitute virtq_add_buffer_chain() and
 * observe how many requests are in flight before any completion happens. A
 * second case limits the descriptors the fake queue hands out, so a deadlock
 * from interleaved chain construction shows up as a timeout. A third case
 * submits more requests than the pool depth, one at a time, and checks that
 * completed slots are reusable.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/ztest.h>
#include <string.h>

/*
 * The driver is compiled into this translation unit and no virtio-blk device
 * is instantiated. That keeps CONFIG_DISK_DRIVER_VIRTIO_BLK off, so its
 * Kconfig menu, which the driver's options live under, is not generated.
 * Supply the values the driver reads; enabling the menu would also compile
 * the driver a second time and collide with the copy included below.
 */
#define CONFIG_DISK_VIRTIO_BLK_MAX_SEGMENTS 4
#define CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE  512
#define CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT 3
#define CONFIG_DISK_VIRTIO_BLK_LOG_LEVEL    0

#include "virtio_blk.c"

/* The test calls the submit path directly; keep the unused driver entry points. */
static const void *const kept_symbols[] __unused = {
	(void *)virtio_blk_dev_init,
	(void *)&virtio_blk_ops,
};

#define POOL_DEPTH CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT

/*
 * Descriptors available to the capacity case. A read request needs three
 * (header, one data segment, status). With four, two callers that each take
 * two before blocking leave both waiting for a third, so neither completes.
 */
#define DESC_BUDGET       4
#define DESC_BUDGET_LIMIT 16

/* Twice the pool depth, so every slot must be returned and acquired again. */
#define REUSE_ITERATIONS (2 * POOL_DEPTH)

K_THREAD_STACK_ARRAY_DEFINE(submit_stacks, POOL_DEPTH, 4096);
static struct k_thread submit_threads[POOL_DEPTH];
static int submit_result[POOL_DEPTH];
static uint8_t submit_buffers[POOL_DEPTH][512];

struct recorded {
	virtq_receive_callback cb;
	void *opaque;
	void *status_addr;
	uint16_t ndesc;
};

static struct recorded recorded[POOL_DEPTH];
static atomic_t submitted;
static atomic_t published;
static atomic_t chain_active;
static struct k_sem all_submitted;
static struct k_sem chain_published;
static struct k_sem desc_budget;
static bool budget_enabled;
static bool reuse_mode;
static atomic_t budget_aborting;
static atomic_t mock_errors;
static struct virtq fake_vq;
static struct virtio_blk_data data;

/*
 * The mock runs in the submitter threads, where zassert() cannot report a
 * failure reliably. Record violations here and let the test thread assert
 * after the workers have been joined.
 */
enum mock_error {
	MOCK_ERROR_CONCURRENT_CHAIN = BIT(0),
	MOCK_ERROR_CHAIN_TOO_SHORT = BIT(1),
	MOCK_ERROR_READABLE_COUNT = BIT(2),
	MOCK_ERROR_STATUS_LEN = BIT(3),
	MOCK_ERROR_POOL_DEPTH = BIT(4),
};

static struct virtq *fake_get_virtqueue(const struct device *dev, uint16_t queue)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(queue);

	return &fake_vq;
}

static void fake_notify_virtqueue(const struct device *dev, uint16_t queue)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(queue);
}

static DEVICE_API(virtio, fake_api) = {
	.get_virtqueue = fake_get_virtqueue,
	.notify_virtqueue = fake_notify_virtqueue,
};

static const struct device fake_dev = {.api = &fake_api};

/*
 * Take descriptors one at a time, as the real queue does, so a partial chain
 * can be held. Yield after each take: with the submit lock the other
 * submitters are blocked before this point, without it they run here and the
 * partial holds exhaust the budget.
 */
static int fake_take_descs(uint16_t count)
{
	uint16_t taken = 0;

	while (taken < count && !atomic_get(&budget_aborting)) {
		k_sem_take(&desc_budget, K_FOREVER);
		taken++;
		k_yield();
	}

	if (atomic_get(&budget_aborting)) {
		for (uint16_t i = 0; i < taken; i++) {
			(void)k_sem_give(&desc_budget);
		}
		return -EIO;
	}

	return 0;
}

int virtq_add_buffer_chain(struct virtq *vq, struct virtq_buf *bufs, uint16_t count,
			   uint16_t readable, virtq_receive_callback cb, void *opaque,
			   k_timeout_t timeout)
{
	atomic_val_t prev;
	int idx;

	ARG_UNUSED(vq);
	ARG_UNUSED(timeout);

	prev = atomic_inc(&chain_active);
	if (!budget_enabled && prev != 0) {
		atomic_or(&mock_errors, MOCK_ERROR_CONCURRENT_CHAIN);
	}

	/*
	 * Yield to let another submitter run inside this call. With the submit
	 * lock it blocks before this point; without the lock it lands here and
	 * the chain_active check above records the overlap.
	 */
	k_yield();

	if (count < 2U) {
		atomic_dec(&chain_active);
		atomic_or(&mock_errors, MOCK_ERROR_CHAIN_TOO_SHORT);
		return -EINVAL;
	}

	if (readable != 1U) {
		atomic_or(&mock_errors, MOCK_ERROR_READABLE_COUNT);
	}

	if (bufs[count - 1].len != 1U) {
		atomic_or(&mock_errors, MOCK_ERROR_STATUS_LEN);
	}

	if (budget_enabled) {
		int ret = fake_take_descs(count);

		if (ret != 0) {
			atomic_dec(&chain_active);
			return ret;
		}
	}

	idx = (int)atomic_inc(&submitted);
	if (reuse_mode) {
		idx %= POOL_DEPTH;
	} else if (idx >= POOL_DEPTH) {
		atomic_dec(&chain_active);
		atomic_or(&mock_errors, MOCK_ERROR_POOL_DEPTH);
		return -EINVAL;
	}

	recorded[idx].cb = cb;
	recorded[idx].opaque = opaque;
	recorded[idx].status_addr = bufs[count - 1].addr;
	recorded[idx].ndesc = count;
	if (atomic_inc(&published) == POOL_DEPTH - 1) {
		k_sem_give(&all_submitted);
	}

	k_sem_give(&chain_published);

	atomic_dec(&chain_active);

	return 0;
}

static void submit_thread(void *p1, void *p2, void *p3)
{
	int idx = POINTER_TO_INT(p1);
	struct virtq_buf seg = {
		.addr = submit_buffers[idx],
		.len = sizeof(submit_buffers[idx]),
	};

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	submit_result[idx] = virtio_blk_submit(&data, VIRTIO_BLK_T_IN, 0, &seg, 1, false);
}

static void reset_pool(void)
{
	data.vdev = &fake_dev;
	data.max_data_segs = 4;
	data.sector_size = 512;
	data.sectors_per_block = 1;

	memset(recorded, 0, sizeof(recorded));
	atomic_clear(&submitted);
	atomic_clear(&published);
	atomic_clear(&chain_active);
	atomic_clear(&mock_errors);

	k_sem_init(&all_submitted, 0, 1);
	k_sem_init(&chain_published, 0, POOL_DEPTH);
	budget_enabled = false;
	reuse_mode = false;
	atomic_clear(&budget_aborting);

	k_mutex_init(&data.submit_lock);
	k_stack_init(&data.free, data.free_idx, ARRAY_SIZE(data.free_idx));
	for (size_t i = 0; i < ARRAY_SIZE(data.free_idx); i++) {
		k_sem_init(&data.requests[i].sem, 0, 1);
		zassert_ok(k_stack_push(&data.free, (stack_data_t)i));
	}
}

static void start_submitters(void)
{
	for (int i = 0; i < POOL_DEPTH; i++) {
		k_thread_create(&submit_threads[i], submit_stacks[i],
				K_THREAD_STACK_SIZEOF(submit_stacks[i]), submit_thread,
				INT_TO_POINTER(i), NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	}
}

static void complete_recorded(int idx)
{
	struct virtio_blk_request *req = recorded[idx].opaque;

	*(uint8_t *)recorded[idx].status_addr = VIRTIO_BLK_S_OK;

	if (budget_enabled) {
		for (uint16_t i = 0; i < recorded[idx].ndesc; i++) {
			(void)k_sem_give(&desc_budget);
		}
	}

	recorded[idx].cb(req, 0);
}

/* Submit sequentially from one thread; the test completes each request. */
static void reuse_thread(void *p1, void *p2, void *p3)
{
	struct virtq_buf seg = {
		.addr = submit_buffers[0],
		.len = sizeof(submit_buffers[0]),
	};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	submit_result[0] = 0;
	for (int i = 0; i < REUSE_ITERATIONS; i++) {
		submit_result[0] = virtio_blk_submit(&data, VIRTIO_BLK_T_IN, 0, &seg, 1, false);
		if (submit_result[0] != 0) {
			return;
		}
	}
}

ZTEST(virtio_blk_pool, test_concurrent_submit)
{
	bool bad = false;
	int ret;

	reset_pool();
	start_submitters();

	/*
	 * All submissions must reach the virtqueue before any completion. With a
	 * device-wide mutex only one would get here and this would time out.
	 */
	ret = k_sem_take(&all_submitted, K_SECONDS(1));
	if (ret != 0) {
		/*
		 * Best-effort cleanup: complete whatever was recorded so the
		 * workers can leave the submit path, then join them. A regression
		 * that serializes submissions can require several passes.
		 */
		for (int pass = 0; pass < POOL_DEPTH; pass++) {
			for (int i = 0; i < POOL_DEPTH; i++) {
				if (recorded[i].opaque != NULL) {
					struct virtio_blk_request *req = recorded[i].opaque;

					req->status = VIRTIO_BLK_S_OK;
					recorded[i].cb(req, 0);
					recorded[i].opaque = NULL;
				}
			}
			for (int i = 0; i < POOL_DEPTH; i++) {
				(void)k_thread_join(&submit_threads[i], K_MSEC(50));
			}
		}
		zassert_ok(ret, "submissions were serialized");
	}

	zassert_equal(atomic_get(&submitted), POOL_DEPTH);

	for (int i = 0; i < POOL_DEPTH; i++) {
		struct virtio_blk_request *req = recorded[i].opaque;

		if (req == NULL) {
			bad = true;
			continue;
		}

		if (recorded[i].status_addr == (void *)&req->status) {
			/* Write the status through the descriptor the driver queued. */
			*(uint8_t *)recorded[i].status_addr = VIRTIO_BLK_S_OK;
		} else {
			/* Keep the worker moving, but fail the descriptor check. */
			req->status = VIRTIO_BLK_S_OK;
			bad = true;
		}

		for (int j = i + 1; j < POOL_DEPTH; j++) {
			if (recorded[i].status_addr == recorded[j].status_addr) {
				bad = true;
			}
		}

		recorded[i].cb(req, 0);
	}

	for (int i = 0; i < POOL_DEPTH; i++) {
		zassert_ok(k_thread_join(&submit_threads[i], K_SECONDS(1)),
			   "worker %d did not exit", i);
	}

	zassert_equal(atomic_get(&mock_errors), 0, "mock protocol violation: 0x%x",
		      (unsigned int)atomic_get(&mock_errors));

	for (int i = 0; i < POOL_DEPTH; i++) {
		zassert_equal(submit_result[i], 0, "submit failed after completion");
	}

	zassert_false(bad, "status descriptors are not per-request");
}

ZTEST(virtio_blk_pool, test_capacity_limited_no_deadlock)
{
	bool joined_all = true;
	int done;

	reset_pool();
	budget_enabled = true;
	k_sem_init(&desc_budget, 0, DESC_BUDGET_LIMIT);
	for (int i = 0; i < DESC_BUDGET; i++) {
		(void)k_sem_give(&desc_budget);
	}

	start_submitters();

	/*
	 * Complete each request as soon as it is published and return its
	 * descriptors. If two chains interleave, neither finishes and nothing
	 * else is published, so the wait below times out.
	 *
	 * The budget holds less than two complete chains, so at most one chain
	 * is fully acquired at a time and the fake allocates the record index in
	 * publication order. That is why the loop counter indexes recorded[].
	 */
	for (done = 0; done < POOL_DEPTH; done++) {
		if (k_sem_take(&chain_published, K_SECONDS(1)) != 0) {
			break;
		}
		complete_recorded(done);
	}

	if (done != POOL_DEPTH) {
		atomic_set(&budget_aborting, 1);
		for (int i = 0; i < DESC_BUDGET_LIMIT; i++) {
			(void)k_sem_give(&desc_budget);
		}
	}

	for (int i = 0; i < POOL_DEPTH; i++) {
		if (k_thread_join(&submit_threads[i], K_SECONDS(1)) != 0) {
			joined_all = false;
		}
	}

	budget_enabled = false;

	zassert_true(joined_all, "a submitter did not exit");
	zassert_equal(done, POOL_DEPTH, "deadlock: %d of %d requests reached the virtqueue", done,
		      POOL_DEPTH);
	zassert_equal(atomic_get(&mock_errors), 0, "mock protocol violation: 0x%x",
		      (unsigned int)atomic_get(&mock_errors));

	for (int i = 0; i < POOL_DEPTH; i++) {
		zassert_equal(submit_result[i], 0, "submit %d failed", i);
	}
}

ZTEST(virtio_blk_pool, test_slot_reuse)
{
	int ret;

	reset_pool();
	reuse_mode = true;

	k_thread_create(&submit_threads[0], submit_stacks[0],
			K_THREAD_STACK_SIZEOF(submit_stacks[0]), reuse_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/*
	 * More submissions than pool slots, one at a time. Each completion must
	 * return the slot, otherwise the submitter blocks in its next
	 * virtio_blk_request_get() and no further chain is published.
	 */
	for (int i = 0; i < REUSE_ITERATIONS; i++) {
		ret = k_sem_take(&chain_published, K_SECONDS(1));
		if (ret != 0) {
			k_thread_abort(&submit_threads[0]);
			zassert_ok(ret, "request slot %d was not returned", i);
		}
		complete_recorded(i % POOL_DEPTH);
	}

	zassert_ok(k_thread_join(&submit_threads[0], K_SECONDS(1)), "submitter did not exit");
	zassert_equal(submit_result[0], 0, "submit failed");
	zassert_equal(atomic_get(&mock_errors), 0, "mock protocol violation: 0x%x",
		      (unsigned int)atomic_get(&mock_errors));
}

ZTEST_SUITE(virtio_blk_pool, NULL, NULL, NULL, NULL, NULL);
