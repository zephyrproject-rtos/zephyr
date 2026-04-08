/*
 * Copyright (c) 2025-2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_flash_controller

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <reg/xbi.h>
#include <reg/efp.h>
#ifdef CONFIG_FLASH_EX_OP_ENABLED
#include <zephyr/drivers/flash/ene_flash_api_ex.h>
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_ene_kb106x, LOG_LEVEL_ERR);

#define FLASH_ENE_TIMEOUT_US 100000 /* 100ms */

#define SOC_NV_FLASH_NODE DT_NODELABEL(flash0)
#define SOC_NV_FLASH_ADDR DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_WRITE_BLOCK_SIZE 1
#define FLASH_ERASE_BLOCK_SIZE 0x200

static const struct flash_parameters flash_kb106x_parameters = {
	.write_block_size = FLASH_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
};

/* Device config */
struct flash_kb106x_config {
	struct xbi_regs *base_addr;
	struct efp_regs *efp_base_addr;
};

/* Driver data */
struct flash_kb106x_data {
	struct k_sem flash_sem;
};

/* flash_controller local functions */
static inline bool flash_kb106x_range_exists(const struct device *dev, off_t offset, uint32_t len)
{
	struct flash_pages_info info;

	return !(flash_get_page_info_by_offs(dev, offset, &info) < 0 ||
		 flash_get_page_info_by_offs(dev, offset + len - 1, &info) < 0);
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
#if defined(CONFIG_FLASH_ENE_EFLASH_PROTECT)
static int flash_kb106x_ex_op_protect_set(const struct device *dev, const uintptr_t in, void *out)
{
	const struct flash_kb106x_config *config = dev->config;
	struct ene_ex_ops_protect_config *protect_cfg = (struct ene_ex_ops_protect_config *)in;
	struct efp_regs *efp;

	ARG_UNUSED(out);
	if (protect_cfg->region_num > MAX_PROTECT_REGION_NUM) {
		LOG_ERR("protect_region_num(%u) excess max number", MAX_PROTECT_REGION_NUM);
		return -EINVAL;
	}
	if (protect_cfg->start_page_index > protect_cfg->end_page_index) {
		LOG_ERR("protect range error (start > end)");
		return -EINVAL;
	}

	efp = config->efp_base_addr + protect_cfg->region_num;
	efp->EFPADDR =
		(protect_cfg->end_page_index << EFP_END_UNIT_POS) | protect_cfg->start_page_index;
	efp->EFPCR = protect_cfg->protect_type | (protect_cfg->config_lock << EFP_LOCK_POS);

	return 0;
}

static int flash_kb106x_ex_op_protect_get(const struct device *dev, const uintptr_t in, void *out)
{
	const struct flash_kb106x_config *config = dev->config;
	struct ene_ex_ops_protect_config *protect_cfg = (struct ene_ex_ops_protect_config *)in;
	struct ene_ex_ops_protect_config *protect_get = (struct ene_ex_ops_protect_config *)out;
	struct efp_regs *efp;

	if (protect_cfg->region_num > MAX_PROTECT_REGION_NUM) {
		LOG_ERR("protect_region_num(%u) excess max number", MAX_PROTECT_REGION_NUM);
		return -EINVAL;
	}

	efp = config->efp_base_addr + protect_cfg->region_num;
	protect_get->region_num = protect_cfg->region_num;
	protect_get->end_page_index = FIELD_GET((GENMASK(31, 16)), efp->EFPADDR);
	protect_get->start_page_index = FIELD_GET((GENMASK(15, 00)), efp->EFPADDR);
	protect_get->protect_type = efp->EFPCR & EFP_PROTECT_TYPE_MASK;
	protect_get->config_lock = FIELD_GET((GENMASK(01, 01)), efp->EFPCR);

	return 0;
}
#endif /* CONFIG_FLASH_ENE_EFLASH_PROTECT */
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

/* flash_controller api functions */
static int flash_kb106x_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if (len == 0U) {
		return 0;
	}

	if (offset < 0 || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		LOG_ERR("Read range invalid. Offset: %ld, len: %zu", offset, len);
		return -EINVAL;
	}

	LOG_DBG("Read offset: %ld, len: %zu", offset, len);

	memcpy(data, (uint8_t *)SOC_NV_FLASH_ADDR + offset, len);

	return 0;
}

static int flash_kb106x_erase(const struct device *dev, off_t offset, size_t len)
{
	const struct flash_kb106x_config *config = dev->config;
	struct flash_kb106x_data *data = dev->data;
	struct xbi_regs *xbi = (struct xbi_regs *)config->base_addr;
	uint32_t timeout;
	int rc = 0;
	struct flash_pages_info info;
	uint32_t start_sector, end_sector;

	if (len == 0U) {
		return 0;
	}

	if ((offset % FLASH_ERASE_BLOCK_SIZE) || (len % FLASH_ERASE_BLOCK_SIZE)) {
		LOG_ERR("Erase offset/len is not multiples of the erase block size(%x)",
			FLASH_ERASE_BLOCK_SIZE);
		return -EINVAL;
	}

	rc = flash_get_page_info_by_offs(dev, offset, &info);
	if (rc < 0) {
		return rc;
	}
	start_sector = info.index;
	rc = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (rc < 0) {
		return rc;
	}
	end_sector = info.index;

	k_sem_take(&data->flash_sem, K_FOREVER);

	xbi->EFADDR = offset + SOC_NV_FLASH_ADDR;
	for (int i = start_sector; i <= end_sector; i++) {
		/* CMD_Sector_Erase_Increase */
		xbi->EFCMD = CMD_SECTOR_ERASE_INCREASE;
		for (timeout = FLASH_ENE_TIMEOUT_US; (xbi->EFSTA & BIT(0)) && (timeout > 0);
		     timeout--) {
			k_busy_wait(1);
		}
		if (timeout == 0) {
			rc = -ETIMEDOUT;
			break;
		}
	}

	k_sem_give(&data->flash_sem);

	return rc;
}

static int flash_kb106x_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_kb106x_data *dev_data = dev->data;
	const struct flash_kb106x_config *config = dev->config;
	struct xbi_regs *xbi = (struct xbi_regs *)config->base_addr;
	unsigned char *wdat = (unsigned char *)data;
	uint32_t timeout;
	int rc = 0;

	if (len == 0U) {
		return 0;
	}

	if ((offset % FLASH_WRITE_BLOCK_SIZE) || (len % FLASH_WRITE_BLOCK_SIZE)) {
		LOG_ERR("Write offset/len is not multiples of the write block size(%x)",
			FLASH_WRITE_BLOCK_SIZE);
		return -EINVAL;
	}

	if (offset < 0 || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		LOG_ERR("Write range invalid. Offset: %ld, len: %zu", offset, len);
		return -EINVAL;
	}

	k_sem_take(&dev_data->flash_sem, K_FOREVER);

	LOG_DBG("Write offset: %ld, len: %zu", offset, len);

	xbi->EFADDR = offset + SOC_NV_FLASH_ADDR;
	for (int i = 0; i < len; i++) {
		xbi->EFDAT = wdat[i];
		/* CMD_Program_Increase */
		xbi->EFCMD = CMD_PROGRAM_INCREASE;
		for (timeout = FLASH_ENE_TIMEOUT_US; (xbi->EFSTA & BIT(0)) && (timeout > 0);
		     timeout--) {
			k_busy_wait(1);
		}
		if (timeout == 0) {
			rc = -ETIMEDOUT;
			break;
		}
	}

	k_sem_give(&dev_data->flash_sem);

	return rc;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_kb106x_ex_op(const struct device *dev, uint16_t code, const uintptr_t in,
			      void *out)
{
	int rv = -ENOTSUP;
#if defined(CONFIG_FLASH_ENE_EFLASH_PROTECT)
	struct flash_kb106x_data *data = dev->data;

	LOG_DBG("%s: %d", __func__, code);

	k_sem_take(&data->flash_sem, K_FOREVER);

	switch (code) {
	case FLASH_ENE_EX_OP_PROTECT_CONFIG:
		rv = flash_kb106x_ex_op_protect_set(dev, in, out);
		break;

	case FLASH_ENE_EX_OP_PROTECT_GET:
		rv = flash_kb106x_ex_op_protect_get(dev, in, out);
		break;
	}

	k_sem_give(&data->flash_sem);
#endif /* CONFIG_FLASH_ENE_EFLASH_PROTECT */

	return rv;
}
#endif

static const struct flash_parameters *flash_kb106x_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_kb106x_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout pages_layout = {
	.pages_count = SOC_NV_FLASH_SIZE / FLASH_ERASE_BLOCK_SIZE,
	.pages_size = FLASH_ERASE_BLOCK_SIZE,
};

static void flash_kb106x_page_layout(const struct device *dev,
				     const struct flash_pages_layout **layout, size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_kb106x_api) = {
	.erase = flash_kb106x_erase,
	.write = flash_kb106x_write,
	.read = flash_kb106x_read,
	.get_parameters = flash_kb106x_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_kb106x_page_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_kb106x_ex_op,
#endif
};

static int kb106x_flash_init(const struct device *dev)
{
	const struct flash_kb106x_config *config = dev->config;
	struct flash_kb106x_data *data = dev->data;
	struct xbi_regs *xbi = (struct xbi_regs *)config->base_addr;

	/* Enable EF command */
	xbi->EFCFG |= BIT(0);

	k_sem_init(&data->flash_sem, 1, 1);

	return 0;
}

static struct flash_kb106x_data flash_kb106x_data_0;
static const struct flash_kb106x_config flash_kb106x_config_0 = {
	.base_addr = (struct xbi_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0),
	.efp_base_addr = (struct efp_regs *)DT_INST_REG_ADDR_BY_IDX(0, 1),
};
DEVICE_DT_INST_DEFINE(0, kb106x_flash_init, NULL, &flash_kb106x_data_0, &flash_kb106x_config_0,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_kb106x_api);
