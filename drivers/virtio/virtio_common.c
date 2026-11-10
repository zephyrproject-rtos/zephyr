/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
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
			uint16_t used_idx;

			/* An unused, zero-size queue has no used ring to consume. */
			if (vq == NULL || vq->num == 0U) {
				continue;
			}

			used_idx = sys_le16_to_cpu(vq->used->idx);

			while (vq->last_used_idx != used_idx) {
				uint16_t idx = vq->last_used_idx % vq->num;
				uint32_t chain_head = sys_le32_to_cpu(vq->used->ring[idx].id);
				uint32_t used_len = sys_le32_to_cpu(vq->used->ring[idx].len);

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

				/*
				 * We are making a copy here, because chain will be
				 * returned before invoking the callback and may be
				 * overwritten by the time callback is called. This
				 * is to allow callback to immediately place the
				 * descriptors back in the avail_ring
				 */
				struct virtq_receive_callback_entry cbe =
					vq->recv_cbs[chain_head];

				int ret = virtq_release_chain(vq, (uint16_t)chain_head);

				vq->last_used_idx++;

				/*
				 * A rejected chain was not partially freed and
				 * is not reported to the driver: its callback is
				 * skipped, and only the used index advances.
				 */
				if (ret == 0 && cbe.cb != NULL) {
					cbe.cb(cbe.opaque, used_len);
				}
			}
		}
	}
	if (isr_status & VIRTIO_DEVICE_CONFIGURATION_INTERRUPT) {
		LOG_ERR("device configuration change interrupt is currently unsupported");
	}
}
