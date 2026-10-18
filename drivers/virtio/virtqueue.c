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
	v->free_desc_n = size;

	return 0;
}

void virtq_free(struct virtq *v)
{
	k_free(v->desc);
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

static bool virtq_add_free_desc_locked(struct virtq *v, uint16_t desc_idx)
{
	/* A duplicate used entry must not grow the free count past the queue size. */
	if (v->free_desc_n >= v->num) {
		return false;
	}

	if (k_stack_push(&v->free_desc_stack, desc_idx) != 0) {
		return false;
	}

	v->free_desc_n++;

	return true;
}

void virtq_add_free_desc(struct virtq *v, uint16_t desc_idx)
{
	k_spinlock_key_t key = k_spin_lock(&v->lock);

	(void)virtq_add_free_desc_locked(v, desc_idx);
	k_spin_unlock(&v->lock, key);
}

uint16_t virtq_get_free_desc_count(struct virtq *v)
{
	k_spinlock_key_t key = k_spin_lock(&v->lock);
	uint16_t count = v->free_desc_n;

	k_spin_unlock(&v->lock, key);

	return count;
}

void virtq_process_used(struct virtq *v)
{
	void (*irq_cb)(void *) = NULL;
	void *irq_cb_data = NULL;
	k_spinlock_key_t key;
	uint16_t used_snapshot;
	uint16_t budget;

	if (v->num == 0U) {
		return;
	}

	/*
	 * Process at most the entries completed at entry time. A callback may
	 * requeue a buffer and the device may complete it immediately, so
	 * draining until the used ring is empty could keep this pass running
	 * indefinitely. Newer completions are handled by the next call.
	 */
	key = k_spin_lock(&v->lock);
	if (v->faulted) {
		k_spin_unlock(&v->lock, key);
		return;
	}

	used_snapshot = sys_le16_to_cpu(v->used->idx);
	budget = (uint16_t)(used_snapshot - v->last_used_idx);
	if (budget > v->num) {
		/*
		 * A compliant device cannot complete more buffers than the queue
		 * holds. Reject the batch: the last num slots may belong to older
		 * completions, so processing them could return in-flight buffers.
		 */
		v->invalid_used_count++;
		v->faulted = true;
		k_spin_unlock(&v->lock, key);
		return;
	}

	barrier_dmem_fence_full();
	k_spin_unlock(&v->lock, key);

	while (budget > 0U) {
		uint16_t used_idx;

		key = k_spin_lock(&v->lock);

		/* Another context may have faulted the queue while we were unlocked. */
		if (v->faulted) {
			k_spin_unlock(&v->lock, key);
			break;
		}

		used_idx = sys_le16_to_cpu(v->used->idx);
		barrier_dmem_fence_full();

		/* Another context may have consumed the remaining entries. */
		if (v->last_used_idx == used_idx) {
			k_spin_unlock(&v->lock, key);
			break;
		}

		uint16_t idx = v->last_used_idx % v->num;
		uint32_t chain_head = sys_le32_to_cpu(v->used->ring[idx].id);
		uint32_t used_len = sys_le32_to_cpu(v->used->ring[idx].len);

		/* The device writes the used ring and its descriptor id is untrusted. */
		if (chain_head >= v->num) {
			v->invalid_used_count++;
			v->last_used_idx++;
			k_spin_unlock(&v->lock, key);
			budget--;
			continue;
		}

		struct virtq_receive_callback_entry cbe = v->recv_cbs[chain_head];
		uint16_t curr = (uint16_t)chain_head;
		uint16_t chain_len = 1U;
		bool valid = true;

		/*
		 * Validate the whole chain before touching the free stack. A
		 * malformed or cyclic chain must not return descriptors or feed
		 * duplicates into the free stack.
		 */
		for (uint16_t i = 0U; i < v->num; i++) {
			uint16_t flags = sys_le16_to_cpu(v->desc[curr].flags);

			if ((flags & VIRTQ_DESC_F_NEXT) == 0U) {
				break;
			}

			uint16_t next = sys_le16_to_cpu(v->desc[curr].next);

			if (next >= v->num) {
				valid = false;
				break;
			}

			curr = next;
			chain_len++;

			if (i == (uint16_t)(v->num - 1U)) {
				/* No terminal descriptor within the queue size. */
				valid = false;
			}
		}

		if (valid && ((uint32_t)v->free_desc_n + chain_len > v->num)) {
			valid = false;
		}

		v->last_used_idx++;
		if (!valid) {
			v->invalid_used_count++;
			k_spin_unlock(&v->lock, key);
			budget--;
			continue;
		}

		curr = (uint16_t)chain_head;
		for (uint16_t i = 0U; i < chain_len; i++) {
			uint16_t next = sys_le16_to_cpu(v->desc[curr].next);

			if (!virtq_add_free_desc_locked(v, curr)) {
				v->invalid_used_count++;
			}
			curr = next;
		}

		v->irq_pending = true;
		k_spin_unlock(&v->lock, key);
		budget--;

		if (cbe.cb != NULL) {
			cbe.cb(cbe.opaque, used_len);
		}
	}

	/*
	 * Deliver the coalesced notification once per pass, including for
	 * completions reclaimed by a thread.
	 */
	key = k_spin_lock(&v->lock);
	if (!v->faulted && k_is_in_isr() && v->irq_pending) {
		v->irq_pending = false;
		irq_cb = v->irq_cb;
		irq_cb_data = v->irq_cb_data;
	}

	k_spin_unlock(&v->lock, key);
	if (irq_cb != NULL) {
		irq_cb(irq_cb_data);
	}
}
