/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

/* Compile the real MMIO queue initialization path without registering a device. */
#undef DEVICE_DT_INST_DEFINE
#define DEVICE_DT_INST_DEFINE(...)

#include "../../../../../drivers/virtio/virtio_mmio.c"

static uint32_t fake_regs[(VIRTIO_MMIO_QUEUE_USED_HIGH / sizeof(uint32_t)) + 1U];
static struct virtio_mmio_data fake_data;
static struct virtio_mmio_config fake_config = {
	.reg_base = {
		.addr = (mm_reg_t)fake_regs,
	},
};

static struct device fake_device = {
	.name = "virtio-mmio-zero-size-queue",
	.config = &fake_config,
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

	memset(fake_regs, 0, sizeof(fake_regs));
	memset(&fake_data, 0, sizeof(fake_data));
	fake_regs[VIRTIO_MMIO_QUEUE_SIZE_MAX / sizeof(uint32_t)] = sys_cpu_to_le32(8U);
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

ZTEST(virtio_mmio_zero_size_queue, test_zero_size_queue_is_not_ready)
{
	zassert_ok(virtio_mmio_set_virtqueues(&fake_device, 1U, skip_queue, NULL));
	zassert_equal(fake_data.virtqueues[0].num, 0U);
	zassert_equal(sys_le32_to_cpu(fake_regs[VIRTIO_MMIO_QUEUE_READY / sizeof(uint32_t)]), 0U);
	zassert_equal(sys_le32_to_cpu(fake_regs[VIRTIO_MMIO_QUEUE_SIZE / sizeof(uint32_t)]), 0U);
}

ZTEST_SUITE(virtio_mmio_zero_size_queue, NULL, NULL, before, after, NULL);
