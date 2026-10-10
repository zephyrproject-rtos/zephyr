/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/ztest.h>

#include "virtio_mmio_status.h"

ZTEST(virtio_mmio_status_endianness, test_status_bits_stay_in_cpu_byte_order)
{
	uint32_t status = BIT(7);

	status = virtio_mmio_status_with_bit(status, 0);
	zassert_equal(status, BIT(7) | BIT(0));

	status = virtio_mmio_status_with_bit(status, 3);
	zassert_equal(status, BIT(7) | BIT(3) | BIT(0));
}

ZTEST_SUITE(virtio_mmio_status_endianness, NULL, NULL, NULL, NULL, NULL);
