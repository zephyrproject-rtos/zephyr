/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_WRITE64_H_
#define ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_WRITE64_H_

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

/*
 * VirtIO PCI 4.1.3.1 requires 64-bit fields to be accessed as two
 * little-endian 32-bit words.
 */
static inline void virtio_pci_write64(uint64_t val, uint64_t *dst)
{
	((uint32_t *)dst)[0] = sys_cpu_to_le32((uint32_t)val);
	((uint32_t *)dst)[1] = sys_cpu_to_le32((uint32_t)(val >> 32));
}

#endif /* ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_WRITE64_H_ */
