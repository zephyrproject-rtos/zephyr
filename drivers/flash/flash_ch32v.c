/*
 * SPDX-FileCopyrightText: Copyright Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <hal_ch32fun.h>
#include <string.h>

#include "flash_priv.h"

#define DT_DRV_COMPAT wch_ch32v_flash_controller

/* ch32fun uses different names for the 003 vs 00x vs 20x/30x */
#ifndef FLASH_CTLR_PAGE_FTPG
#define FLASH_CTLR_PAGE_FTPG FLASH_CTLR_PAGE_PG
#endif
#ifndef FLASH_CTLR_PAGE_FTER
#define FLASH_CTLR_PAGE_FTER FLASH_CTLR_PAGE_ER
#endif
#ifndef FLASH_CTLR_FLOCK
#define FLASH_CTLR_FLOCK FLASH_CTLR_FAST_LOCK
#endif
#ifndef FLASH_STATR_WRBSY
#define FLASH_STATR_WRBSY BIT(1)
#endif

struct flash_ch32v_config {
	FLASH_TypeDef *regs;
	uint8_t *base_addr;
	size_t size;
	uint8_t erase_block_shift;
	uint8_t write_block_shift;
	struct flash_parameters params;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

static void flash_ch32v_unlock(FLASH_TypeDef *regs)
{
	if ((regs->CTLR & FLASH_CTLR_LOCK) != 0) {
		regs->KEYR = FLASH_KEY1;
		regs->KEYR = FLASH_KEY2;
	}
	if ((regs->CTLR & FLASH_CTLR_FLOCK) != 0) {
		regs->MODEKEYR = FLASH_KEY1;
		regs->MODEKEYR = FLASH_KEY2;
	}
}

static void flash_ch32v_lock(FLASH_TypeDef *regs)
{
	regs->CTLR |= FLASH_CTLR_FLOCK | FLASH_CTLR_LOCK;
}

static int flash_ch32v_wait_busy(FLASH_TypeDef *regs)
{
	while ((regs->STATR & FLASH_STATR_BSY) != 0) {
	}

	if ((regs->STATR & FLASH_STATR_EOP) != 0) {
		regs->STATR = FLASH_STATR_EOP;
	}

	if ((regs->STATR & FLASH_STATR_WRPRTERR) != 0) {
		regs->STATR = FLASH_STATR_WRPRTERR;
		return -EIO;
	}

	return 0;
}

static int flash_ch32v_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_ch32v_config *config = dev->config;

	if (len == 0) {
		return 0;
	}

	if (data == NULL || offset < 0 || offset + len > config->size) {
		return -EINVAL;
	}

	memcpy(data, config->base_addr + offset, len);

	return 0;
}

static int flash_ch32v_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct flash_ch32v_config *config = dev->config;
	FLASH_TypeDef *regs = config->regs;
	const uint32_t *src = data;
	uint32_t write_block_size = 1U << config->write_block_shift;
	int rc;

	if (len == 0) {
		return 0;
	}

	if (data == NULL || offset < 0 || offset + len > config->size) {
		return -EINVAL;
	}

	/* Offset and length must be aligned to the page size */
	if ((offset & (write_block_size - 1)) != 0 || (len & (write_block_size - 1)) != 0) {
		return -EINVAL;
	}

	flash_ch32v_unlock(regs);

	rc = flash_ch32v_wait_busy(regs);
	if (rc < 0) {
		goto out;
	}

	/* Enable fast programming mode */
	regs->CTLR |= FLASH_CTLR_PAGE_FTPG;

	/*
	 * The CH32V003 and CH32V00x use an explicitly managed buffer while the CH32V20x/30x use an
	 * internal buffer. The following code uses `BUF_LOAD` to detect which method to use.
	 */
	for (size_t p = 0; p < len; p += write_block_size) {
		uint8_t *page_addr = config->base_addr + offset + p;
#ifdef FLASH_CTLR_BUF_LOAD
		/* Clear the internal buffer */
		regs->CTLR |= FLASH_CTLR_BUF_RST;
		rc = flash_ch32v_wait_busy(regs);
		if (rc < 0) {
			goto out;
		}
#endif
		for (int i = 0; i < write_block_size; i += sizeof(uint32_t)) {
			*(volatile uint32_t *)(page_addr + i) = src[(p + i) / 4];
#ifdef FLASH_CTLR_BUF_LOAD
			/* Load the write into the buffer */
			regs->CTLR |= FLASH_CTLR_BUF_LOAD;
			rc = flash_ch32v_wait_busy(regs);
			if (rc < 0) {
				goto out;
			}
#else
			/* Wait for the write to finish loading into the buffer */
			while ((regs->STATR & FLASH_STATR_WRBSY) != 0) {
			}
#endif
		}

#ifdef FLASH_CTLR_BUF_LOAD
		regs->ADDR = (uint32_t)page_addr;
		regs->CTLR |= FLASH_CTLR_STRT;
#else
		regs->CTLR |= FLASH_CTLR_PG_STRT;
#endif
		rc = flash_ch32v_wait_busy(regs);
		if (rc < 0) {
			goto out;
		}
	}

out:
	regs->CTLR &= ~FLASH_CTLR_PAGE_FTPG;
	flash_ch32v_lock(regs);
	return rc;
}

static int flash_ch32v_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_ch32v_config *config = dev->config;
	FLASH_TypeDef *regs = config->regs;
	uint32_t erase_block_size = 1U << config->erase_block_shift;
	int rc;

	if (size == 0) {
		return 0;
	}

	if (offset < 0 || offset + size > config->size) {
		return -EINVAL;
	}

	/* Offset and size must be aligned to the erase block size */
	if ((offset & (erase_block_size - 1)) != 0 || (size & (erase_block_size - 1)) != 0) {
		return -EINVAL;
	}

	flash_ch32v_unlock(regs);

	rc = flash_ch32v_wait_busy(regs);
	if (rc < 0) {
		goto out;
	}

	/* Enable fast erase mode */
	regs->CTLR |= FLASH_CTLR_PAGE_FTER;

	for (off_t addr = offset; addr < offset + size; addr += erase_block_size) {
		regs->ADDR = (uint32_t)(config->base_addr + addr);
		regs->CTLR |= FLASH_CTLR_STRT;

		rc = flash_ch32v_wait_busy(regs);
		regs->CTLR &= ~FLASH_CTLR_PER;
		if (rc < 0) {
			goto out;
		}
	}

out:
	regs->CTLR &= ~FLASH_CTLR_PAGE_FTER;
	flash_ch32v_lock(regs);
	return rc;
}

static const struct flash_parameters *flash_ch32v_get_parameters(const struct device *dev)
{
	const struct flash_ch32v_config *config = dev->config;

	return &config->params;
}

static int flash_ch32v_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_ch32v_config *config = dev->config;

	*size = config->size;
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_ch32v_page_layout(const struct device *dev,
				    const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct flash_ch32v_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_ch32v_api) = {
	.read = flash_ch32v_read,
	.write = flash_ch32v_write,
	.erase = flash_ch32v_erase,
	.get_parameters = flash_ch32v_get_parameters,
	.get_size = flash_ch32v_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_ch32v_page_layout,
#endif
};

static int flash_ch32v_init(const struct device *dev)
{
	const struct flash_ch32v_config *config = dev->config;
	FLASH_TypeDef *regs = config->regs;

	flash_ch32v_lock(regs);

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define FLASH_CH32V_INIT_LAYOUT(idx)                                                               \
	.layout = {                                                                                \
		.pages_count = DT_REG_SIZE(SOC_NV_FLASH_CHILD_NODE(idx)) /                         \
			       DT_PROP(SOC_NV_FLASH_CHILD_NODE(idx), erase_block_size),            \
		.pages_size = DT_PROP(SOC_NV_FLASH_CHILD_NODE(idx), erase_block_size),             \
	},
#else
#define FLASH_CH32V_INIT_LAYOUT(idx)
#endif

#define FLASH_CH32V_INIT(idx)                                                                      \
	static const struct flash_ch32v_config flash_ch32v_##idx##_config = {                      \
		.regs = (FLASH_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.base_addr = (uint8_t *)DT_REG_ADDR(DT_INST(idx, soc_nv_flash)),                   \
		.size = DT_REG_SIZE(DT_INST(idx, soc_nv_flash)),                                   \
		.erase_block_shift =                                                               \
			LOG2(DT_PROP(SOC_NV_FLASH_CHILD_NODE(idx), erase_block_size)),             \
		.write_block_shift =                                                               \
			LOG2(DT_PROP(SOC_NV_FLASH_CHILD_NODE(idx), write_block_size)),             \
		.params =                                                                          \
			{                                                                          \
				.write_block_size =                                                \
					DT_PROP(SOC_NV_FLASH_CHILD_NODE(idx), write_block_size),   \
				.erase_value = (uint8_t)DT_INST_PROP(idx, erase_value),            \
			},                                                                         \
		FLASH_CH32V_INIT_LAYOUT(idx)};                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, flash_ch32v_init, NULL, NULL, &flash_ch32v_##idx##_config,      \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_ch32v_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_CH32V_INIT)
