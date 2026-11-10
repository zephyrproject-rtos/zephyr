/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the VIRTIO virtqueue API.
 * @ingroup virtqueue_interface
 */

#ifndef ZEPHYR_VIRTIO_VIRTQUEUE_H_
#define ZEPHYR_VIRTIO_VIRTQUEUE_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

/**
 * @brief Virtqueue Interface
 * @defgroup virtqueue_interface Virtqueue Interface
 * @ingroup virtio_interface
 * @{
 */

/**
 * used in virtq_desc::flags, enables chaining descriptor via virtq_desc::next
 */
#define VIRTQ_DESC_F_NEXT 1
/**
 * used in virtq_desc::flags, makes descriptor device writeable
 */
#define VIRTQ_DESC_F_WRITE 2

/**
 * @brief virtqueue descriptor
 *
 * Describes a single buffer
 */
struct virtq_desc {
	/**
	 * physical address of the buffer
	 */
	uint64_t addr;
	/**
	 * length of the buffer
	 */
	uint32_t len;
	/**
	 * buffer flags
	 */
	uint16_t flags;
	/**
	 * chaining next descriptor, valid if flags & VIRTQ_DESC_F_NEXT
	 */
	uint16_t next;
};

/**
 * @brief virtqueue available ring
 *
 * Used to pass descriptors to the virtio device. Driver writeable, device readable
 */
struct virtq_avail {
	/**
	 * ring flags, e.g. VIRTQ_AVAIL_F_NO_INTERRUPT, currently unused
	 */
	uint16_t flags;
	/**
	 * head of the ring, by increasing it newly added descriptors are committed
	 */
	uint16_t idx;
	/**
	 * ring with indexes of descriptors
	 */
	uint16_t ring[];
};

/**
 * @brief used descriptor chain
 *
 * Describes a single descriptor chain returned by the virtio device
 */
struct virtq_used_elem {
	/**
	 * index of the head of descriptor chain
	 */
	uint32_t id;
	/**
	 * total amount of bytes written to descriptor chain by the virtio device
	 */
	uint32_t len;
};

/**
 * @brief virtqueue used ring
 *
 * Used to receive descriptors from the virtio device. Driver readable, device writeable
 */
struct virtq_used {
	/**
	 * ring flags, e.g. VIRTQ_USED_F_NO_NOTIFY, currently unused
	 */
	uint16_t flags;
	/**
	 * head of the ring
	 */
	uint16_t idx;
	/**
	 * ring of struct virtq_used_elem
	 */
	struct virtq_used_elem ring[];
};

/**
 * @brief receive callback function type
 *
 * @param opaque argument passed to the callback
 * @param used_len total amount of bytes written to the descriptor chain by the virtio device
 */
typedef void (*virtq_receive_callback)(void *opaque, uint32_t used_len);

/**
 * @brief callback descriptor
 *
 * contains callback function ad its argument, invoked after virtio device return
 * descriptor chain its associated with
 */
struct virtq_receive_callback_entry {
	/**
	 * callback function pointer
	 */
	virtq_receive_callback cb;
	/**
	 * argument passed to the callback function
	 */
	void *opaque;
};

/**
 * @brief virtqueue
 *
 * contains structures required for virtqueue operation
 */
struct virtq {
	/**
	 * lock used to synchronize operations on virtqueue
	 */
	struct k_spinlock lock;

	/**
	 * size of virtqueue
	 */
	uint16_t num;
	/**
	 * array with descriptors
	 */
	struct virtq_desc *desc;
	/**
	 * available ring
	 */
	struct virtq_avail *avail;
	/**
	 * used ring
	 */
	struct virtq_used *used;

	/**
	 * last seen idx in used ring, used to determine first descriptor to process
	 * after receiving virtqueue interrupt
	 */
	uint16_t last_used_idx;
	/**
	 * Stack containing indexes of free descriptors. Because virtio devices are
	 * not required to use received descriptors in order (see 2.7.9) unless
	 * VIRTIO_F_IN_ORDER was offered, we can't use array with descriptors as another
	 * ring buffer, always taking next descriptor. This is an auxiliary structure to
	 * easily determine next free descriptor
	 *
	 * @warning This is implementation state. The virtqueue helpers are its only
	 * supported users: they pop with K_NO_WAIT while holding @ref virtq.lock.
	 * Blocking on this stack directly, or pushing to it, can break the free
	 * descriptor accounting and is not supported.
	 */
	struct k_stack free_desc_stack;

	/**
	 * amount of free descriptors in the free_desc_stack
	 *
	 * Updated with atomic operations while @ref virtq.lock is held, and equal
	 * to the stack depth at every completed allocation or release. It is the
	 * authority for availability: submission rechecks it under the lock.
	 * Outside the lock it is only a scheduling hint.
	 *
	 * @warning This is implementation state; modification is not supported.
	 */
	atomic_t free_desc_n;

	/**
	 * number of threads currently blocked waiting for free descriptors
	 *
	 * @warning This is implementation state; modification is not supported.
	 */
	atomic_t waiters;

	/**
	 * serializes the blocking allocation path and pairs with @ref virtq.wait_cond
	 *
	 * @warning This is implementation state; use by drivers is not supported.
	 */
	struct k_mutex wait_lock;

	/**
	 * broadcast when descriptors are released while @ref virtq.waiters is non-zero
	 *
	 * @warning This is implementation state; use by drivers is not supported.
	 */
	struct k_condvar wait_cond;

	/**
	 * notifies @ref virtq.wait_cond waiters from the shared virtio wake workqueue
	 *
	 * @warning This is implementation state; use by drivers is not supported.
	 */
	struct k_work wake_work;

	/**
	 * synchronization storage in the queue's heap allocation for flushing wake_work
	 *
	 * @warning This is implementation state; use by drivers is not supported.
	 */
	struct k_work_sync *wake_sync;

	/**
	 * array with callbacks invoked after receiving buffers back from the device
	 */
	struct virtq_receive_callback_entry *recv_cbs;
};

/**
 * @brief creates virtqueue
 *
 * Starts the shared virtio wake workqueue on first use, so this must be called
 * from thread context, at POST_KERNEL or later.
 *
 * @param v virtqueue to be created
 * @param size size of the virtqueue
 * @retval 0 Virtqueue created.
 * @retval -EINVAL Size is not a power of two no greater than 32 KiB, or the
 * caller is in interrupt context.
 * @retval -ENOMEM Backing memory could not be allocated.
 */
int virtq_create(struct virtq *v, size_t size);

/**
 * @brief frees virtqueue
 *
 * Must be called from thread context. The caller must first quiesce all
 * submission and completion sources of this virtqueue, ensure that no thread
 * is blocked waiting for its descriptors and that no release is in flight.
 * The shared wake workqueue is not affected; only this virtqueue's pending
 * notification is completed before its memory is released.
 *
 * @param v virtqueue to be freed
 */
void virtq_free(struct virtq *v);

/**
 * @brief single buffer passed to virtq_add_buffer_chain
 */
struct virtq_buf {
	/**
	 * virtual address of the buffer
	 */
	void *addr;
	/**
	 * length of the buffer
	 */
	uint32_t len;
};

/**
 * @brief adds chain of buffers to the virtqueue
 *
 * The chain is allocated all-or-nothing: either every descriptor it needs is
 * taken from the free pool or none is taken. A chain needing more descriptors
 * than the queue has fails immediately with -EINVAL and never waits, so it
 * cannot wait on itself.
 *
 * Timeout handling:
 * - K_NO_WAIT attempts the allocation once and returns -EBUSY when the free
 *   descriptors are insufficient. This works in any context.
 * - K_FOREVER or any finite value blocks the calling thread for up to
 *   @p timeout, until the whole chain fits. The timeout is one budget for the
 *   whole chain, not a fresh budget per descriptor; it starts when the call is
 *   made and also covers the initial non-blocking attempt. A timed-out call
 *   returns -EAGAIN.
 * - Interrupt context never waits, whatever timeout is passed: it either
 *   succeeds or returns -EBUSY. A completion callback that resubmits to its
 *   own queue is such a caller.
 *
 * Blocking requires an independent completion path. A thread or workqueue
 * that is the only source of progress for this queue must not wait for
 * descriptors only it can return. Blocking is not allowed while holding a
 * spinlock or with interrupts locked; use K_NO_WAIT instead. Waiting threads
 * are woken greedily and in no particular order, so a sustained stream of
 * small chains can delay a large one.
 *
 * Because the wait is implemented with a condition variable, a call that
 * times out can still spend an unbounded time reacquiring the internal mutex
 * after the deadline has passed; the deadline is otherwise never extended by
 * wakeups. Aborting a thread that is blocked in this call is not supported:
 * let it return and join it before the queue is freed.
 *
 * Note that according to spec 2.7.13.3 the device may access the buffers as soon
 * as the avail->idx is increased, which is done at the end of this function, so
 * the device may access the buffers without notifying it with virtio_notify_virtqueue
 *
 * @param v virtqueue it operates on
 * @param bufs array of buffers to be added to the virtqueue
 * @param bufs_size amount of buffers
 * @param device_readable_count amount of buffers readable by the device, the first
 * device_readable_count buffers will be set as device readable
 * @param cb callback to be invoked after device returns the buffer chain, can be NULL
 * @param cb_opaque opaque value that will be passed to the cb
 * @param timeout how long to wait for the whole chain to fit
 * @retval 0 Chain was queued.
 * @retval -EINVAL Empty chain, chain larger than the queue, or chain longer
 * than 2^32 bytes.
 * @retval -EBUSY Free descriptors were insufficient and the caller cannot wait.
 * @retval -EAGAIN The whole chain did not fit within @p timeout.
 */
int virtq_add_buffer_chain(
	struct virtq *v, struct virtq_buf *bufs, uint16_t bufs_size,
	uint16_t device_readable_count, virtq_receive_callback cb, void *cb_opaque,
	k_timeout_t timeout
);

/**
 * @brief check whether a buffer range is in the linear RAM mapping
 *
 * The whole range must lie in the boot-time RAM mapping used by
 * k_mem_phys_addr(): the kernel image or all RAM on MMU systems, and the
 * chosen SRAM region on systems without an MMU.
 *
 * With demand paging enabled, all ranges are rejected. A NULL address or a
 * range whose end overflows is also rejected. A zero-length range must start
 * strictly inside the region.
 *
 * Device address windows, IOMMU mappings and cache coherency are not checked.
 * The caller must keep the buffer mapping valid while the device uses it.
 *
 * @param addr start of the buffer
 * @param len length of the buffer in bytes
 * @retval true The whole range is in the boot-time RAM mapping.
 * @retval false The range is outside the boot mapping or cannot be validated.
 */
bool virtio_buf_is_in_linear_mapping(const void *addr, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_VIRTIO_VIRTQUEUE_H_ */
