/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qemu_fwcfg.h"

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

static inline uint16_t sel_port(const struct device *dev)
{
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;

	return cfg->u.io.sel_port;
}

static inline uint16_t data_port(const struct device *dev)
{
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;

	return cfg->u.io.data_port;
}

static inline uint16_t dma_port(const struct device *dev)
{
	return sel_port(dev) + 4U;
}

static int fwcfg_ioport_select(const struct device *dev, uint16_t key)
{
	sys_out16(key, sel_port(dev));
	return 0;
}

static int fwcfg_ioport_read(const struct device *dev, void *dst, size_t len)
{
	uint8_t *p = (uint8_t *)dst;

	for (size_t i = 0; i < len; i++) {
		p[i] = sys_in8(data_port(dev));
	}

	return 0;
}

static int fwcfg_ioport_dma_kick(const struct device *dev, uint64_t descriptor_addr)
{
	sys_out32(sys_cpu_to_be32((uint32_t)(descriptor_addr >> 32)), dma_port(dev));
	sys_out32(sys_cpu_to_be32((uint32_t)descriptor_addr), dma_port(dev) + sizeof(uint32_t));

	return 0;
}

static const struct fwcfg_api fwcfg_ioport_api = {
	.select = fwcfg_ioport_select,
	.read = fwcfg_ioport_read,
	.dma_kick = fwcfg_ioport_dma_kick,
};

static int fwcfg_ioport_init(const struct device *dev)
{
	return fwcfg_init_common(dev);
}

#define FWCFG_IOPORT_DEVICE_DEFINE(node_id)                                                        \
	static struct fwcfg_data fwcfg_data_ioport_##node_id;                                      \
	static const struct fwcfg_config fwcfg_cfg_ioport_##node_id = {                            \
		.transport = FWCFG_IOPORT,                                                         \
		.u.io.sel_port = DT_REG_ADDR(node_id),                                             \
		.u.io.data_port = DT_REG_ADDR(node_id) + 1U,                                       \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, fwcfg_ioport_init, NULL, &fwcfg_data_ioport_##node_id,           \
			 &fwcfg_cfg_ioport_##node_id, POST_KERNEL,                                 \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fwcfg_ioport_api)

DT_FOREACH_STATUS_OKAY(qemu_fw_cfg_ioport, FWCFG_IOPORT_DEVICE_DEFINE);
