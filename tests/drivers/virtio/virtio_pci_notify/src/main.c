/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "virtio_pci.h"

#define GUARD_LEN  16U
#define REGION_LEN 8U
#define CANARY     0xA5U

static uint8_t backing[GUARD_LEN + REGION_LEN + GUARD_LEN];
static uint8_t *const region = &backing[GUARD_LEN];

static void reset_region(void)
{
	memset(backing, CANARY, sizeof(backing));
}

static void assert_untouched(void)
{
	for (size_t i = 0U; i < sizeof(backing); i++) {
		zassert_equal(backing[i], CANARY, "unexpected write at byte %zu", i);
	}
}

static void assert_only_written(size_t offset, uint16_t value)
{
	uint8_t expected[sizeof(uint16_t)];

	sys_put_le16(value, expected);

	for (size_t i = 0U; i < sizeof(backing); i++) {
		if (i >= GUARD_LEN + offset && i < GUARD_LEN + offset + sizeof(expected)) {
			zassert_equal(backing[i], expected[i - GUARD_LEN - offset],
				      "unexpected byte at offset %zu", i);
		} else {
			zassert_equal(backing[i], CANARY, "unexpected write at byte %zu", i);
		}
	}
}

ZTEST(virtio_pci_notify, test_valid_offset_writes_queue_index)
{
	reset_region();

	zassert_true(virtio_pci_notify_write(region, REGION_LEN, 3U, 2U, 0x1234U));
	assert_only_written(6U, 0x1234U);
}

ZTEST(virtio_pci_notify, test_last_two_bytes_are_valid)
{
	reset_region();

	zassert_true(virtio_pci_notify_write(region, REGION_LEN, 6U, 1U, 0x2345U));
	assert_only_written(6U, 0x2345U);
}

ZTEST(virtio_pci_notify, test_one_byte_remaining_is_rejected)
{
	reset_region();

	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 7U, 1U, 0x3456U));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_offset_at_region_end_is_rejected)
{
	reset_region();

	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 8U, 1U, 0x4567U));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_offset_past_region_end_is_rejected)
{
	reset_region();

	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 9U, 1U, 0x5678U));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_region_shorter_than_access_is_rejected)
{
	reset_region();

	zassert_false(virtio_pci_notify_write(region, 0U, 0U, 0U, 0x6789U));
	zassert_false(virtio_pci_notify_write(region, 1U, 0U, 0U, 0x6789U));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_zero_multiplier_is_valid)
{
	reset_region();

	zassert_true(virtio_pci_notify_write(region, sizeof(uint16_t), UINT16_MAX, 0U, 0x789AU));
	assert_only_written(0U, 0x789AU);
}

ZTEST(virtio_pci_notify, test_multiplication_overflow_is_rejected)
{
	reset_region();

	/* The product is larger than the 32-bit NOTIFY region length. */
	zassert_false(virtio_pci_notify_write(region, REGION_LEN, UINT16_MAX, UINT32_MAX, 0x89ABU));
	/* The product is also larger than a 32-bit size_t. */
	zassert_false(virtio_pci_notify_write(region, REGION_LEN, UINT16_MAX, 0x10002U, 0x89ABU));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_device_changes_offset_after_first_notification)
{
	reset_region();

	zassert_true(virtio_pci_notify_write(region, REGION_LEN, 0U, 2U, 0x9ABCU));
	assert_only_written(0U, 0x9ABCU);

	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 4U, 2U, 0xABCDU));
	assert_only_written(0U, 0x9ABCU);
}

ZTEST(virtio_pci_notify, test_32bit_product_wrap_is_rejected)
{
	reset_region();

	/*
	 * 0x8000 * 0x20000 is 2^32, which wraps to zero in 32-bit
	 * arithmetic. The validation must use the 64-bit product.
	 */
	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 0x8000U, 0x20000U, 0x1111U));
	assert_untouched();
}

ZTEST(virtio_pci_notify, test_offset_plus_two_wrap_is_rejected)
{
	reset_region();

	/*
	 * 0xffff * 0x10001 is 0xffffffff. Adding the two-byte access
	 * width wraps to one in 32-bit arithmetic.
	 */
	zassert_false(virtio_pci_notify_write(region, REGION_LEN, 0xffffU, 0x10001U, 0x2222U));
	assert_untouched();
}

ZTEST_SUITE(virtio_pci_notify, NULL, NULL, NULL, NULL, NULL);
