/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_COMMON_H_
#define ZEPHYR_VIRTIO_VIRTIO_COMMON_H_

#include <zephyr/drivers/virtio/virtio_config.h>

/**
 * Common virtio isr
 *
 * @param dev virtio device it operates on
 * @param isr_status value of isr status register
 * @param virtqueue_count amount of available virtqueues
 */
void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count);

#endif /*ZEPHYR_VIRTIO_VIRTIO_COMMON_H_*/
