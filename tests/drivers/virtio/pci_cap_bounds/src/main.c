/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define TEST_BAR_BASE 0x10000000U
#define TEST_BAR_SIZE 0x00001000U

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *mbar)
{
	ARG_UNUSED(bdf);
	zassert_equal(bar_index, 0U);

	mbar->phys_addr = TEST_BAR_BASE;
	mbar->size = TEST_BAR_SIZE;
	return true;
}

/* Compile the real capability mapping path without registering a device. */
#undef IRQ_CONNECT
#define IRQ_CONNECT(...)
#undef DEVICE_PCIE_INST_DECLARE
#define DEVICE_PCIE_INST_DECLARE(inst)
#undef DEVICE_PCIE_INST_INIT
#define DEVICE_PCIE_INST_INIT(inst, name) .name = NULL,
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_pci.c"

ZTEST(virtio_pci_cap_bounds, test_capability_past_bar_is_rejected)
{
	struct virtio_pci_cap cap = {
		.bar = 0U,
		.offset = sys_cpu_to_le32(0x0f00U),
		.length = sys_cpu_to_le32(0x0200U),
	};
	void *mapped = (void *)UINTPTR_MAX;

	zassert_false(virtio_pci_map_cap(PCIE_BDF(0, 0, 0), &cap, &mapped));
	zassert_equal((uintptr_t)mapped, UINTPTR_MAX);
}

ZTEST(virtio_pci_cap_bounds, test_capability_ending_at_bar_boundary_is_accepted)
{
	struct virtio_pci_cap cap = {
		.bar = 0U,
		.offset = sys_cpu_to_le32(0x0f00U),
		.length = sys_cpu_to_le32(0x0100U),
	};
	void *mapped = NULL;

	zassert_true(virtio_pci_map_cap(PCIE_BDF(0, 0, 0), &cap, &mapped));
	zassert_equal((uintptr_t)mapped, TEST_BAR_BASE + 0x0f00U);
}

ZTEST(virtio_pci_cap_bounds, test_large_offset_cannot_wrap_bounds_check)
{
	struct virtio_pci_cap cap = {
		.bar = 0U,
		.offset = sys_cpu_to_le32(UINT32_MAX - 0x0fU),
		.length = sys_cpu_to_le32(0x20U),
	};
	void *mapped = NULL;

	zassert_false(virtio_pci_map_cap(PCIE_BDF(0, 0, 0), &cap, &mapped));
}

ZTEST_SUITE(virtio_pci_cap_bounds, NULL, NULL, NULL, NULL, NULL);
