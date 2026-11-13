/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_H_
#define ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>

/**
 * Validate a device-supplied notification offset against the NOTIFY region.
 *
 * The notification value is a 16-bit queue index. Both the product
 * queue_notify_off * notify_off_multiplier and the complete two-byte access
 * must fit in the region described by the NOTIFY capability length.
 *
 * @param region_len NOTIFY capability length in bytes
 * @param queue_notify_off device-supplied per-queue offset
 * @param multiplier device-supplied notification stride
 * @param offset calculated region-relative offset on success
 *
 * @retval true the complete notification access is inside the region
 * @retval false the device-supplied values cannot be used safely
 */
static inline bool virtio_pci_notify_offset_valid(uint32_t region_len, uint16_t queue_notify_off,
						  uint32_t multiplier, size_t *offset)
{
	uint64_t off = (uint64_t)queue_notify_off * (uint64_t)multiplier;

#if SIZE_MAX < UINT64_MAX
	if (off > (uint64_t)SIZE_MAX) {
		return false;
	}
#endif

	if (region_len < sizeof(uint16_t)) {
		return false;
	}

	if (off > (uint64_t)region_len - sizeof(uint16_t)) {
		return false;
	}

	*offset = (size_t)off;

	return true;
}

/**
 * Write a virtqueue notification if the device-supplied offset is valid.
 *
 * @param region NOTIFY capability mapping
 * @param region_len NOTIFY capability length in bytes
 * @param queue_notify_off device-supplied per-queue offset
 * @param multiplier device-supplied notification stride
 * @param queue_idx queue index to write
 *
 * @retval true the notification was written
 * @retval false the offset was rejected and no write was performed
 */
static inline bool virtio_pci_notify_write(volatile uint8_t *region, uint32_t region_len,
					   uint16_t queue_notify_off, uint32_t multiplier,
					   uint16_t queue_idx)
{
	size_t offset;

	if (!virtio_pci_notify_offset_valid(region_len, queue_notify_off, multiplier, &offset)) {
		return false;
	}

	*(volatile uint16_t *)(region + offset) = sys_cpu_to_le16(queue_idx);

	return true;
}

#endif /* ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_H_ */
