/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/barrier.h>
#include <errno.h>

LOG_MODULE_REGISTER(virtio, CONFIG_VIRTIO_LOG_LEVEL);

/*
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

/*
 * The maximum queue size is 2^15 (see 2.7),
 * so any 16bit value larger than that can be used as a sentinel in the next field
 */
#define VIRTQ_DESC_NEXT_SENTINEL 0xffff

/* According to the spec 2.7.5.2 the maximum size of descriptor chain is 4GB */
#define MAX_DESCRIPTOR_CHAIN_LENGTH ((uint64_t)1 << 32)

int virtq_create(struct virtq *v, size_t size)
{
	__ASSERT(IS_POWER_OF_TWO(size), "size of virtqueue must be a power of 2");
	__ASSERT(size <= KB(32), "size of virtqueue must be at most 32KB");
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
	size_t v_size = shared_size + sizeof(struct virtq_receive_callback_entry) * size;

	uint8_t *v_area = k_aligned_alloc(16, v_size);

	if (!v_area) {
		LOG_ERR("unable to allocate virtqueue");
		return -ENOMEM;
	}

	v->num = size;
	v->desc = (struct virtq_desc *)v_area;
	v->avail = (struct virtq_avail *)((uint8_t *)v->desc + descriptor_table_size);
	v->used = (struct virtq_used *)((uint8_t *)v->avail + available_ring_size + used_ring_pad);
	v->recv_cbs = (struct virtq_receive_callback_entry *)((uint8_t *)v->used + used_ring_size);

	/*
	 * At the beginning of the descriptor table, the available ring and the used ring have to be
	 * set to zero. It's the case for both PCI (4.1.5.1.3) and MMIO (4.2.3.2) transport options.
	 * Its unspecified for channel I/O (chapter 4.3), but its used on platforms not supported by
	 * Zephyr, so we don't have to handle it here
	 */
	memset(v_area, 0, v_size);

	v->last_used_idx = 0;

	k_stack_alloc_init(&v->free_desc_stack, size);
	for (uint16_t i = 0; i < size; i++) {
		k_stack_push(&v->free_desc_stack, i);
	}
	v->free_desc_n = size;

	return 0;
}

void virtq_free(struct virtq *v)
{
	k_free(v->desc);
	k_stack_cleanup(&v->free_desc_stack);
}

static int virtq_add_available(struct virtq *v, uint16_t desc_idx)
{
	uint16_t new_idx_le = sys_cpu_to_le16(sys_le16_to_cpu(v->avail->idx) % v->num);

	v->avail->ring[new_idx_le] = sys_cpu_to_le16(desc_idx);
	barrier_dmem_fence_full();
	v->avail->idx = sys_cpu_to_le16(sys_le16_to_cpu(v->avail->idx) + 1);

	return 0;
}

int virtq_add_buffer_chain(
	struct virtq *v, struct virtq_buf *bufs, uint16_t bufs_size,
	uint16_t device_readable_count, virtq_receive_callback cb, void *cb_opaque,
	k_timeout_t timeout)
{
	uint64_t total_len = 0;

	for (int i = 0; i < bufs_size; i++) {
		total_len += bufs[i].len;
	}

	if (total_len > MAX_DESCRIPTOR_CHAIN_LENGTH) {
		LOG_ERR("buffer chain is longer than 2^32 bytes");
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&v->lock);

	if (v->free_desc_n < bufs_size && !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		/* we don't have enough free descriptors to push all buffers to the queue */
		k_spin_unlock(&v->lock, key);
		return -EBUSY;
	}

	uint16_t prev_desc = VIRTQ_DESC_NEXT_SENTINEL;
	uint16_t head = VIRTQ_DESC_NEXT_SENTINEL;

	for (uint16_t buf_n = 0; buf_n < bufs_size; buf_n++) {
		uint16_t desc;

		/*
		 * we've checked before that we have enough free descriptors
		 * and the queue is locked, so popping from stack is guaranteed
		 * to succeed and we don't have to check its return value
		 */
		virtq_get_free_desc(v, &desc, timeout);

		uint16_t desc_le = sys_cpu_to_le16(desc);

		if (head == VIRTQ_DESC_NEXT_SENTINEL) {
			head = desc;
		}
		v->desc[desc_le].addr = k_mem_phys_addr(bufs[buf_n].addr);
		v->desc[desc_le].len = bufs[buf_n].len;
		if (buf_n < device_readable_count) {
			v->desc[desc_le].flags = 0;
		} else {
			v->desc[desc_le].flags = VIRTQ_DESC_F_WRITE;
		}
		if (buf_n < bufs_size - 1) {
			v->desc[desc_le].flags |= VIRTQ_DESC_F_NEXT;
		} else {
			v->desc[desc_le].next = 0;
		}

		if (prev_desc != VIRTQ_DESC_NEXT_SENTINEL) {
			uint16_t prev_desc_le = sys_cpu_to_le16(prev_desc);

			v->desc[prev_desc_le].next = desc_le;
		}

		prev_desc = desc;
	}

	v->recv_cbs[head].cb = cb;
	v->recv_cbs[head].opaque = cb_opaque;

	virtq_add_available(v, head);

	k_spin_unlock(&v->lock, key);

	return 0;
}

int virtq_get_free_desc(struct virtq *v, uint16_t *desc_idx, k_timeout_t timeout)
{
	stack_data_t desc;

	int ret = k_stack_pop(&v->free_desc_stack, &desc, timeout);

	if (ret == 0) {
		*desc_idx = (uint16_t)desc;
		v->free_desc_n--;
	}

	return ret;
}

void virtq_add_free_desc(struct virtq *v, uint16_t desc_idx)
{
	k_stack_push(&v->free_desc_stack, desc_idx);
	v->free_desc_n++;
}
