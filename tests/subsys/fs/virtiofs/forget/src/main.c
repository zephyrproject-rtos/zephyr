/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * White-box test for the asynchronous FORGET slot pool. The production source
 * is included directly so the test can control submission and completion
 * without a real virtio transport: the mocked virtq_add_buffer_chain() records
 * the request buffers and the test invokes the registered callback when it
 * wants a completion to happen.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/ztest.h>

#if CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS > 0

static struct {
	struct {
		struct virtq_buf buf;
		virtq_receive_callback cb;
		void *opaque;
	} entries[CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS];
	uint32_t count;
	uint32_t submissions;
	uint32_t notifications;
	int enqueue_error;
} submit;

#include "virtiofs.c"

int virtq_add_buffer_chain(struct virtq *vq, struct virtq_buf *bufs, uint16_t count,
			   uint16_t readable, virtq_receive_callback cb, void *opaque,
			   k_timeout_t timeout)
{
	ARG_UNUSED(vq);
	ARG_UNUSED(readable);
	ARG_UNUSED(timeout);

	submit.submissions++;
	if (submit.enqueue_error != 0) {
		return submit.enqueue_error;
	}

	zassert_equal(count, 1U, "unexpected descriptor count");
	zassert_true(submit.count < ARRAY_SIZE(submit.entries), "too many submissions");

	submit.entries[submit.count].buf = bufs[0];
	submit.entries[submit.count].cb = cb;
	submit.entries[submit.count].opaque = opaque;
	submit.count++;

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

	submit.notifications++;
}

static DEVICE_API(virtio, test_api) = {
	.get_virtqueue = get_virtqueue,
	.notify_virtqueue = notify_virtqueue,
};

static void complete_submission(uint32_t index)
{
	zassert_true(index < submit.count, "no submission to complete");

	submit.entries[index].cb(submit.entries[index].opaque, 0);

	for (uint32_t i = index + 1; i < submit.count; i++) {
		submit.entries[i - 1] = submit.entries[i];
	}
	submit.count--;
}

/* A slot must not be reused before its completion, and must be reused after. */
ZTEST(virtiofs_forget, test_slot_lifecycle)
{
	const struct device dev = {.api = &test_api};
	const uint32_t slots = CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS;
	uintptr_t first_addr;

	zassert_true(slots >= 2U, "test needs at least two slots");
	virtiofs_forget_pool_init();
	zassert_equal(submit.count, 0U);

	for (uint32_t i = 0; i < slots; i++) {
		zassert_true(virtiofs_forget_async(&dev, 2U + i, 1U), "submit %u failed", i);
	}

	/* All slots are in flight: no completion has happened yet. */
	zassert_false(virtiofs_forget_async(&dev, 100U, 1U), "pool should be exhausted");

	first_addr = POINTER_TO_UINT(submit.entries[0].buf.addr);
	complete_submission(0);
	zassert_true(virtiofs_forget_async(&dev, 101U, 1U), "resubmit failed");
	zassert_equal(POINTER_TO_UINT(submit.entries[submit.count - 1].buf.addr), first_addr,
		      "slot was not reused after completion");

	while (submit.count > 0U) {
		complete_submission(0);
	}
}

/* A failed enqueue must return the slot it took. */
ZTEST(virtiofs_forget, test_failed_enqueue_returns_slot)
{
	const struct device dev = {.api = &test_api};
	const uint32_t slots = CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS;

	virtiofs_forget_pool_init();
	zassert_equal(submit.count, 0U);

	submit.enqueue_error = -EBUSY;
	zassert_false(virtiofs_forget_async(&dev, 2U, 1U), "enqueue error should fail");
	submit.enqueue_error = 0;

	/* If the failed submission leaked its slot, the pool would run dry early. */
	for (uint32_t i = 0; i < slots; i++) {
		zassert_true(virtiofs_forget_async(&dev, 3U + i, 1U),
			     "failed submit leaked a slot (success %u/%u)", i, slots);
	}
	zassert_false(virtiofs_forget_async(&dev, 100U, 1U), "pool should be exhausted");

	while (submit.count > 0U) {
		complete_submission(0);
	}
}

ZTEST_SUITE(virtiofs_forget, NULL, NULL, NULL, NULL, NULL);

#else /* CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS > 0 */

ZTEST(virtiofs_forget, test_async_forget_disabled)
{
	ztest_test_skip();
}

ZTEST_SUITE(virtiofs_forget, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_VIRTIOFS_ASYNC_FORGET_BUFFERS > 0 */
