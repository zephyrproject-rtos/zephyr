/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qemu_fwcfg.h"

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

#define FWCFG_MMIO_DATA_OFF 0x00
#define FWCFG_MMIO_SEL_OFF  0x08
#define FWCFG_MMIO_DMA_OFF  0x10

static inline uintptr_t mmio_base(const struct device *dev)
{
	return DEVICE_MMIO_GET(dev);
}

static int fwcfg_mmio_select(const struct device *dev, uint16_t key)
{
	uintptr_t base;

	base = mmio_base(dev);
	sys_write16(sys_cpu_to_be16(key), base + FWCFG_MMIO_SEL_OFF);
	return 0;
}

static int fwcfg_mmio_read(const struct device *dev, void *dst, size_t len)
{
	uint8_t *p = (uint8_t *)dst;
	uintptr_t base;

	base = mmio_base(dev);

	for (size_t i = 0; i < len; i++) {
		p[i] = sys_read8(base + FWCFG_MMIO_DATA_OFF);
	}

	return 0;
}

static int fwcfg_mmio_dma_kick(const struct device *dev, uint64_t descriptor_addr)
{
	uintptr_t base;

	base = mmio_base(dev);
	sys_write32(sys_cpu_to_be32((uint32_t)(descriptor_addr >> 32)), base + FWCFG_MMIO_DMA_OFF);
	sys_write32(sys_cpu_to_be32((uint32_t)descriptor_addr),
		    base + FWCFG_MMIO_DMA_OFF + sizeof(uint32_t));

	return 0;
}

static const struct fwcfg_api fwcfg_mmio_api = {
	.select = fwcfg_mmio_select,
	.read = fwcfg_mmio_read,
	.dma_kick = fwcfg_mmio_dma_kick,
};

static int fwcfg_mmio_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return fwcfg_init_common(dev);
}

#define FWCFG_MMIO_DEVICE_DEFINE(node_id)                                                          \
	static struct fwcfg_data fwcfg_data_mmio_##node_id;                                        \
	static const struct fwcfg_config fwcfg_cfg_mmio_##node_id = {                              \
		DEVICE_MMIO_ROM_INIT(node_id),                                                     \
		.transport = FWCFG_MMIO,                                                           \
		.u.mmio.base = DT_REG_ADDR(node_id),                                               \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, fwcfg_mmio_init, NULL, &fwcfg_data_mmio_##node_id,               \
			 &fwcfg_cfg_mmio_##node_id, POST_KERNEL,                                   \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fwcfg_mmio_api)

DT_FOREACH_STATUS_OKAY(qemu_fw_cfg_mmio, FWCFG_MMIO_DEVICE_DEFINE);
