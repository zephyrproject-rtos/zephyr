/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * VIRTIO common definitions based on the specification.
 *
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

#ifndef ZEPHYR_DRIVERS_VIRTIO_VIRTIO_CONFIG_H_
#define ZEPHYR_DRIVERS_VIRTIO_VIRTIO_CONFIG_H_

/**
 * @name Virtio device status bits
 *
 * Bit positions of the device status field.
 * These are described in
 * <a href="https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#section.2.1">
 * 2.1 Device Status Field </a>
 *
 * @{
 */

/** Indicates the guest has found and recognized the device presence. */
#define DEVICE_STATUS_ACKNOWLEDGE 0
/** Indicates the guest driver is ready to drive the device. */
#define DEVICE_STATUS_DRIVER      1
/** Indicates the driver has successfully set up the device and is ready. */
#define DEVICE_STATUS_DRIVER_OK   2
/** Indicates the driver and device agreed on the negotiated feature set. */
#define DEVICE_STATUS_FEATURES_OK 3
/** Indicates the device requests a reset to recover from an error. */
#define DEVICE_STATUS_NEEDS_RESET 6
/** Indicates the device has experienced a non-recoverable error. */
#define DEVICE_STATUS_FAILED      7

/** @} */

/**
 * @name Feature Bits
 *
 * Negotiable device-independent feature bit positions.
 * These are described in
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#chapter.6">
 * 6 Reserved Feature Bits
 * </a>
 *
 * @{
 */

/** Indicates descriptors can reference descriptor tables. */
#define VIRTIO_RING_F_INDIRECT_DESC 28
/** Indicates driver/device use event index for notifications. */
#define VIRTIO_RING_F_EVENT_IDX     29
/** Indicates device complies with Virtio 1.0+ semantics. */
#define VIRTIO_F_VERSION_1          32
/** Indicates device needs platform-specific handling. */
#define VIRTIO_F_ACCESS_PLATFORM    33

/** @} */

/**
 * @name Ring Flag Bits
 *
 * Available and used ring flag bit positions as described in
 *
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#subsection.2.7.6">
 * 2.7.6 The Virtqueue Available Ring
 * </a>
 * and
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#subsection.2.7.8">
 * 2.7.8 The Virtqueue Used Ring
 * </a>
 *
 * @{
 */

/**
 * Driver requests the device to skip interrupts.
 * This is valid if @ref VIRTIO_RING_F_EVENT_IDX negotiated.
 */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

/**
 * Device requests the driver to suppress notifications.
 * This is valid if @ref VIRTIO_RING_F_EVENT_IDX negotiated.
 */
#define VIRTQ_USED_F_NO_NOTIFY 1

/** @} */

/**
 * @name Ranges of feature bits
 *
 * These described in
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#section.2.2">
 * 2.2 Feature Bits
 * </a>
 *
 * @{
 */

/** Start of the first device-specific feature range. */
#define DEV_TYPE_FEAT_RANGE_0_BEGIN 0
/** End of the first device-specific feature range. */
#define DEV_TYPE_FEAT_RANGE_0_END   23
/** Start of the second device-specific feature range. */
#define DEV_TYPE_FEAT_RANGE_1_BEGIN 50
/** End of the second device-specific feature range. */
#define DEV_TYPE_FEAT_RANGE_1_END   127

/** @} */

/**
 * @name Transport interrupt bits
 *
 * While defined separately in
 *
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#subsubsection.4.1.4.5">
 * 4.1.4.5 ISR status capabilit </a> for PCI and
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#subsection.4.2.2">
 * 4.2.2 MMIO Device Register Layout </a> for MMIO
 *
 * the same bits are responsible for the same interrupts, so defines
 * with them can be unified
 *
 * @{
 */

/** A virtqueue has pending used buffers. */
#define VIRTIO_QUEUE_INTERRUPT                1
/** Device configuration space has changed. */
#define VIRTIO_DEVICE_CONFIGURATION_INTERRUPT 2

/** @} */

/**
 * @name VIRTIO-MMIO registers
 *
 * The details are described in
 * <a href=
 * "https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf#subsection.4.2.2">
 * 4.2.2 MMIO Device Register Layout </a>
 *
 * @{
 */

/** Magic value identifying the virtio MMIO device. */
#define VIRTIO_MMIO_MAGIC_VALUE         0x000
/** Virtio specification version exposed by the device. */
#define VIRTIO_MMIO_VERSION             0x004
/** Device type identifier register. */
#define VIRTIO_MMIO_DEVICE_ID           0x008
/** Vendor-specific identifier register. */
#define VIRTIO_MMIO_VENDOR_ID           0x00c
/** Lower 32 bits of the device feature bitmap. */
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010
/** Selector choosing the device feature word. */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
/** Lower 32 bits of the negotiated driver feature bitmap. */
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020
/** Selector choosing the driver feature word. */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
/** Virtqueue index selected for subsequent accesses. */
#define VIRTIO_MMIO_QUEUE_SEL           0x030
/** Maximum queue size supported by the selected virtqueue. */
#define VIRTIO_MMIO_QUEUE_SIZE_MAX      0x034
/** Queue size chosen by the driver for the selected virtqueue. */
#define VIRTIO_MMIO_QUEUE_SIZE          0x038
/** Ready flag indicating driver ownership of the queue. */
#define VIRTIO_MMIO_QUEUE_READY         0x044
/** Doorbell register for queue notifications. */
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050
/** Pending interrupt summary bits. */
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060
/** Interrupt acknowledgment register. */
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064
/** Device status. */
#define VIRTIO_MMIO_STATUS              0x070
/** Lower 32 bits of the descriptor table address. */
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
/** Upper 32 bits of the descriptor table address. */
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
/** Lower 32 bits of the available ring address. */
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090
/** Upper 32 bits of the available ring address. */
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094
/** Lower 32 bits of the used ring address. */
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x0a0
/** Upper 32 bits of the used ring address. */
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x0a4
/** Shared memory region selector. */
#define VIRTIO_MMIO_SHM_SEL             0x0ac
/** Lower 32 bits of the shared memory length. */
#define VIRTIO_MMIO_SHM_LEN_LOW         0x0b0
/** Upper 32 bits of the shared memory length. */
#define VIRTIO_MMIO_SHM_LEN_HIGH        0x0b4
/** Lower 32 bits of the shared memory base address. */
#define VIRTIO_MMIO_SHM_BASE_LOW        0x0b8
/** Upper 32 bits of the shared memory base address. */
#define VIRTIO_MMIO_SHM_BASE_HIGH       0x0bc
/** Queue reset control register. */
#define VIRTIO_MMIO_QUEUE_RESET         0x0c0
/** Generation counter for configuration space. */
#define VIRTIO_MMIO_CONFIG_GENERATION   0x0fc
/** Base offset of the device configuration structure. */
#define VIRTIO_MMIO_CONFIG              0x100

/** @} */

#endif /* ZEPHYR_DRIVERS_VIRTIO_VIRTIO_CONFIG_H_ */
