/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_COMMON_H_
#define ZEPHYR_VIRTIO_VIRTIO_COMMON_H_

#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>

/**
 * Common virtio isr
 *
 * @param dev virtio device it operates on
 * @param isr_status value of isr status register
 * @param virtqueue_count amount of available virtqueues
 */
void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count);

/**
 * @brief checks whether the device has to be notified about added buffers
 *
 * The device may suppress notifications for a virtqueue while it is processing
 * it anyway (see spec 2.7.10), which the transports honor in
 * virtio_notify_virtqueue(). Must be called after the buffers were added.
 *
 * @param v virtqueue it operates on
 * @return true if the device has to be notified, false if it suppressed notifications
 */
static inline bool virtq_notification_needed(const struct virtq *v)
{
	/*
	 * Per spec 2.7.13.3.1 a memory barrier has to separate the available
	 * ring update from reading the flags. VIRTIO_F_EVENT_IDX is not
	 * negotiated, so the flags decide (see spec 2.7.10.1), otherwise
	 * avail_event would have to be consulted instead
	 */
	barrier_dmem_fence_full();

	return (sys_le16_to_cpu(v->used->flags) & VIRTQ_USED_F_NO_NOTIFY) == 0U;
}

#endif /*ZEPHYR_VIRTIO_VIRTIO_COMMON_H_*/
