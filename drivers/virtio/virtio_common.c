/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include "virtio_common.h"

LOG_MODULE_REGISTER(virtio_common, CONFIG_VIRTIO_LOG_LEVEL);

void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count)
{
	if (isr_status & VIRTIO_QUEUE_INTERRUPT) {
		for (int i = 0; i < virtqueue_count; i++) {
			struct virtq *vq = virtio_get_virtqueue(dev, i);
			uint16_t used_idx = sys_le16_to_cpu(vq->used->idx);
			uint16_t first_completed = UINT16_MAX;
			uint16_t *last_completed = &first_completed;

			while (vq->last_used_idx != used_idx) {
				uint16_t idx = vq->last_used_idx % vq->num;
				uint16_t idx_le = sys_cpu_to_le16(idx);
				uint32_t chain_head = sys_le32_to_cpu(vq->used->ring[idx_le].id);

				/*
				 * The used ring is written by the (untrusted) device.
				 * A descriptor id beyond the queue size would index
				 * recv_cbs[]/desc[] out of bounds and lead to an
				 * arbitrary callback pointer being invoked below.
				 */
				if (chain_head >= vq->num) {
					LOG_ERR("invalid used ring id %u (num %u)",
						chain_head, vq->num);
					vq->last_used_idx++;
					continue;
				}

				uint16_t chain_head_idx = (uint16_t)chain_head;
				k_spinlock_key_t key = k_spin_lock(&vq->lock);
				bool in_flight =
					atomic_test_and_clear_bit(vq->in_flight, chain_head_idx);

				if (in_flight) {
					struct virtq_receive_callback_entry *cbe =
						&vq->recv_cbs[chain_head_idx];

					cbe->used_len = sys_le32_to_cpu(vq->used->ring[idx_le].len);
					cbe->completed_next = UINT16_MAX;
					*last_completed = chain_head_idx;
					last_completed = &cbe->completed_next;
				}

				k_spin_unlock(&vq->lock, key);

				if (!in_flight) {
					LOG_ERR("used ring id %u does not name an in-flight chain",
						chain_head);
					vq->last_used_idx++;
					continue;
				}

				vq->last_used_idx++;
			}

			/*
			 * Defer callbacks until the whole batch is checked, so a callback
			 * cannot resubmit a head and make a duplicate entry valid.
			 * shortcut: late duplicates after head reuse are indistinguishable;
			 * upgrade if used ring IDs gain a per-submission generation.
			 */
			while (first_completed != UINT16_MAX) {
				uint16_t chain_head_idx = first_completed;
				struct virtq_receive_callback_entry cbe =
					vq->recv_cbs[chain_head_idx];

				first_completed = cbe.completed_next;
				uint16_t next = chain_head_idx;
				bool last = false;
				uint16_t walked = 0U;

				/*
				 * We are done processing the descriptor chain, and
				 * we can add used descriptors back to the free stack.
				 * The only thing left to do is calling the callback
				 * associated with the chain, but it was saved above on
				 * the stack, so other code is free to use the descriptors
				 */
				while (!last) {
					uint16_t curr = next;
					uint16_t curr_le = sys_cpu_to_le16(curr);

					if (walked++ >= vq->num) {
						LOG_ERR("descriptor chain from %u too long",
							chain_head_idx);
						break;
					}

					next = vq->desc[curr_le].next;
					last = !(vq->desc[curr_le].flags & VIRTQ_DESC_F_NEXT);
					virtq_add_free_desc(vq, curr);
				}

				if (cbe.cb) {
					cbe.cb(cbe.opaque, cbe.used_len);
				}
			}
		}
	}
	if (isr_status & VIRTIO_DEVICE_CONFIGURATION_INTERRUPT) {
		LOG_ERR("device configuration change interrupt is currently unsupported");
	}
}
