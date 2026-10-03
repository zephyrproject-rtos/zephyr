/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "virtio_pci_write64.h"

#define TEST_VALUE UINT64_C(0x0123456789abcdef)

ZTEST(virtio_pci_write64_endianness, test_64_bit_value_is_stored_as_little_endian_dwords)
{
	uint64_t storage = 0U;
	const uint8_t *bytes = (const uint8_t *)&storage;

	virtio_pci_write64(TEST_VALUE, &storage);

	zassert_equal(sys_get_le32(bytes), UINT32_C(0x89abcdef));
	zassert_equal(sys_get_le32(bytes + sizeof(uint32_t)), UINT32_C(0x01234567));
	zassert_equal(sys_get_le64(bytes), TEST_VALUE);
}

ZTEST_SUITE(virtio_pci_write64_endianness, NULL, NULL, NULL, NULL, NULL);
