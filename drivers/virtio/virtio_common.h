/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_COMMON_H_
#define ZEPHYR_VIRTIO_VIRTIO_COMMON_H_

#define DEVICE_STATUS_ACKNOWLEDGE 0
#define DEVICE_STATUS_DRIVER      1
#define DEVICE_STATUS_DRIVER_OK   2
#define DEVICE_STATUS_FEATURES_OK 3
#define DEVICE_STATUS_NEEDS_RESET 6
#define DEVICE_STATUS_FAILED      7

#define VIRTIO_F_VERSION_1 32

/* Ranges of feature bits for specific device types (see spec 2.2)*/
#define DEV_TYPE_FEAT_RANGE_0_BEGIN 0
#define DEV_TYPE_FEAT_RANGE_0_END   23
#define DEV_TYPE_FEAT_RANGE_1_BEGIN 50
#define DEV_TYPE_FEAT_RANGE_1_END   127

/*
 * While defined separately in 4.1.4.5 for PCI and in 4.2.2 for MMIO
 * the same bits are responsible for the same interrupts, so defines
 * with them can be unified
 */
#define VIRTIO_QUEUE_INTERRUPT                1
#define VIRTIO_DEVICE_CONFIGURATION_INTERRUPT 2


/**
 * Common virtio isr
 *
 * @param dev virtio device it operates on
 * @param isr_status value of isr status register
 * @param virtqueue_count amount of available virtqueues
 */
void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count);

#endif /*ZEPHYR_VIRTIO_VIRTIO_COMMON_H_*/
