/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_NODELABEL(virtio_pci_test)

static unsigned int irq_enable_count;

void __wrap_arch_irq_enable(unsigned int irq)
{
	ARG_UNUSED(irq);
	irq_enable_count++;
}

ZTEST(virtio_pci_init_irq, test_failed_init_does_not_enable_irq)
{
	const struct device *dev = DEVICE_DT_GET(TEST_NODE);
	int ret;

	irq_enable_count = 0U;
	zassert_false(device_is_ready(dev), "deferred VirtIO PCI device unexpectedly ready");
	zassert_not_null(dev->ops.init, "VirtIO PCI device has no init callback");

	ret = dev->ops.init(dev);

	zassert_not_equal(ret, 0, "VirtIO PCI transport unexpectedly initialized");
	zassert_equal(irq_enable_count, 0U,
		      "VirtIO PCI IRQ enabled despite failed transport initialization");
}

ZTEST_SUITE(virtio_pci_init_irq, NULL, NULL, NULL, NULL, NULL);
