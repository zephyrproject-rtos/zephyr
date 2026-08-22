/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *mbar)
{
	ARG_UNUSED(bdf);
	ARG_UNUSED(bar_index);
	ARG_UNUSED(mbar);
	return false;
}

/* Compile the real PCI queue initialization path without registering a device. */
#undef IRQ_CONNECT
#define IRQ_CONNECT(...)
#undef DEVICE_PCIE_INST_DECLARE
#define DEVICE_PCIE_INST_DECLARE(inst)
#undef DEVICE_PCIE_INST_INIT
#define DEVICE_PCIE_INST_INIT(inst, name) .name = NULL,
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_pci.c"

static struct virtio_pci_common_cfg fake_common_cfg;
static struct virtio_pci_data fake_data;
static struct device fake_device = {
	.name = "virtio-pci-zero-size-queue",
	.data = &fake_data,
};

static uint16_t skip_queue(uint16_t queue_idx, uint16_t queue_size_max, void *opaque)
{
	ARG_UNUSED(opaque);
	zassert_equal(queue_idx, 0U);
	zassert_equal(queue_size_max, 8U);
	return 0U;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&fake_common_cfg, 0, sizeof(fake_common_cfg));
	memset(&fake_data, 0, sizeof(fake_data));
	fake_common_cfg.num_queues = sys_cpu_to_le16(1U);
	fake_common_cfg.queue_size = sys_cpu_to_le16(8U);
	fake_data.common_cfg = &fake_common_cfg;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (fake_data.virtqueues != NULL) {
		virtq_free(&fake_data.virtqueues[0]);
		k_free(fake_data.virtqueues);
		fake_data.virtqueues = NULL;
	}
}

ZTEST(virtio_pci_zero_size_queue, test_zero_size_queue_is_not_enabled)
{
	zassert_ok(virtio_pci_init_virtqueues(&fake_device, 1U, skip_queue, NULL));
	zassert_equal(fake_data.virtqueues[0].num, 0U);
	zassert_equal(sys_le16_to_cpu(fake_common_cfg.queue_enable), 0U);
	zassert_equal(sys_le16_to_cpu(fake_common_cfg.queue_size), 8U);
}

ZTEST_SUITE(virtio_pci_zero_size_queue, NULL, NULL, before, after, NULL);
