/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTIO_VIRTIO_MMIO_STATUS_H_
#define ZEPHYR_DRIVERS_VIRTIO_VIRTIO_MMIO_STATUS_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

static inline uint32_t virtio_mmio_status_with_bit(uint32_t status, int bit)
{
	return status | BIT(bit);
}

#endif /* ZEPHYR_DRIVERS_VIRTIO_VIRTIO_MMIO_STATUS_H_ */
