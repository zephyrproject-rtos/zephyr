/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Regression coverage for the used ring completion path: duplicate chain
 * returns, a batch of two valid completions, and a self-referential chain.
 *
 * The ISR must reject a used ring entry that does not name a chain
 * submitted and still awaiting completion: a repeating device would
 * otherwise push the same descriptors onto the free stack twice, so two
 * later chains share buffers, and the free descriptor counter grows past
 * the queue size while k_stack_push() drops the pushes.
 *
 * The vhost-user-fs device is only used to bring the request virtqueue up
 * and quiesce it; the completions are written by the test itself and
 * processed by calling virtio_isr() directly, so the test is
 * deterministic. The queue state is restored before returning so the other
 * cases in the suite are unaffected.
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "virtio_common.h"

#define VIRTIO_DEV DEVICE_DT_GET(DT_NODELABEL(virtio_pci))

/* descriptors in the fake in-flight chain whose id gets re-returned */
#define FAKE_CHAIN_LEN 2U

/* extra descriptors held out of the stack, one as a never-submitted id */
#define SPARE_DESC_LEN 2U

/* duplicate completions injected after the legitimate one */
#define DUPLICATE_RETURNS 3U

/* independent chains completed in a single used ring update */
#define BATCH_CHAIN_COUNT 2U

struct requeue_test_context {
	struct virtq *vq;
	uint16_t chain_head;
	uint16_t calls;
};

static struct virtq *find_request_virtqueue(const struct device *dev, uint16_t *queue_count)
{
	struct virtq *request_vq = NULL;
	uint16_t matches = 0U;
	uint16_t i = 0U;

	while (true) {
		struct virtq *vq = virtio_get_virtqueue(dev, i);

		if (vq == NULL) {
			break;
		}
		/* the request queue is the only one mount traffic touched */
		if (sys_le16_to_cpu(vq->avail->idx) != 0) {
			request_vq = vq;
			matches++;
		}
		i++;
	}

	*queue_count = i;
	/* an ambiguous request queue would make the test assert the wrong one */
	return matches == 1U ? request_vq : NULL;
}

static void inject_used_entry(struct virtq *vq, uint32_t chain_head)
{
	struct virtq_used_elem *slot;
	uint16_t pos = sys_le16_to_cpu(vq->used->idx) % vq->num;

	slot = &vq->used->ring[pos];
	slot->id = sys_cpu_to_le32(chain_head);
	slot->len = 0U;
	vq->used->idx = sys_cpu_to_le16(sys_le16_to_cpu(vq->used->idx) + 1U);
}

static void requeue_test_cb(void *opaque, uint32_t used_len)
{
	struct requeue_test_context *ctx = opaque;

	ARG_UNUSED(used_len);
	ctx->calls++;
	atomic_set_bit(ctx->vq->in_flight, ctx->chain_head);
}

struct completion_test_context {
	uint16_t calls;
	uint16_t order;
};

static uint16_t completion_order;

static void completion_test_cb(void *opaque, uint32_t used_len)
{
	struct completion_test_context *ctx = opaque;

	ARG_UNUSED(used_len);
	ctx->calls++;
	ctx->order = completion_order++;
}

ZTEST(virtiofs, test_used_ring_duplicate_chain_return)
{
	const struct device *dev = VIRTIO_DEV;
	uint16_t chain_desc[FAKE_CHAIN_LEN];
	uint16_t spare_desc[SPARE_DESC_LEN];
	struct virtq *vq;
	stack_data_t *slots;
	uint8_t *pushed;
	uint16_t queue_count;
	uint16_t base_free_n;
	uint16_t saved_used_idx;
	uint16_t chain_head;
	uint16_t popped;
	uint16_t head_count = 0U;
	uint16_t second_count = 0U;
	struct virtq_receive_callback_entry saved_cbe;
	struct requeue_test_context requeue_ctx;
	bool high_id_rejected;
	bool never_submitted_rejected;
	bool batch_duplicate_rejected;
	bool separate_duplicates_rejected;
	bool setup_count_valid;
	bool stack_valid;
	bool restore_valid = true;
	int ret;

	vq = find_request_virtqueue(dev, &queue_count);
	zassert_not_null(vq, "expected exactly one active request virtqueue");
	zassert_not_equal(queue_count, 0U, "device reports no virtqueues");

	/* the mount traffic of the suite setup must be fully processed */
	for (uint16_t i = 0U; i < 1000U && vq->last_used_idx != sys_le16_to_cpu(vq->used->idx);
	     i++) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(vq->last_used_idx, sys_le16_to_cpu(vq->used->idx), "used ring not quiesced");

	/* a quiesced queue has every descriptor on the free stack */
	base_free_n = vq->free_desc_n;
	zassert_equal(base_free_n, vq->num, "unexpected free descriptor count %u/%u",
		      (unsigned int)base_free_n, (unsigned int)vq->num);

	/* the device has its own shadow of used->idx; the fake entries below
	 * are invisible to it, so the guest side has to be rewound to this
	 * value at the end to stay aligned with the next real completion
	 */
	saved_used_idx = sys_le16_to_cpu(vq->used->idx);
	slots = k_malloc(vq->num * sizeof(stack_data_t));
	zassert_not_null(slots, "out of memory for free stack dump");
	pushed = k_malloc(vq->num);
	if (pushed == NULL) {
		k_free(slots);
		zassert_not_null(pushed, "out of memory for push back mask");
	}

	/*
	 * Wire a well-formed two descriptor chain the way a submission would
	 * leave it, and hold two extra descriptors out of the stack: one
	 * provides headroom, the other is never submitted and its id is used
	 * to probe the never-submitted path below.
	 */
	uint16_t chain_acquired = 0U;
	uint16_t spare_acquired = 0U;
	bool setup_ok = true;

	for (uint16_t i = 0U; i < FAKE_CHAIN_LEN; i++) {
		ret = virtq_get_free_desc(vq, &chain_desc[i], K_NO_WAIT);
		if (ret != 0) {
			setup_ok = false;
			break;
		}
		chain_acquired++;
	}
	for (uint16_t i = 0U; setup_ok && i < SPARE_DESC_LEN; i++) {
		ret = virtq_get_free_desc(vq, &spare_desc[i], K_NO_WAIT);
		if (ret != 0) {
			setup_ok = false;
			break;
		}
		spare_acquired++;
	}
	if (!setup_ok) {
		for (uint16_t i = 0U; i < chain_acquired; i++) {
			virtq_add_free_desc(vq, chain_desc[i]);
		}
		for (uint16_t i = 0U; i < spare_acquired; i++) {
			virtq_add_free_desc(vq, spare_desc[i]);
		}
		k_free(slots);
		k_free(pushed);
		zassert_true(setup_ok, "unable to allocate descriptors for the test");
		return;
	}
	setup_count_valid = vq->free_desc_n == vq->num - FAKE_CHAIN_LEN - SPARE_DESC_LEN;

	chain_head = chain_desc[0];
	vq->desc[chain_head].next = sys_cpu_to_le16(chain_desc[1]);
	vq->desc[chain_head].flags = sys_cpu_to_le16(VIRTQ_DESC_F_NEXT);
	vq->desc[chain_desc[1]].next = 0U;
	vq->desc[chain_desc[1]].flags = 0U;
	atomic_set_bit(vq->in_flight, chain_head);

	saved_cbe = vq->recv_cbs[chain_head];
	requeue_ctx.vq = vq;
	requeue_ctx.chain_head = chain_head;
	requeue_ctx.calls = 0U;
	vq->recv_cbs[chain_head].cb = requeue_test_cb;
	vq->recv_cbs[chain_head].opaque = &requeue_ctx;

	/* A used-ring ID with high bits set must not alias a valid descriptor. */
	inject_used_entry(vq, ((uint32_t)1U << 16U) | chain_head);
	virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);
	high_id_rejected = vq->free_desc_n == (vq->num - FAKE_CHAIN_LEN - SPARE_DESC_LEN) &&
			   requeue_ctx.calls == 0U && atomic_test_bit(vq->in_flight, chain_head);

	/* a used ring entry for an id that was never submitted is skipped */
	inject_used_entry(vq, spare_desc[0]);
	virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);
	never_submitted_rejected = vq->free_desc_n == (vq->num - FAKE_CHAIN_LEN - SPARE_DESC_LEN) &&
				   requeue_ctx.calls == 0U;

	/* A same-batch duplicate must not complete the callback's requeue. */
	inject_used_entry(vq, chain_head);
	inject_used_entry(vq, chain_head);
	virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);
	batch_duplicate_rejected =
		vq->free_desc_n == vq->num - SPARE_DESC_LEN && requeue_ctx.calls == 1U;

	/* The callback simulates a requeue by marking the same head in flight. */
	atomic_clear_bit(vq->in_flight, chain_head);

	/* Later duplicates must leave the free stack and counter unchanged. */
	for (uint16_t i = 0U; i < DUPLICATE_RETURNS; i++) {
		inject_used_entry(vq, chain_head);
		virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);
	}
	separate_duplicates_rejected = vq->free_desc_n == vq->num - SPARE_DESC_LEN &&
				       requeue_ctx.calls == 1U &&
				       vq->last_used_idx == sys_le16_to_cpu(vq->used->idx);

	/* Check every descriptor, then rebuild the queue before reporting failures. */
	popped = 0U;
	while (popped < vq->num) {
		ret = k_stack_pop(&vq->free_desc_stack, &slots[popped], K_NO_WAIT);
		if (ret != 0) {
			break;
		}
		popped++;
	}
	stack_valid = popped == vq->num - SPARE_DESC_LEN;
	memset(pushed, 0, vq->num);

	for (uint16_t i = 0U; i < popped; i++) {
		uint16_t d = (uint16_t)slots[i];

		if (d >= vq->num || pushed[d] != 0U) {
			stack_valid = false;
			continue;
		}
		pushed[d] = 1U;
		if (d == chain_head) {
			head_count++;
		}
		if (d == chain_desc[1]) {
			second_count++;
		}
	}
	for (uint16_t i = 0U; i < vq->num; i++) {
		bool expected_free = i != spare_desc[0] && i != spare_desc[1];

		if ((pushed[i] != 0U) != expected_free) {
			stack_valid = false;
		}
	}

	for (uint16_t i = 0U; i < vq->num; i++) {
		ret = k_stack_push(&vq->free_desc_stack, (stack_data_t)i);
		if (ret != 0) {
			restore_valid = false;
		}
	}
	vq->free_desc_n = base_free_n;
	atomic_clear_bit(vq->in_flight, chain_head);
	vq->recv_cbs[chain_head] = saved_cbe;

	/*
	 * Rewind the used ring bookkeeping to the device's own shadow: the
	 * fake entries stay in the ring but the next real completion lands on
	 * the first injected slot and overwrites it.
	 */
	vq->used->idx = sys_cpu_to_le16(saved_used_idx);
	vq->last_used_idx = saved_used_idx;

	k_free(slots);
	k_free(pushed);

	zassert_true(setup_count_valid, "free count after fake submission mismatch");
	zassert_true(high_id_rejected, "used ring ID with high bits was accepted");
	zassert_true(never_submitted_rejected, "never-submitted ID was accepted");
	zassert_true(batch_duplicate_rejected,
		     "same-batch duplicate was accepted after its callback requeued the head");
	zassert_true(separate_duplicates_rejected,
		     "later duplicates changed queue state or invoked the callback");
	zassert_true(stack_valid, "free stack contains missing or duplicate descriptors");
	zassert_equal(head_count, 1U, "chain head returned %u times", (unsigned int)head_count);
	zassert_equal(second_count, 1U, "chain tail returned %u times", (unsigned int)second_count);
	zassert_true(restore_valid, "failed to restore free descriptor stack");
}

ZTEST(virtiofs, test_used_ring_batch_of_two_completions)
{
	const struct device *dev = VIRTIO_DEV;
	struct virtq *vq;
	stack_data_t slot;
	uint16_t queue_count;
	uint16_t desc[BATCH_CHAIN_COUNT * 2U];
	uint16_t head[BATCH_CHAIN_COUNT];
	struct virtq_receive_callback_entry saved_cbe[BATCH_CHAIN_COUNT];
	struct completion_test_context ctx[BATCH_CHAIN_COUNT];
	uint16_t base_free_n;
	uint16_t saved_used_idx;
	uint16_t acquired = 0U;
	bool callbacks_ok = true;
	bool completion_ok;
	int ret;

	vq = find_request_virtqueue(dev, &queue_count);
	zassert_not_null(vq, "expected exactly one active request virtqueue");
	zassert_not_equal(queue_count, 0U, "device reports no virtqueues");

	for (uint16_t i = 0U; i < 1000U && vq->last_used_idx != sys_le16_to_cpu(vq->used->idx);
	     i++) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(vq->last_used_idx, sys_le16_to_cpu(vq->used->idx), "used ring not quiesced");

	base_free_n = vq->free_desc_n;
	zassert_equal(base_free_n, vq->num, "unexpected free descriptor count %u/%u",
		      (unsigned int)base_free_n, (unsigned int)vq->num);
	saved_used_idx = sys_le16_to_cpu(vq->used->idx);
	completion_order = 0U;

	for (uint16_t i = 0U; i < BATCH_CHAIN_COUNT * 2U; i++) {
		ret = virtq_get_free_desc(vq, &desc[i], K_NO_WAIT);
		if (ret != 0) {
			break;
		}
		acquired++;
	}
	if (acquired != BATCH_CHAIN_COUNT * 2U) {
		for (uint16_t i = 0U; i < acquired; i++) {
			virtq_add_free_desc(vq, desc[i]);
		}
		zassert_unreachable("not enough free descriptors for the batch test");
	}

	for (uint16_t i = 0U; i < BATCH_CHAIN_COUNT; i++) {
		uint16_t h = desc[i * 2U];
		uint16_t t = desc[i * 2U + 1U];

		head[i] = h;
		vq->desc[h].next = sys_cpu_to_le16(t);
		vq->desc[h].flags = sys_cpu_to_le16(VIRTQ_DESC_F_NEXT);
		vq->desc[t].next = 0U;
		vq->desc[t].flags = 0U;
		atomic_set_bit(vq->in_flight, h);

		saved_cbe[i] = vq->recv_cbs[h];
		ctx[i].calls = 0U;
		ctx[i].order = UINT16_MAX;
		vq->recv_cbs[h].cb = completion_test_cb;
		vq->recv_cbs[h].opaque = &ctx[i];
	}

	for (uint16_t i = 0U; i < BATCH_CHAIN_COUNT; i++) {
		inject_used_entry(vq, head[i]);
	}
	virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);

	for (uint16_t i = 0U; i < BATCH_CHAIN_COUNT; i++) {
		if (ctx[i].calls != 1U || ctx[i].order != i) {
			callbacks_ok = false;
		}
	}
	completion_ok = vq->free_desc_n == base_free_n &&
			vq->last_used_idx == (uint16_t)(saved_used_idx + BATCH_CHAIN_COUNT);

	/* rebuild the queue before reporting failures */
	while (k_stack_pop(&vq->free_desc_stack, &slot, K_NO_WAIT) == 0) {
	}
	for (uint16_t i = 0U; i < vq->num; i++) {
		(void)k_stack_push(&vq->free_desc_stack, (stack_data_t)i);
	}
	vq->free_desc_n = base_free_n;
	for (uint16_t i = 0U; i < BATCH_CHAIN_COUNT; i++) {
		atomic_clear_bit(vq->in_flight, head[i]);
		vq->recv_cbs[head[i]] = saved_cbe[i];
	}
	vq->used->idx = sys_cpu_to_le16(saved_used_idx);
	vq->last_used_idx = saved_used_idx;

	zassert_true(callbacks_ok, "batch callbacks were not invoked once each in order");
	zassert_true(completion_ok, "batch completion changed the free count or used index");
}

ZTEST(virtiofs, test_used_ring_walk_is_bounded)
{
	const struct device *dev = VIRTIO_DEV;
	struct virtq *vq;
	stack_data_t slot;
	uint16_t queue_count;
	uint16_t head;
	uint16_t base_free_n;
	uint16_t saved_used_idx;
	struct virtq_receive_callback_entry saved_cbe;
	struct completion_test_context ctx;
	bool completed;
	int ret;

	vq = find_request_virtqueue(dev, &queue_count);
	zassert_not_null(vq, "expected exactly one active request virtqueue");
	zassert_not_equal(queue_count, 0U, "device reports no virtqueues");

	for (uint16_t i = 0U; i < 1000U && vq->last_used_idx != sys_le16_to_cpu(vq->used->idx);
	     i++) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(vq->last_used_idx, sys_le16_to_cpu(vq->used->idx), "used ring not quiesced");

	base_free_n = vq->free_desc_n;
	zassert_equal(base_free_n, vq->num, "unexpected free descriptor count %u/%u",
		      (unsigned int)base_free_n, (unsigned int)vq->num);
	saved_used_idx = sys_le16_to_cpu(vq->used->idx);

	ret = virtq_get_free_desc(vq, &head, K_NO_WAIT);
	zassert_equal(ret, 0, "no free descriptor for the walk test");

	/* a chain that points at itself must not trap the descriptor walk */
	vq->desc[head].next = sys_cpu_to_le16(head);
	vq->desc[head].flags = sys_cpu_to_le16(VIRTQ_DESC_F_NEXT);
	atomic_set_bit(vq->in_flight, head);

	saved_cbe = vq->recv_cbs[head];
	ctx.calls = 0U;
	ctx.order = UINT16_MAX;
	vq->recv_cbs[head].cb = completion_test_cb;
	vq->recv_cbs[head].opaque = &ctx;

	inject_used_entry(vq, head);
	virtio_isr(dev, VIRTIO_QUEUE_INTERRUPT, queue_count);

	completed = ctx.calls == 1U && vq->last_used_idx == (uint16_t)(saved_used_idx + 1U) &&
		    vq->free_desc_n == base_free_n;

	while (k_stack_pop(&vq->free_desc_stack, &slot, K_NO_WAIT) == 0) {
	}
	for (uint16_t i = 0U; i < vq->num; i++) {
		(void)k_stack_push(&vq->free_desc_stack, (stack_data_t)i);
	}
	vq->free_desc_n = base_free_n;
	atomic_clear_bit(vq->in_flight, head);
	vq->recv_cbs[head] = saved_cbe;
	vq->used->idx = sys_cpu_to_le16(saved_used_idx);
	vq->last_used_idx = saved_used_idx;

	zassert_true(completed, "descriptor walk was not bounded or the completion was lost");
}
