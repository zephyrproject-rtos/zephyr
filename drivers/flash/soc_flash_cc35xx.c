/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <ti/drivers/xmem/flash/FlashWFF3.h>
#include <ti/drivers/xmem/XMEMWFF3.h>
#include <driverlib/cpu.h>

#define DT_DRV_COMPAT           ti_cc35xx_nv_flash
#define CC35XX_ERASE_TIMEOUT    200
#define CC35XX_FLASH_WRITE_SIZE 256

struct flash_cc35xx_config {
	mem_addr_t base;
	mem_addr_t phys_addr;
	size_t size;
	size_t erase_size;
	const struct flash_pages_layout *layout;
	const struct flash_parameters *parameters;
};
static int flash_cc35xx_initialized;
static struct k_mutex flash_cc35xx_mutex;
static uint8_t flash_cc35xx_write_buf[CC35XX_FLASH_WRITE_SIZE] __aligned(sizeof(uint32_t));

static mem_addr_t flash_cc35xx_offset_to_phys_addr(const struct device *dev, off_t offset)
{
	const struct flash_cc35xx_config *config = dev->config;

	return config->phys_addr + offset;
}

static void *flash_cc35xx_offset_to_logic_addr(const struct device *dev, off_t offset)
{
	const struct flash_cc35xx_config *config = dev->config;
	uintptr_t addr = config->base + offset;

	return (void *)addr;
}

static bool flash_cc35xx_is_range_valid(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_cc35xx_config *config = dev->config;

	if ((offset < 0) || ((size_t)offset > config->size)) {
		return false;
	}

	return size <= config->size - (size_t)offset;
}

static int flash_cc35xx_get_udma_status(void)
{
	uint32_t status;

	status = XIPGetUDMAIrqStatus(XIP_UDMA_SECURE_CHANNEL);
	if (status != XIP_UDMA_JOB_IRQ_STATUS_DONE) {
		return -EIO;
	}

	return 0;
}

extern XMEMWFF3_HWAttrs XMEMWFF3_hwAttrs;
static const FlashType flash_cc35xx_is25wj032f = {
	/* Operations */
	.writeStigCfg.preStigCfg = 1,
	.writeStigCfg.postStigCfg = 0,
	.readStigCfg.preStigCfg = 0,
	.readStigCfg.postStigCfg = 0,
	.eraseStigCfg.preStigCfg = 1,
	.eraseStigCfg.postStigCfg = 0,
	.eraseStigCfg.StigCfg = 1,

	/* Enter STIG mode */
	.enterStigCfg[0].address = OSPI_REGS_BASE + OSPI_O_CONFIG,
	.enterStigCfg[0].data = 0x82080089,
	.enterStigCfg[1].address = OSPI_REGS_BASE + OSPI_O_DEV_INSTR_RD_CONFIG,
	.enterStigCfg[1].data = 0x0402220b,
	.enterStigCfg[2].address = OSPI_REGS_BASE + OSPI_O_DEV_INSTR_WR_CONFIG,
	.enterStigCfg[2].data = 0x00022002,

	/* EXIT STIG mode */
	.exitStigCfg[0].address = OSPI_REGS_BASE + OSPI_O_CONFIG,
	.exitStigCfg[0].data = 0x82080089,
	.exitStigCfg[1].address = OSPI_REGS_BASE + OSPI_O_DEV_INSTR_RD_CONFIG,
	.exitStigCfg[1].data = 0x0402220b,
	.exitStigCfg[2].address = OSPI_REGS_BASE + OSPI_O_DEV_INSTR_WR_CONFIG,
	.exitStigCfg[2].data = 0x00022002,

	/* Pre STIG configuration */
	.writeStigCfg.preStigOperation[0].address = OSPI_REGS_BASE + OSPI_O_FLASH_CMD_CTRL,
	.writeStigCfg.preStigOperation[0].data = 0x06000001,
	.eraseStigCfg.preStigOperation[0].address = OSPI_REGS_BASE + OSPI_O_FLASH_CMD_CTRL,
	.eraseStigCfg.preStigOperation[0].data = 0x06000001,

	/* Execute STIG operation */
	.readStigCfg.stigOperation[0].address = OSPI_REGS_BASE + OSPI_O_FLASH_CMD_CTRL,
	.readStigCfg.stigOperation[0].data = 0x0bba0200,
	.writeStigCfg.stigOperation[0].address = OSPI_REGS_BASE + OSPI_O_FLASH_CMD_CTRL,
	.writeStigCfg.stigOperation[0].data = 0x020ab000,
	.eraseStigCfg.stigOperation[0].address = OSPI_REGS_BASE + OSPI_O_FLASH_CMD_CTRL,
	.eraseStigCfg.stigOperation[0].data = 0x200a0000,

	/* Polling operation */
	.pollingCfg.command = 0x05900000,
	.pollingCfg.timeOut = CC35XX_ERASE_TIMEOUT,
	.pollingCfg.NumOfIteration = 4,

	/* General */
	.sectorSize = 0x1000,
	.verifyBufSize = 256,
};

static int flash_cc35xx_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (flash_cc35xx_initialized == 0) {
		k_mutex_init(&flash_cc35xx_mutex);
		flash_cc35xx_initialized = 1;
	}

	XMEMWFF3_hwAttrs.flashType = flash_cc35xx_is25wj032f;

	return 0;
}

__ramfunc static int flash_cc35xx_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_cc35xx_config *config = dev->config;
	size_t erase_size = config->erase_size;
	mem_addr_t addr = flash_cc35xx_offset_to_phys_addr(dev, offset);
	int ret = 0;
	unsigned int key;

	if (offset % erase_size) {
		return -EINVAL;
	}
	if (size % erase_size) {
		return -EINVAL;
	}
	if (!flash_cc35xx_is_range_valid(dev, offset, size)) {
		return -EINVAL;
	}

	k_mutex_lock(&flash_cc35xx_mutex, K_FOREVER);
	for (; size; size -= erase_size, addr += erase_size) {
		for (int tries = 3; tries; tries--) {
			key = irq_lock();
			ret = FlashSectorErase(addr, 0);
			irq_unlock(key);
			if (ret == 0) {
				break;
			}
		}
		if (ret) {
			break;
		}
	}
	k_mutex_unlock(&flash_cc35xx_mutex);

	return ret ? -ETIMEDOUT : 0;
}

static int flash_cc35xx_write(const struct device *dev, off_t offset, const void *buf, size_t size)
{
	const struct flash_cc35xx_config *config = dev->config;
	const uint8_t *src = buf;
	size_t remaining = size;
	int ret = 0;

	if (!size) {
		return 0;
	}

	if (size % config->parameters->write_block_size) {
		return -EINVAL;
	}

	if (!flash_cc35xx_is_range_valid(dev, offset, size)) {
		return -EINVAL;
	}

	if (offset % config->parameters->write_block_size) {
		return -EINVAL;
	}

	k_mutex_lock(&flash_cc35xx_mutex, K_FOREVER);
	while (remaining) {
		off_t chunk_offset = offset & ~(CC35XX_FLASH_WRITE_SIZE - 1);
		size_t chunk_pos = offset - chunk_offset;
		size_t chunk_len = MIN(remaining, CC35XX_FLASH_WRITE_SIZE - chunk_pos);
		uint8_t *addr = flash_cc35xx_offset_to_logic_addr(dev, chunk_offset);

		if (chunk_pos != 0 || chunk_len != sizeof(flash_cc35xx_write_buf)) {
			FlashRead((uint32_t *)addr, (uint32_t *)flash_cc35xx_write_buf,
				  sizeof(flash_cc35xx_write_buf));
			ret = flash_cc35xx_get_udma_status();
			if (ret) {
				break;
			}
		}
		memcpy(&flash_cc35xx_write_buf[chunk_pos], src, chunk_len);
		FlashWrite((uint32_t *)flash_cc35xx_write_buf, (uint32_t *)addr,
			   sizeof(flash_cc35xx_write_buf));
		ret = flash_cc35xx_get_udma_status();
		if (ret) {
			break;
		}

		src += chunk_len;
		offset += chunk_len;
		remaining -= chunk_len;
	}
	k_mutex_unlock(&flash_cc35xx_mutex);

	return ret;
}

static int flash_cc35xx_read(const struct device *dev, off_t offset, void *buf, size_t size)
{
	void *addr = flash_cc35xx_offset_to_logic_addr(dev, offset);

	if (!flash_cc35xx_is_range_valid(dev, offset, size)) {
		return -EINVAL;
	}

	k_mutex_lock(&flash_cc35xx_mutex, K_FOREVER);
	memcpy(buf, addr, size);
	k_mutex_unlock(&flash_cc35xx_mutex);

	return 0;
}

static const struct flash_parameters *flash_cc35xx_get_parameters(const struct device *dev)
{
	const struct flash_cc35xx_config *config = dev->config;

	return config->parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_cc35xx_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	const struct flash_cc35xx_config *config = dev->config;

	*layout = config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_cc35xx_api) = {
	.erase = flash_cc35xx_erase,
	.write = flash_cc35xx_write,
	.read = flash_cc35xx_read,
	.get_parameters = flash_cc35xx_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_cc35xx_layout,
#endif
};

#define FLASH_CC35XX_DEVICE(n)                                                                     \
	static const struct flash_pages_layout flash_cc35xx_layout_##n = {                         \
		.pages_size = DT_INST_PROP(n, erase_block_size),                                   \
		.pages_count = DT_INST_REG_SIZE_BY_IDX(n, 0) / DT_INST_PROP(n, erase_block_size),  \
	};                                                                                         \
	static const struct flash_parameters flash_cc35xx_parameters_##n = {                       \
		.write_block_size = DT_INST_PROP(n, write_block_size),                             \
		.erase_value = 0xff,                                                               \
	};                                                                                         \
	static const struct flash_cc35xx_config flash_cc35xx_config_##n = {                        \
		.base = DT_INST_REG_ADDR_BY_IDX(n, 0),                                             \
		.size = DT_INST_REG_SIZE_BY_IDX(n, 0),                                             \
		.phys_addr = DT_INST_REG_ADDR_BY_IDX(n, 1),                                        \
		.erase_size = DT_INST_PROP(n, erase_block_size),                                   \
		.layout = &flash_cc35xx_layout_##n,                                                \
		.parameters = &flash_cc35xx_parameters_##n,                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, flash_cc35xx_init, NULL, NULL, &flash_cc35xx_config_##n,          \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_cc35xx_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_CC35XX_DEVICE)
