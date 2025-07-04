/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTQUEUE_H_
#define ZEPHYR_VIRTIO_VIRTQUEUE_H_
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
	 * ring buffer, always taking next descriptor. This is an auxilary structure to
	 * easily determine next free descriptor
	 */
	struct k_stack free_desc_stack;

	/**
	 * amount of free descriptors in the free_desc_stack
	 */
	uint16_t free_desc_n;

	/**
	 * array with callbacks invoked after receiving buffers back from the device
	 */
	struct virtq_receive_callback_entry *recv_cbs;
};


/**
 * @brief creates virtqueue
 *
 * @param v virtqueue to be created
 * @param size size of the virtqueue
 * @return 0 or error code on failure
 */
int virtq_create(struct virtq *v, size_t size);

/**
 * @brief frees virtqueue
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
 * Note that according to spec 2.7.13.3 the device may access the buffers as soon
 * as the avail->idx is increased, which is done at the end of this function, so
 * the device may access the buffers without notifying it with virtio_notify_virtqueue
 *
 * @param v virtqueue it operates on
 * @param bufs array of buffers to be added to the virtqueue
 * @param bufs_size amount of buffers
 * @param device_readable_count amount of bufferes readable by the device, the first
 * device_readable_count buffers will be set as device readable
 * @param cb callback to be invoked after device returns the buffer chain, can be NULL
 * @param cb_opaque opaque value that will be passed to the cb
 * @param timeout amount of time it will wait for free descriptors, with K_NO_WAIT it
 * can be called from isr
 * @return 0 or error code on failure
 */
int virtq_add_buffer_chain(
	struct virtq *v, struct virtq_buf *bufs, uint16_t bufs_size,
	uint16_t device_readable_count, virtq_receive_callback cb, void *cb_opaque,
	k_timeout_t timeout
);

/**
 * @brief adds free descriptor back
 *
 * @param v virtqueue it operates on
 * @param desc_idx index of returned descriptor
 */
void virtq_add_free_desc(struct virtq *v, uint16_t desc_idx);

/**
 * @brief gets next free descriptor
 *
 * @param v virtqueue it operates on
 * @param desc_idx address where index of descriptor will be stored
 * @param timeout amount of time it will wait for free descriptor, with K_NO_WAIT it
 * can be called from isr
 * @return 0 or error code on failure
 */
int virtq_get_free_desc(struct virtq *v, uint16_t *desc_idx, k_timeout_t timeout);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_VIRTIO_VIRTQUEUE_H_ */
