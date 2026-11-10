/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_COMMON_H_
#define ZEPHYR_VIRTIO_VIRTIO_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/drivers/virtio/virtqueue.h>

/**
 * Common virtio isr
 *
 * @param dev virtio device it operates on
 * @param isr_status value of isr status register
 * @param virtqueue_count amount of available virtqueues
 */
void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count);

/**
 * Return a completed descriptor chain to the free pool.
 *
 * Validates the chain before publishing any descriptor, so a malformed chain
 * leaves the queue unchanged. Wakes blocked submitters when needed.
 *
 * @param v virtqueue it operates on
 * @param head index of the first descriptor of the chain
 * @retval 0 Chain was returned to the free pool.
 * @retval -EINVAL Chain head is out of range, the chain leaves the descriptor
 * table or does not terminate within the queue size, or it does not fit the
 * free list.
 */
int virtq_release_chain(struct virtq *v, uint16_t head);

#endif /*ZEPHYR_VIRTIO_VIRTIO_COMMON_H_*/
