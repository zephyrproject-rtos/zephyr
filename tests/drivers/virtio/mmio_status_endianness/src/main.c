/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_NODELABEL(virtio_mmio_test)
#define TEST_REG(offset) ((mem_addr_t)(DT_REG_ADDR(TEST_NODE) + (offset)))

#define TEST_VIRTIO_MAGIC     0x74726976U
#define TEST_VIRTIO_VERSION   2U
#define TEST_VIRTIO_DEVICE_ID 1U

static void test_reg_write(uint32_t offset, uint32_t value)
{
	sys_write32(sys_cpu_to_le32(value), TEST_REG(offset));
}

static uint32_t test_reg_read(uint32_t offset)
{
	return sys_le32_to_cpu(sys_read32(TEST_REG(offset)));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_reg_write(VIRTIO_MMIO_MAGIC_VALUE, TEST_VIRTIO_MAGIC);
	test_reg_write(VIRTIO_MMIO_VERSION, TEST_VIRTIO_VERSION);
	test_reg_write(VIRTIO_MMIO_DEVICE_ID, TEST_VIRTIO_DEVICE_ID);
	test_reg_write(VIRTIO_MMIO_VENDOR_ID, 0U);
	test_reg_write(VIRTIO_MMIO_STATUS, 0U);
}

ZTEST(virtio_mmio_status_endianness, test_status_round_trip_through_transport_api)
{
	const struct device *dev = DEVICE_DT_GET(TEST_NODE);
	uint32_t expected_status;
	int ret;

	zassert_false(device_is_ready(dev), "deferred VirtIO MMIO device unexpectedly ready");

	ret = device_init(dev);
	zassert_ok(ret, "fake VirtIO MMIO transport failed to initialize");
	zassert_true(device_is_ready(dev), "fake VirtIO MMIO transport did not become ready");

	expected_status = BIT(DEVICE_STATUS_ACKNOWLEDGE) | BIT(DEVICE_STATUS_DRIVER);
	zassert_equal(test_reg_read(VIRTIO_MMIO_STATUS), expected_status,
		      "initial MMIO status bits were not written in little-endian order");

	zassert_ok(virtio_commit_feature_bits(dev),
		   "FEATURES_OK status bit did not round-trip through the MMIO transport");
	expected_status |= BIT(DEVICE_STATUS_FEATURES_OK);
	zassert_equal(test_reg_read(VIRTIO_MMIO_STATUS), expected_status,
		      "FEATURES_OK status bit was not preserved in little-endian order");

	virtio_finalize_init(dev);
	expected_status |= BIT(DEVICE_STATUS_DRIVER_OK);
	zassert_equal(test_reg_read(VIRTIO_MMIO_STATUS), expected_status,
		      "DRIVER_OK status bit was not preserved in little-endian order");
}

ZTEST_SUITE(virtio_mmio_status_endianness, NULL, NULL, before, NULL, NULL);
