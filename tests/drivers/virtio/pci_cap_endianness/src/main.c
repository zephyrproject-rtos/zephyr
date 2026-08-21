/*
 * SPDX-FileCopyrightText: 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define TEST_BAR_BASE 0x10000000U
#define TEST_BAR_SIZE 0x00200000U
#define TEST_CAP_OFFSET 0x00100000U
#define TEST_CAP_LENGTH 0x00010000U

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

ZTEST(virtio_pci_cap_endianness, test_capability_offset_and_length_are_little_endian)
{
	struct virtio_pci_cap cap = {
		.bar = 0U,
		.offset = sys_cpu_to_le32(TEST_CAP_OFFSET),
		.length = sys_cpu_to_le32(TEST_CAP_LENGTH),
	};
	void *mapped = NULL;

	zassert_true(virtio_pci_map_cap(PCIE_BDF(0, 0, 0), &cap, &mapped));
	zassert_equal((uintptr_t)mapped, TEST_BAR_BASE + TEST_CAP_OFFSET,
		      "VirtIO PCI capability offset was not converted from little-endian");
}

ZTEST_SUITE(virtio_pci_cap_endianness, NULL, NULL, NULL, NULL, NULL);
