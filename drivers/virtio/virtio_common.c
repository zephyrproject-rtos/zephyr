/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/virtio/virtio.h>
#include <zephyr/virtio/virtqueue.h>
#include "virtio_common.h"

LOG_MODULE_REGISTER(virtio_common, CONFIG_VIRTIO_LOG_LEVEL);

void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count)
{
	if (isr_status & VIRTIO_QUEUE_INTERRUPT) {
		for (int i = 0; i < virtqueue_count; i++) {
			struct virtq *vq = virtio_get_virtqueue(dev, i);
			uint16_t used_idx = sys_le16_to_cpu(vq->used->idx);

			while (vq->last_used_idx != used_idx) {
				uint16_t idx = vq->last_used_idx % vq->num;
				uint16_t idx_le = sys_cpu_to_le16(idx);
				uint16_t chain_head_le = vq->used->ring[idx_le].id;
				uint16_t chain_head = sys_le16_to_cpu(chain_head_le);
				uint32_t used_len = sys_le32_to_cpu(
					vq->used->ring[idx_le].len
				);

				/*
				 * We are making a copy here, because chain will be
				 * returned before invoking the callback and may be
				 * overwritten by the time callback is called. This
				 * is to allow callback to immediately place the
				 * descriptors back in the avail_ring
				 */
				struct virtq_receive_callback_entry cbe =
					vq->recv_cbs[chain_head];

				uint16_t next = chain_head;
				bool last = false;

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

					next = vq->desc[curr_le].next;
					last = !(vq->desc[curr_le].flags & VIRTQ_DESC_F_NEXT);
					virtq_add_free_desc(vq, curr);
				}

				vq->last_used_idx++;

				if (cbe.cb) {
					cbe.cb(cbe.opaque, used_len);
				}
			}
		}
	}
	if (isr_status & VIRTIO_DEVICE_CONFIGURATION_INTERRUPT) {
		LOG_ERR("device configuration change interrupt is currently unsupported");
	}
}
