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
#define TEST_QUEUE_MAX_SIZE   8U

static void test_reg_write(uint32_t offset, uint32_t value)
{
	sys_write32(sys_cpu_to_le32(value), TEST_REG(offset));
}

static uint32_t test_reg_read(uint32_t offset)
{
	return sys_le32_to_cpu(sys_read32(TEST_REG(offset)));
}

static uint16_t skip_queue(uint16_t queue_idx, uint16_t max_queue_size, void *opaque)
{
	ARG_UNUSED(queue_idx);
	ARG_UNUSED(opaque);

	zassert_equal(max_queue_size, TEST_QUEUE_MAX_SIZE,
		      "MMIO transport did not expose the advertised queue size");
	return 0U;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_reg_write(VIRTIO_MMIO_MAGIC_VALUE, TEST_VIRTIO_MAGIC);
	test_reg_write(VIRTIO_MMIO_VERSION, TEST_VIRTIO_VERSION);
	test_reg_write(VIRTIO_MMIO_DEVICE_ID, TEST_VIRTIO_DEVICE_ID);
	test_reg_write(VIRTIO_MMIO_VENDOR_ID, 0U);
	test_reg_write(VIRTIO_MMIO_STATUS, 0U);
	test_reg_write(VIRTIO_MMIO_QUEUE_SEL, 0U);
	test_reg_write(VIRTIO_MMIO_QUEUE_SIZE_MAX, TEST_QUEUE_MAX_SIZE);
	test_reg_write(VIRTIO_MMIO_QUEUE_SIZE, 0U);
	test_reg_write(VIRTIO_MMIO_QUEUE_READY, 0U);
}

ZTEST(virtio_zero_size_queue_mmio, test_zero_size_queue_is_not_activated)
{
	const struct device *dev = DEVICE_DT_GET(TEST_NODE);
	int ret;

	zassert_false(device_is_ready(dev), "deferred VirtIO MMIO device unexpectedly ready");

	ret = device_init(dev);
	zassert_ok(ret, "fake VirtIO MMIO transport failed to initialize");
	zassert_true(device_is_ready(dev), "fake VirtIO MMIO transport did not become ready");

	ret = virtio_init_virtqueues(dev, 1U, skip_queue, NULL);
	zassert_ok(ret, "zero-sized MMIO queue enumeration failed");
	zassert_equal(test_reg_read(VIRTIO_MMIO_QUEUE_SIZE), 0U,
		      "zero-sized MMIO queue was programmed into the transport");
	zassert_equal(test_reg_read(VIRTIO_MMIO_QUEUE_READY), 0U,
		      "zero-sized MMIO queue was activated");
}

ZTEST_SUITE(virtio_zero_size_queue_mmio, NULL, NULL, before, NULL, NULL);
