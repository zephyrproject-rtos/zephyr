/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_INTERNAL_H_
#define ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_INTERNAL_H_

#include <zephyr/irq.h>

static inline void virtio_pci_enable_irq_on_success(int init_ret, unsigned int irq)
{
	if (init_ret == 0) {
		irq_enable(irq);
	}
}

#endif /* ZEPHYR_DRIVERS_VIRTIO_VIRTIO_PCI_INTERNAL_H_ */
