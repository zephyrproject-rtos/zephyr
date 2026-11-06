/*
 * Copyright (c) 2024-2026 Antmicro <www.antmicro.com>
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include "virtio_common.h"

LOG_MODULE_REGISTER(virtio, CONFIG_VIRTIO_LOG_LEVEL);

/*
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

/* According to the spec 2.7.5.2 the maximum size of descriptor chain is 4GB */
#define MAX_DESCRIPTOR_CHAIN_LENGTH BIT64(32)

BUILD_ASSERT((CONFIG_VIRTIO_WAKE_QUEUE_PRIORITY >= -CONFIG_NUM_COOP_PRIORITIES) &&
		     (CONFIG_VIRTIO_WAKE_QUEUE_PRIORITY < CONFIG_NUM_PREEMPT_PRIORITIES),
	     "CONFIG_VIRTIO_WAKE_QUEUE_PRIORITY is not a valid non-idle priority");

/*
 * Shared workqueue running the per-virtqueue wake handlers. It is deliberately
 * not the system workqueue: a handler blocked there must not delay descriptor
 * notifications. Started lazily by virtq_create(), serialized by
 * virtio_wake_queue_lock, and never stopped, drained or plugged.
 */
static struct k_work_q virtio_wake_queue;
static K_KERNEL_STACK_DEFINE(virtio_wake_stack, CONFIG_VIRTIO_WAKE_QUEUE_STACK_SIZE);
static K_MUTEX_DEFINE(virtio_wake_queue_lock);
static bool virtio_wake_queue_started;

static void virtq_wake_handler(struct k_work *work)
{
	struct virtq *v = CONTAINER_OF(work, struct virtq, wake_work);

	k_mutex_lock(&v->wait_lock, K_FOREVER);
	k_condvar_broadcast(&v->wait_cond);
	k_mutex_unlock(&v->wait_lock);
}

static void virtio_wake_queue_start_once(void)
{
	k_mutex_lock(&virtio_wake_queue_lock, K_FOREVER);

	if (!virtio_wake_queue_started) {
		k_work_queue_init(&virtio_wake_queue);
		k_work_queue_start(&virtio_wake_queue, virtio_wake_stack,
				   K_KERNEL_STACK_SIZEOF(virtio_wake_stack),
				   CONFIG_VIRTIO_WAKE_QUEUE_PRIORITY, NULL);
		/* Published only once the worker can run work items. */
		virtio_wake_queue_started = true;
	}

	k_mutex_unlock(&virtio_wake_queue_lock);
}

static void virtq_submit_wake(struct virtq *v)
{
	int ret = k_work_submit_to_queue(&virtio_wake_queue, &v->wake_work);

	if (ret < 0) {
		/*
		 * Submitting is part of the lifecycle contract: 0, 1 and 2 mean
		 * the notification is pending, anything else means a waiter may
		 * sleep forever. There is no fallback wake protocol.
		 */
		LOG_ERR("could not submit virtqueue wake work (%d)", ret);
		__ASSERT(false, "virtqueue wake work submission failed (%d)", ret);
	}
}

/*
 * The free descriptor count, the stack depth and the queue size disagree, so
 * the accounting the allocation path relies on is broken. Assertions report
 * it; without them there is no safe way to continue.
 */
static void virtq_fatal_accounting(struct virtq *v)
{
	LOG_ERR("virtqueue free descriptor accounting is corrupted (size %u)", v->num);
	__ASSERT(false, "virtqueue free descriptor accounting is corrupted");

	if (!IS_ENABLED(CONFIG_ASSERT)) {
		k_panic();
	}
}

/* Caller holds v->lock. */
static bool virtq_chain_length(struct virtq *v, uint16_t head, uint16_t *length)
{
	uint16_t curr = head;

	for (uint16_t i = 0; i < v->num; i++) {
		if (curr >= v->num) {
			return false;
		}

		if ((sys_le16_to_cpu(v->desc[curr].flags) & VIRTQ_DESC_F_NEXT) == 0U) {
			*length = i + 1U;
			return true;
		}

		curr = sys_le16_to_cpu(v->desc[curr].next);
	}

	return false;
}

/*
 * Caller holds v->lock. The chain has been validated, so every index is in
 * range and the chain terminates after count descriptors. Each next link is
 * read before its descriptor becomes reusable.
 */
static void virtq_push_chain(struct virtq *v, uint16_t head, uint16_t count)
{
	uint16_t curr = head;

	for (uint16_t i = 0; i < count; i++) {
		uint16_t next = sys_le16_to_cpu(v->desc[curr].next);

		if (k_stack_push(&v->free_desc_stack, curr) != 0) {
			virtq_fatal_accounting(v);
		}

		curr = next;
	}
}

/*
 * Caller holds v->lock. Publishes a validated batch, updates the count and
 * reports whether a wakeup has to be submitted after the lock is released.
 */
static bool virtq_publish_locked(struct virtq *v, uint16_t head, uint16_t count)
{
	virtq_push_chain(v, head, count);
	atomic_add(&v->free_desc_n, count);
	__ASSERT_NO_MSG((uint32_t)atomic_get(&v->free_desc_n) <= v->num);

	/* Read after the descriptors are back on the stack. */
	return atomic_get(&v->waiters) > 0;
}

/*
 * Caller holds v->lock. The free descriptor count is the only authority on
 * availability: a count large enough for the whole chain guarantees that the
 * following non-blocking pops succeed.
 */
static int virtq_try_alloc_desc_chain_locked(struct virtq *v, uint16_t count, uint16_t *head)
{
	uint16_t first = 0;
	uint16_t prev = 0;

	if ((uint32_t)atomic_get(&v->free_desc_n) < count) {
		return -EBUSY;
	}

	for (uint16_t i = 0; i < count; i++) {
		stack_data_t desc;

		if (k_stack_pop(&v->free_desc_stack, &desc, K_NO_WAIT) != 0) {
			/*
			 * The count promised enough descriptors, so a failed pop
			 * is broken bookkeeping, not a retryable shortage. Put
			 * back whatever was taken so the queue is not left with
			 * descriptors lost in the middle of a chain.
			 */
			if (i > 0U) {
				virtq_push_chain(v, first, i);
			}

			virtq_fatal_accounting(v);

			return -EIO;
		}

		if (i == 0U) {
			first = (uint16_t)desc;
		} else {
			/* Linked temporarily; overwritten when the chain is built. */
			v->desc[prev].next = sys_cpu_to_le16((uint16_t)desc);
		}

		prev = (uint16_t)desc;
	}

	atomic_sub(&v->free_desc_n, count);
	__ASSERT_NO_MSG((uint32_t)atomic_get(&v->free_desc_n) <= v->num);
	*head = first;

	return 0;
}

static int virtq_try_alloc_desc_chain(struct virtq *v, uint16_t count, uint16_t *head)
{
	k_spinlock_key_t key = k_spin_lock(&v->lock);
	int ret = virtq_try_alloc_desc_chain_locked(v, count, head);

	k_spin_unlock(&v->lock, key);

	return ret;
}

/* Called with v->wait_lock held; returns with it held. */
static int virtq_wait_for_desc_chain_locked(struct virtq *v, uint16_t count, uint16_t *head,
					    k_timepoint_t deadline)
{
	for (;;) {
		k_timeout_t remaining = sys_timepoint_timeout(deadline);

		if (K_TIMEOUT_EQ(remaining, K_NO_WAIT)) {
			return -EAGAIN;
		}

		if (virtq_try_alloc_desc_chain(v, count, head) == 0) {
			return 0;
		}

		/*
		 * The attempt may have crossed the deadline while it waited for
		 * the queue lock or was preempted; never block with a budget
		 * measured before it.
		 */
		remaining = sys_timepoint_timeout(deadline);
		if (K_TIMEOUT_EQ(remaining, K_NO_WAIT)) {
			return -EAGAIN;
		}

		/*
		 * k_condvar_wait() releases the mutex atomically with pending
		 * this thread, so a wake handler cannot miss the window. It
		 * re-acquires the mutex with K_FOREVER, which is why a timeout
		 * can only promise "no later than deadline plus one mutex
		 * acquisition plus scheduling latency".
		 */
		if (k_condvar_wait(&v->wait_cond, &v->wait_lock, remaining) != 0) {
			return -EAGAIN;
		}
	}
}

static int virtq_wait_for_desc_chain(struct virtq *v, uint16_t count, uint16_t *head,
				     k_timepoint_t deadline)
{
	k_timeout_t remaining = sys_timepoint_timeout(deadline);
	int ret;

	/*
	 * Register before the allocation attempt whose failure can precede the
	 * first sleep: a release pushes descriptors before it reads this
	 * counter, so either the attempt below sees them or the matching wake
	 * is guaranteed.
	 */
	atomic_inc(&v->waiters);

	if (K_TIMEOUT_EQ(remaining, K_NO_WAIT)) {
		ret = -EAGAIN;
	} else {
		ret = k_mutex_lock(&v->wait_lock, remaining);

		/* No caller of the thread blocking API may observe a mutex code. */
		if (ret != 0) {
			if (ret != -EAGAIN) {
				LOG_ERR("unexpected mutex error %d while allocating", ret);
			}
			ret = -EAGAIN;
		} else {
			ret = virtq_wait_for_desc_chain_locked(v, count, head, deadline);
			k_mutex_unlock(&v->wait_lock);
		}
	}

	atomic_dec(&v->waiters);

	return ret;
}

static int virtq_alloc_desc_chain(struct virtq *v, uint16_t count, uint16_t *head, bool may_wait,
				  k_timepoint_t deadline)
{
	/* A descriptor chain can never be larger than the queue itself. */
	if (count > v->num) {
		return -EINVAL;
	}

	/*
	 * Interrupt context never waits, regardless of the requested timeout,
	 * and neither does a caller that asked not to wait. The same -EBUSY is
	 * what the legacy blocking pop returned in those cases. These callers
	 * never block, so they do not pay for reading the clock either.
	 */
	if (!may_wait) {
		return virtq_try_alloc_desc_chain(v, count, head) == 0 ? 0 : -EBUSY;
	}

	if (virtq_try_alloc_desc_chain(v, count, head) == 0) {
		return 0;
	}

	return virtq_wait_for_desc_chain(v, count, head, deadline);
}

static bool virtq_release_allowed(struct virtq *v, uint16_t count)
{
	uint32_t free_n = (uint32_t)atomic_get(&v->free_desc_n);

	return free_n <= v->num && count <= (uint32_t)v->num - free_n;
}

int virtq_release_chain(struct virtq *v, uint16_t head)
{
	const char *rejected = NULL;
	uint16_t count;
	bool wake = false;
	k_spinlock_key_t key;

	if (head >= v->num) {
		LOG_ERR("invalid descriptor chain head %u (queue size %u)", head, v->num);
		return -EINVAL;
	}

	key = k_spin_lock(&v->lock);

	/* Validate the whole chain before publishing any part of it. */
	if (!virtq_chain_length(v, head, &count)) {
		rejected = "malformed or terminates outside the descriptor table";
	} else if (!virtq_release_allowed(v, count)) {
		rejected = "does not fit the free list";
	} else {
		wake = virtq_publish_locked(v, head, count);
	}

	k_spin_unlock(&v->lock, key);

	if (rejected != NULL) {
		LOG_ERR("descriptor chain at %u rejected: %s (queue size %u)", head, rejected,
			v->num);
		return -EINVAL;
	}

	if (wake) {
		virtq_submit_wake(v);
	}

	return 0;
}

int virtq_create(struct virtq *v, size_t size)
{
	/*
	 * A size of 0 denotes a queue the driver enumerates but does not use
	 * (e.g. the virtiofs high priority queue). Such a queue is set up empty
	 * and is never operated on. Other split virtqueue sizes must be powers
	 * of two no greater than 32 KiB.
	 */
	if (size > KB(32) || (size != 0U && !IS_POWER_OF_TWO(size))) {
		LOG_ERR("invalid virtqueue size %zu", size);
		return -EINVAL;
	}

	/*
	 * Creating a queue starts the shared wake workqueue on first use, which
	 * requires thread context.
	 */
	__ASSERT(!k_is_in_isr(), "virtqueues must be created from thread context");
	if (k_is_in_isr()) {
		return -EINVAL;
	}

	/*
	 * For sizes and alignments see table in spec 2.7. We are supporting only modern virtio, so
	 * we don't have to adhere to additional constraints from spec 2.7.2
	 */
	size_t descriptor_table_size = 16 * size;
	size_t available_ring_size = 2 * size + 6;
	size_t used_ring_pad = (descriptor_table_size + available_ring_size) % 4;
	size_t used_ring_size = 8 * size + 6;
	size_t shared_size =
		descriptor_table_size + available_ring_size + used_ring_pad + used_ring_size;
	size_t recv_cbs_pad = WB_UP(shared_size) - shared_size;
	size_t recv_cbs_size = recv_cbs_pad + sizeof(struct virtq_receive_callback_entry) * size;
	size_t v_size = shared_size + recv_cbs_size + size * sizeof(stack_data_t);
	size_t wake_sync_offset = ROUND_UP(v_size, __alignof__(struct k_work_sync));

	/* Keep flush synchronization in coherent memory, outside the caller's stack. */
	v_size = wake_sync_offset + sizeof(struct k_work_sync);

	uint8_t *v_area = k_aligned_alloc(16, v_size);

	if (v_area == NULL) {
		LOG_ERR("unable to allocate virtqueue");
		return -ENOMEM;
	}

	memset(v, 0, sizeof(*v));
	v->num = size;
	v->desc = (struct virtq_desc *)v_area;
	v->avail = (struct virtq_avail *)((uint8_t *)v->desc + descriptor_table_size);
	v->used = (struct virtq_used *)((uint8_t *)v->avail + available_ring_size + used_ring_pad);
	v->recv_cbs = (struct virtq_receive_callback_entry *)((uint8_t *)v->used + used_ring_size +
							      recv_cbs_pad);
	v->wake_sync = (struct k_work_sync *)(v_area + wake_sync_offset);

	/*
	 * At the beginning of the descriptor table, the available ring and the used ring have to be
	 * set to zero. It's the case for both PCI (4.1.5.1.3) and MMIO (4.2.3.2) transport options.
	 * Its unspecified for channel I/O (chapter 4.3), but its used on platforms not supported by
	 * Zephyr, so we don't have to handle it here
	 */
	memset(v_area, 0, v_size);

	/* pointer-aligned as recv_cbs starts WB_UP()-aligned and holds pointer pairs */
	stack_data_t *stack_buf = (stack_data_t *)(v_area + shared_size + recv_cbs_size);

	k_stack_init(&v->free_desc_stack, stack_buf, size);
	for (uint16_t i = 0; i < size; i++) {
		k_stack_push(&v->free_desc_stack, i);
	}
	atomic_set(&v->free_desc_n, size);
	atomic_set(&v->waiters, 0);

	k_mutex_init(&v->wait_lock);
	k_condvar_init(&v->wait_cond);
	k_work_init(&v->wake_work, virtq_wake_handler);

	/*
	 * No waiter can exist before the queue is returned to the caller, so
	 * this also guarantees that the wake workqueue is running before the
	 * first notification can be submitted.
	 */
	virtio_wake_queue_start_once();

	return 0;
}

void virtq_free(struct virtq *v)
{
	__ASSERT(!k_is_in_isr(), "virtqueues must be freed from thread context");
	__ASSERT(atomic_get(&v->waiters) == 0, "virtqueue freed with blocked waiters");

	/*
	 * The caller has quiesced all submission and completion sources of this
	 * queue, so only this queue's own notification may be pending. The
	 * shared workqueue is never drained: that would reject other
	 * virtqueues' wakeups.
	 */
	(void)k_work_flush(&v->wake_work, v->wake_sync);

	k_free(v->desc);
}

static void virtq_add_available(struct virtq *v, uint16_t desc_idx)
{
	uint16_t idx = sys_le16_to_cpu(v->avail->idx);

	v->avail->ring[idx & (v->num - 1)] = sys_cpu_to_le16(desc_idx);
	barrier_dmem_fence_full();
	v->avail->idx = sys_cpu_to_le16(idx + 1);
}

int virtq_add_buffer_chain(
	struct virtq *v, struct virtq_buf *bufs, uint16_t bufs_size,
	uint16_t device_readable_count, virtq_receive_callback cb, void *cb_opaque,
	k_timeout_t timeout)
{
	k_timepoint_t deadline = {0};
	bool may_wait = !k_is_in_isr() && !K_TIMEOUT_EQ(timeout, K_NO_WAIT);
	uint64_t total_len = 0;
	uint16_t head;
	int ret;

	/* Charge buffer validation and every allocation attempt to the same budget. */
	if (may_wait) {
		deadline = sys_timepoint_calc(timeout);
	}

	if (bufs_size == 0U) {
		return -EINVAL;
	}

	/* A single chain can never need more descriptors than the queue has. */
	if (bufs_size > v->num) {
		return -EINVAL;
	}

	for (uint16_t i = 0; i < bufs_size; i++) {
		total_len += bufs[i].len;
	}

	if (total_len > MAX_DESCRIPTOR_CHAIN_LENGTH) {
		LOG_ERR("buffer chain is longer than 2^32 bytes");
		return -EINVAL;
	}

	/* All-or-nothing: the whole chain is allocated or nothing is. */
	ret = virtq_alloc_desc_chain(v, bufs_size, &head, may_wait, deadline);
	if (ret != 0) {
		return ret;
	}

	uint16_t desc = head;

	for (uint16_t buf_n = 0; buf_n < bufs_size; buf_n++) {
		/* The allocation linked the chain through next. */
		uint16_t next = sys_le16_to_cpu(v->desc[desc].next);
		uint16_t flags = buf_n < device_readable_count ? 0U : VIRTQ_DESC_F_WRITE;

		if (buf_n < bufs_size - 1U) {
			flags |= VIRTQ_DESC_F_NEXT;
			v->desc[desc].next = sys_cpu_to_le16(next);
		} else {
			v->desc[desc].next = 0U;
		}

		v->desc[desc].addr = sys_cpu_to_le64(k_mem_phys_addr(bufs[buf_n].addr));
		v->desc[desc].len = sys_cpu_to_le32(bufs[buf_n].len);
		v->desc[desc].flags = sys_cpu_to_le16(flags);

		desc = next;
	}

	v->recv_cbs[head].cb = cb;
	v->recv_cbs[head].opaque = cb_opaque;

	k_spinlock_key_t key = k_spin_lock(&v->lock);

	virtq_add_available(v, head);
	k_spin_unlock(&v->lock, key);

	return 0;
}

/*
 * The accepted region is the boot mapping of RAM: the interval in which
 * k_mem_phys_addr() is defined. It is derived from the same definitions the
 * kernel uses, which are visible to drivers, instead of the kernel-private
 * mmu.h.
 */
static bool virtio_buf_region(uintptr_t *lower, uintptr_t *upper)
{
#if defined(CONFIG_MMU) && defined(CONFIG_ARCH_MAPS_ALL_RAM)
	uintptr_t phys_end = (uintptr_t)DT_CHOSEN_SRAM_ADDR;

	/* The whole RAM is mapped at boot, so the window ends with it. */
	if (phys_end > UINTPTR_MAX - (uintptr_t)DT_CHOSEN_SRAM_SIZE) {
		return false;
	}

	phys_end += (uintptr_t)DT_CHOSEN_SRAM_SIZE;
	*lower = (uintptr_t)z_mapped_start;
	*upper = (uintptr_t)K_MEM_VIRT_ADDR(phys_end);

	return *upper > *lower;
#elif defined(CONFIG_MMU)
	/* Only the kernel image is mapped at boot. */
	*lower = (uintptr_t)z_mapped_start;
	*upper = (uintptr_t)z_mapped_end;

	return *upper > *lower;
#elif DT_HAS_CHOSEN(zephyr_sram)
	*lower = (uintptr_t)DT_CHOSEN_SRAM_ADDR;
	*upper = (uintptr_t)DT_CHOSEN_SRAM_ADDR;

	if (*upper > UINTPTR_MAX - (uintptr_t)DT_CHOSEN_SRAM_SIZE) {
		return false;
	}

	*upper += (uintptr_t)DT_CHOSEN_SRAM_SIZE;

	return *upper > *lower;
#else
	ARG_UNUSED(lower);
	ARG_UNUSED(upper);

	/* Without a known RAM region there is nothing the device can address. */
	return false;
#endif
}

bool virtio_buf_is_in_linear_mapping(const void *addr, size_t len)
{
	uintptr_t start = (uintptr_t)addr;
	uintptr_t lower;
	uintptr_t upper;

	if (IS_ENABLED(CONFIG_DEMAND_PAGING)) {
		return false;
	}

	if (addr == NULL) {
		return false;
	}

	if (len > UINTPTR_MAX - start) {
		return false;
	}

	if (!virtio_buf_region(&lower, &upper)) {
		return false;
	}

	if (len == 0U) {
		return start > lower && start < upper;
	}

	return start >= lower && (start + len - 1U) < upper;
}
