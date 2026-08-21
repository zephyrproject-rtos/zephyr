/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

/* Compile the real MMIO status helpers without registering a device. */
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_mmio.c"

static uint32_t fake_regs[(VIRTIO_MMIO_STATUS / sizeof(uint32_t)) + 1U];
static struct virtio_mmio_data fake_data;
static struct virtio_mmio_config fake_config = {
	.reg_base = {
		.addr = (mm_reg_t)fake_regs,
	},
};

static struct device fake_device = {
	.name = "virtio-mmio-status-endianness",
	.config = &fake_config,
	.data = &fake_data,
};

ZTEST(virtio_mmio_status_endianness, test_status_bit_uses_cpu_endian_mask)
{
	const uint32_t existing = BIT(DEVICE_STATUS_DRIVER);
	const uint32_t expected = existing | BIT(DEVICE_STATUS_FEATURES_OK);

	fake_regs[VIRTIO_MMIO_STATUS / sizeof(uint32_t)] = sys_cpu_to_le32(existing);

	virtio_mmio_write_status_bit(&fake_device, DEVICE_STATUS_FEATURES_OK);

	zassert_equal(sys_le32_to_cpu(fake_regs[VIRTIO_MMIO_STATUS / sizeof(uint32_t)]),
		      expected);
}

ZTEST_SUITE(virtio_mmio_status_endianness, NULL, NULL, NULL, NULL, NULL);
