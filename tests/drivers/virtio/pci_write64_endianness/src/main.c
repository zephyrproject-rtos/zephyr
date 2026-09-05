/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/ztest.h>

/* Compile the real 64-bit PCI configuration write helper without registering a device. */
#undef IRQ_CONNECT
#define IRQ_CONNECT(...)
#undef DEVICE_PCIE_INST_DECLARE
#define DEVICE_PCIE_INST_DECLARE(inst)
#undef DEVICE_PCIE_INST_INIT
#define DEVICE_PCIE_INST_INIT(inst, name) .name = NULL,
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_pci.c"

ZTEST(virtio_pci_write64_endianness, test_write64_stores_low_dword_first_little_endian)
{
	union {
		uint64_t value;
		uint8_t bytes[sizeof(uint64_t)];
	} storage = {0};
	const uint8_t expected[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

	virtio_pci_write64(UINT64_C(0x1122334455667788), &storage.value);

	zassert_mem_equal(storage.bytes, expected, sizeof(expected));
}

ZTEST_SUITE(virtio_pci_write64_endianness, NULL, NULL, NULL, NULL, NULL);
