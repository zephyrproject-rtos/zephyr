/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include "virtio_common.h"

LOG_MODULE_REGISTER(virtio_common, CONFIG_VIRTIO_LOG_LEVEL);

void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count)
{
	if (isr_status & VIRTIO_QUEUE_INTERRUPT) {
		for (int i = 0; i < virtqueue_count; i++) {
			struct virtq *vq = virtio_get_virtqueue(dev, i);

			virtq_process_used(vq);
		}
	}
	if (isr_status & VIRTIO_DEVICE_CONFIGURATION_INTERRUPT) {
		LOG_ERR("device configuration change interrupt is currently unsupported");
	}
}
