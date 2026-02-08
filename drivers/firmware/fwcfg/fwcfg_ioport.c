/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fwcfg.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

#define FWCFG_DMA_CTL_ERROR    BIT(0)
#define FWCFG_DMA_CTL_READ     BIT(1)
#define FWCFG_DMA_CTL_SKIP     BIT(2)
#define FWCFG_DMA_CTL_SELECT   BIT(3)
#define FWCFG_DMA_CTL_WRITE    BIT(4)
#define FWCFG_DMA_SELECT_SHIFT 16

struct fwcfg_dma_access {
	uint32_t control;
	uint32_t length;
	uint64_t address;
} __packed __aligned(8);

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
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;

	__ASSERT(cfg != NULL, "fwcfg config is NULL");

	if (cfg->transport != FWCFG_IOPORT) {
		return -EFAULT;
	}

	sys_out16(key, sel_port(dev));
	return 0;
}

static int fwcfg_ioport_read(const struct device *dev, uint8_t *dst, size_t len)
{
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;

	__ASSERT(cfg != NULL, "fwcfg config is NULL");
	if ((dst == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if (cfg->transport != FWCFG_IOPORT) {
		return -EFAULT;
	}

	for (size_t i = 0; i < len; i++) {
		dst[i] = sys_in8(data_port(dev));
	}

	return 0;
}

static int fwcfg_ioport_read_dma(const struct device *dev, uint16_t key, uint8_t *dst, size_t len)
{
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;
	volatile struct fwcfg_dma_access access;
	uint64_t descriptor_addr;
	uint32_t control;

	__ASSERT(cfg != NULL, "fwcfg config is NULL");
	if ((dst == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}
	if (cfg->transport != FWCFG_IOPORT) {
		return -EFAULT;
	}
	if (len > UINT32_MAX) {
		return -EINVAL;
	}

	control = FWCFG_DMA_CTL_SELECT | FWCFG_DMA_CTL_READ |
		  ((uint32_t)key << FWCFG_DMA_SELECT_SHIFT);
	access.control = sys_cpu_to_be32(control);
	access.length = sys_cpu_to_be32((uint32_t)len);
	access.address = sys_cpu_to_be64((uint64_t)(uintptr_t)dst);

	descriptor_addr = (uint64_t)(uintptr_t)&access;
	sys_out32(sys_cpu_to_be32((uint32_t)(descriptor_addr >> 32)), dma_port(dev));
	sys_out32(sys_cpu_to_be32((uint32_t)descriptor_addr), dma_port(dev) + sizeof(uint32_t));

	control = sys_be32_to_cpu(access.control);
	for (int i = 0;
	     control != 0U && control != FWCFG_DMA_CTL_ERROR && i < FWCFG_DMA_POLL_MAX_ITER; i++) {
		if (FWCFG_DMA_POLL_WAIT_US > 0) {
			k_busy_wait(FWCFG_DMA_POLL_WAIT_US);
		}
		control = sys_be32_to_cpu(access.control);
	}

	if (control == FWCFG_DMA_CTL_ERROR) {
		return -EIO;
	}
	if (control != 0U) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int fwcfg_ioport_write_dma(const struct device *dev, uint16_t key, const uint8_t *src,
				  size_t len)
{
	const struct fwcfg_config *cfg = (const struct fwcfg_config *)dev->config;
	volatile struct fwcfg_dma_access access;
	uint64_t descriptor_addr;
	uint32_t control;

	__ASSERT(cfg != NULL, "fwcfg config is NULL");
	if ((src == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}
	if (cfg->transport != FWCFG_IOPORT) {
		return -EFAULT;
	}
	if (len > UINT32_MAX) {
		return -EINVAL;
	}

	control = FWCFG_DMA_CTL_SELECT | FWCFG_DMA_CTL_WRITE |
		  ((uint32_t)key << FWCFG_DMA_SELECT_SHIFT);
	access.control = sys_cpu_to_be32(control);
	access.length = sys_cpu_to_be32((uint32_t)len);
	access.address = sys_cpu_to_be64((uint64_t)(uintptr_t)src);

	descriptor_addr = (uint64_t)(uintptr_t)&access;
	sys_out32(sys_cpu_to_be32((uint32_t)(descriptor_addr >> 32)), dma_port(dev));
	sys_out32(sys_cpu_to_be32((uint32_t)descriptor_addr), dma_port(dev) + sizeof(uint32_t));

	control = sys_be32_to_cpu(access.control);
	for (int i = 0;
	     control != 0U && control != FWCFG_DMA_CTL_ERROR && i < FWCFG_DMA_POLL_MAX_ITER; i++) {
		if (FWCFG_DMA_POLL_WAIT_US > 0) {
			k_busy_wait(FWCFG_DMA_POLL_WAIT_US);
		}
		control = sys_be32_to_cpu(access.control);
	}

	if (control == FWCFG_DMA_CTL_ERROR) {
		return -EIO;
	}
	if (control != 0U) {
		return -ETIMEDOUT;
	}

	return 0;
}

const struct fwcfg_ops fwcfg_ioport_ops = {
	.select = fwcfg_ioport_select,
	.read = fwcfg_ioport_read,
	.read_dma = fwcfg_ioport_read_dma,
	.write_dma = fwcfg_ioport_write_dma,
};
