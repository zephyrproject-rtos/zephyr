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

/*
 * VIRTIO-MMIO register definitions.
 *
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

#define VIRTIO_MMIO_MAGIC_VALUE         0x000
#define VIRTIO_MMIO_VERSION             0x004
#define VIRTIO_MMIO_DEVICE_ID           0x008
#define VIRTIO_MMIO_VENDOR_ID           0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL           0x030
#define VIRTIO_MMIO_QUEUE_SIZE_MAX      0x034
#define VIRTIO_MMIO_QUEUE_SIZE          0x038
#define VIRTIO_MMIO_QUEUE_READY         0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064
#define VIRTIO_MMIO_STATUS              0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x0a4
#define VIRTIO_MMIO_SHM_SEL             0x0ac
#define VIRTIO_MMIO_SHM_LEN_LOW         0x0b0
#define VIRTIO_MMIO_SHM_LEN_HIGH        0x0b4
#define VIRTIO_MMIO_SHM_BASE_LOW        0x0b8
#define VIRTIO_MMIO_SHM_BASE_HIGH       0x0bc
#define VIRTIO_MMIO_QUEUE_RESET         0x0c0
#define VIRTIO_MMIO_CONFIG_GENERATION   0x0fc
#define VIRTIO_MMIO_CONFIG              0x100

/**
 * Common virtio isr
 *
 * @param dev virtio device it operates on
 * @param isr_status value of isr status register
 * @param virtqueue_count amount of available virtqueues
 */
void virtio_isr(const struct device *dev, uint8_t isr_status, uint16_t virtqueue_count);

#endif /*ZEPHYR_VIRTIO_VIRTIO_COMMON_H_*/
