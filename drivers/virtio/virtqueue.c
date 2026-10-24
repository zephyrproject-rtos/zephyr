/*
 * Copyright (c) 2024-2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
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
#define MAX_DESCRIPTOR_CHAIN_LENGTH BIT64(32)

int virtq_create(struct virtq *v, size_t max_size)
{
	size_t size = MIN(v->max_num, max_size);

	/*
	 * A size of 0 denotes a queue the driver enumerates but does not use
	 * (e.g. the virtiofs high priority queue). Such a queue is set up empty
	 * and is never operated on. Other split virtqueue sizes must be powers
	 * of two no greater than 32 KiB. The size a virtqueue is defined with is
	 * checked at build time, but the maximum the device reports, which may
	 * limit it further, is not trusted to be valid.
	 */
	if (size != 0U && !IS_POWER_OF_TWO(size)) {
		LOG_ERR("invalid virtqueue size %zu", size);
		return -EINVAL;
	}

	/*
	 * The descriptor table, the available ring and the used ring are laid out for the
	 * requested size at the start of the storage. We are supporting only modern virtio, so
	 * we don't have to adhere to additional constraints from spec 2.7.2
	 */
	uint8_t *ring_area = (uint8_t *)v->desc;

	*v = (struct virtq){
		.num = size,
		.max_num = v->max_num,
		.desc = v->desc,
		.avail = (struct virtq_avail *)(ring_area + VIRTQ_AVAIL_RING_OFFSET(size)),
		.used = (struct virtq_used *)(ring_area + VIRTQ_USED_RING_OFFSET(size)),
		.free_desc_buf = v->free_desc_buf,
		.free_desc_n = size,
		.recv_cbs = v->recv_cbs,
	};

	/*
	 * At the beginning of the descriptor table, the available ring and the used ring have to be
	 * set to zero. It's the case for both PCI (4.1.5.1.3) and MMIO (4.2.3.2) transport options.
	 * Its unspecified for channel I/O (chapter 4.3), but its used on platforms not supported by
	 * Zephyr, so we don't have to handle it here
	 */
	memset(ring_area, 0, VIRTQ_RING_AREA_SIZE(size));
	memset(v->recv_cbs, 0, size * sizeof(*v->recv_cbs));

	k_stack_init(&v->free_desc_stack, v->free_desc_buf, size);
	for (uint16_t i = 0; i < size; i++) {
		k_stack_push(&v->free_desc_stack, i);
	}

	return 0;
}

static void virtq_add_available(struct virtq *v, uint16_t desc_idx)
{
	uint16_t idx = sys_le16_to_cpu(v->avail->idx);

	v->avail->ring[idx & (v->num - 1)] = sys_cpu_to_le16(desc_idx);
	barrier_dmem_fence_full();
	v->avail->idx = sys_cpu_to_le16(idx + 1);
}

static void virtq_return_desc_chain(struct virtq *v, uint16_t head, uint16_t count)
{
	uint16_t curr = head;

	for (uint16_t i = 0; i < count; i++) {
		uint16_t next = sys_le16_to_cpu(v->desc[curr].next);

		virtq_add_free_desc(v, curr);
		curr = next;
	}
}

int virtq_add_buffer_chain(
	struct virtq *v, struct virtq_buf *bufs, uint16_t bufs_size,
	uint16_t device_readable_count, virtq_receive_callback cb, void *cb_opaque,
	k_timeout_t timeout)
{
	if (bufs_size == 0) {
		return -EINVAL;
	}

	uint64_t total_len = 0;

	for (int i = 0; i < bufs_size; i++) {
		total_len += bufs[i].len;
	}

	if (total_len > MAX_DESCRIPTOR_CHAIN_LENGTH) {
		LOG_ERR("buffer chain is longer than 2^32 bytes");
		return -EINVAL;
	}

	uint16_t prev_desc = VIRTQ_DESC_NEXT_SENTINEL;
	uint16_t head = VIRTQ_DESC_NEXT_SENTINEL;

	for (uint16_t buf_n = 0; buf_n < bufs_size; buf_n++) {
		uint16_t desc;
		/* popped outside the queue lock as k_stack_pop() may block */
		int ret = virtq_get_free_desc(v, &desc, timeout);

		if (ret != 0) {
			virtq_return_desc_chain(v, head, buf_n);
			return ret;
		}

		if (head == VIRTQ_DESC_NEXT_SENTINEL) {
			head = desc;
		}

		uint16_t flags = buf_n < device_readable_count ? 0 : VIRTQ_DESC_F_WRITE;

		if (buf_n < bufs_size - 1) {
			flags |= VIRTQ_DESC_F_NEXT;
		} else {
			v->desc[desc].next = 0;
		}
		v->desc[desc].addr = sys_cpu_to_le64(k_mem_phys_addr(bufs[buf_n].addr));
		v->desc[desc].len = sys_cpu_to_le32(bufs[buf_n].len);
		v->desc[desc].flags = sys_cpu_to_le16(flags);

		if (prev_desc != VIRTQ_DESC_NEXT_SENTINEL) {
			v->desc[prev_desc].next = sys_cpu_to_le16(desc);
		}

		prev_desc = desc;
	}

	v->recv_cbs[head].cb = cb;
	v->recv_cbs[head].opaque = cb_opaque;

	k_spinlock_key_t key = k_spin_lock(&v->lock);

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
		K_SPINLOCK(&v->lock) {
			v->free_desc_n--;
		}
	}

	return ret;
}

void virtq_add_free_desc(struct virtq *v, uint16_t desc_idx)
{
	k_stack_push(&v->free_desc_stack, desc_idx);
	K_SPINLOCK(&v->lock) {
		v->free_desc_n++;
	}
}
