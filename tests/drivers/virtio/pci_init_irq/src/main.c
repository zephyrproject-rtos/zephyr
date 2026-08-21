/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static unsigned int virtio_pci_irq_enable_count;

static void test_irq_enable(unsigned int irq)
{
	ARG_UNUSED(irq);
	virtio_pci_irq_enable_count++;
}

/*
 * Compile the real transport init path into this test without registering its
 * devicetree device. Calling it directly lets a failed init return to ZTest
 * instead of passing the driver's positive error through the device framework.
 */
#undef irq_enable
#define irq_enable(irq) test_irq_enable(irq)
#undef IRQ_CONNECT
#define IRQ_CONNECT(...)
#undef DEVICE_PCIE_INST_DECLARE
#define DEVICE_PCIE_INST_DECLARE(inst)
#undef DEVICE_PCIE_INST_INIT
#define DEVICE_PCIE_INST_INIT(inst, name) .name = NULL,
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_pci.c"

static struct pcie_dev fake_pcie = {
	.bdf = PCIE_BDF_NONE,
	.id = PCIE_ID(0x1af4, 0x1052),
};

static struct virtio_pci_config fake_config = {
	.pcie = &fake_pcie,
};

static struct virtio_pci_data fake_data;
static struct device fake_device = {
	.name = "virtio-pci-failed-init",
	.config = &fake_config,
	.data = &fake_data,
};

ZTEST(virtio_pci_init_irq, test_failed_init_does_not_enable_irq)
{
	virtio_pci_irq_enable_count = 0U;

	zassert_not_equal(virtio_pci_init0(&fake_device), 0,
			  "VirtIO PCI transport unexpectedly initialized");
	zassert_equal(virtio_pci_irq_enable_count, 0U,
		      "VirtIO PCI IRQ enabled despite failed transport initialization");
}

ZTEST_SUITE(virtio_pci_init_irq, NULL, NULL, NULL, NULL, NULL);
