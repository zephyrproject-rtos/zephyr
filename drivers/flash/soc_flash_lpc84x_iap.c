/* Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_iap_fmc84x

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <errno.h>
#include <soc.h>

#include <fsl_iap.h>
#include <fsl_common.h>
#include <fsl_power.h>

#include "flash_priv.h"

LOG_MODULE_REGISTER(flash_lpc84x_iap, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_ERASE_VALUE 0xff
#define LPC84X_PAGE_SIZE  FSL_FEATURE_SYSCON_FLASH_PAGE_SIZE_BYTES
#define FLASH_LAYOUT_SIZE 1

struct flash_lpc84x_iap_config {
	DEVICE_MMIO_NAMED_ROM(regs);
	DEVICE_MMIO_NAMED_ROM(flash_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t flash_size;
	uint32_t erase_size;
	uint32_t page_size;
	uint32_t flashcfg_time;
	struct flash_parameters flash_param;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

struct flash_lpc84x_iap_data {
	DEVICE_MMIO_NAMED_RAM(regs);
	DEVICE_MMIO_NAMED_RAM(flash_base);
	struct k_sem write_lock;
	/*
	 * IAP write source buffer must be in SRAM flash is inaccessible during write.
	 * __aligned(4) for IAP word-alignment requirement
	 */
	uint8_t __aligned(4) page_buf[LPC84X_PAGE_SIZE];
};

/* Defined macro to get the Register Base Address */
#define DEV_BASE(dev) DEVICE_MMIO_NAMED_GET(dev, flash_base)

#define DEV_CFG(dev)  ((const struct flash_lpc84x_iap_config *)(dev)->config)
#define DEV_DATA(dev) ((struct flash_lpc84x_iap_data *)(dev)->data)

/*
 * @brief Helper function to wrap FSL IAP status to zephyr errno
 *
 * kStatus_IAP_Success == kStatus_Success in this SDK
 *
 * @param  status Type used for all status and error return values ref, fsl_common.h
 */
static int iap_to_errno(status_t status)
{
	switch (status) {
	case kStatus_Success:
		return 0;
	case kStatus_IAP_Busy:
		return -EBUSY;
	case kStatus_IAP_NotPrepared:
		return -EACCES;
	case kStatus_IAP_SectorNotblank:
		return -ENOTEMPTY;
	case kStatus_IAP_CompareError:
		return -EIO;
	case kStatus_IAP_NoPower:
	case kStatus_IAP_NoClock:
		return -EIO;
	case kStatus_IAP_InvalidSector:
	case kStatus_IAP_CountError:
	case kStatus_IAP_ParamError:
	case kStatus_IAP_AddrError:
	case kStatus_IAP_AddrNotMapped:
	case kStatus_IAP_DstAddrError:
	case kStatus_IAP_SrcAddrError:
	default:
		return -EINVAL;
	}
}

/*
 * @brief:
 *
 * Prepare the sectors that cover the byte range (dst, dst+size).
 * Mandatory before every IAP write/erase.
 */
static int lpc84x_prepare_sectors(const struct flash_lpc84x_iap_config *cfg, uint32_t offset,
				  uint32_t size)
{
	uint32_t start_sector = offset / cfg->erase_size;
	uint32_t end_sector = (offset + size - 1) / cfg->erase_size;

	return iap_to_errno(IAP_PrepareSectorForWrite(start_sector, end_sector));
}

static bool flash_lpc84x_range_is_valid(const struct flash_lpc84x_iap_config *cfg, off_t offset,
					size_t len)
{
	if (offset < 0) {
		return false;
	}

	/* Check does not overflow and stays within flash */
	return (((uint64_t)offset + (uint64_t)len) <= (uint64_t)cfg->flash_size);
}

/*
 * @note:
 *
 * Flash is always readable while no IAP call is active.
 * No lock needed reads cannot interfere with each other.
 */
static int flash_lpc84x_iap_read(const struct device *dev, off_t offset, void *data_ptr, size_t len)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;

	if (flash_lpc84x_range_is_valid(cfg, offset, len) != true) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	memcpy(data_ptr, (void *)(DEV_BASE(dev) + (uint32_t)offset), len);

	return 0;
}

/*
 * @brief flash_write IAP CopyRamToFlash requirement
 *
 * Iterate in the largest valid IAP chunk ≤ remaining.
 * Copy user data  which may be in flash  into data->page_buf (SRAM, aligned).
 * Call prepare + copy for each chunk
 *
 * IAP_CopyRamToFlash calls __disable_irq/__enable_irq internally
 * We therefore don't call irq_lock here to avoid double nesting.
 * The k_sem guarantees thread-level serialisation
 */
static int flash_lpc84x_iap_write(const struct device *dev, off_t offset, const void *data_ptr,
				  size_t len)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;
	struct flash_lpc84x_iap_data *data = dev->data;
	const uint8_t *src = (const uint8_t *)data_ptr;
	size_t remaining = len;
	uint32_t dst, chunk;
	int ret = 0;

	if (data_ptr == NULL) {
		return -EINVAL;
	}

	if (flash_lpc84x_range_is_valid(cfg, offset, len) != true) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	/* Page-alignment: offset and len must be multiples of write_block_size 4*/
	if (((uint32_t)offset % cfg->page_size) != 0 || (len % cfg->page_size) != 0) {
		LOG_ERR("write misaligned: offset=0x%lx len=%zu", (long)offset, len);
		return -EINVAL;
	}

	dst = DEV_BASE(dev) + (uint32_t)offset;
	if (k_sem_take(&data->write_lock, K_FOREVER) != 0) {
		return -EACCES;
	}

	while (remaining > 0) {
		chunk = MIN(remaining, LPC84X_PAGE_SIZE);

		if ((dst % cfg->page_size) != 0) {
			LOG_ERR("write: Destination address 0x%08x isn't page-aligned", dst);
			ret = -EINVAL;
			break;
		}

		/*
		 * Source may be in flash (e.g., a const array). Flash is inaccessible during IAP
		 * write so we must copy to SRAM page_buf first.
		 */
		memcpy(data->page_buf, src, chunk);

		ret = lpc84x_prepare_sectors(cfg, dst, chunk);
		if (ret != 0) {
			LOG_ERR("write: prepare failed at 0x%08x: %d", dst, ret);
			break;
		}

		ret = iap_to_errno(IAP_CopyRamToFlash(dst, (uint32_t *)data->page_buf, chunk,
						      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC));
		if (ret != 0) {
			LOG_ERR("write: IAP_CopyRamToFlash at 0x%08x failed: %d", dst, ret);
			break;
		}

		ret = iap_to_errno(IAP_Compare(dst, (uint32_t *)data->page_buf, chunk));
		if (ret != 0) {
			LOG_ERR("Write verify failed: %d", ret);
			break;
		}

		src += chunk;
		dst += chunk;
		remaining -= chunk;
	}

	k_sem_give(&data->write_lock);

	return ret;
}

static int flash_lpc84x_iap_erase(const struct device *dev, off_t offset, size_t len)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;
	struct flash_lpc84x_iap_data *data = dev->data;
	uint32_t start_sector, end_sector;
	int ret = 0;

	if (flash_lpc84x_range_is_valid(cfg, offset, len) != true) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if (((uint32_t)offset % cfg->erase_size) != 0U || (len % cfg->erase_size) != 0U) {
		LOG_ERR("Erase not sector-aligned: offset=0x%lx len=%zu", (long)offset, len);
		return -EINVAL;
	}

	if (k_sem_take(&data->write_lock, K_FOREVER) != 0) {
		return -EACCES;
	}

	ret = lpc84x_prepare_sectors(cfg, offset, len);
	if (ret != 0) {
		LOG_ERR("Erase: prepare failed at 0x%08lx: %d", (long)offset, ret);
		goto err;
	}

	start_sector = offset / cfg->erase_size;
	end_sector = (offset + len - 1) / cfg->erase_size;

	ret = iap_to_errno(
		IAP_EraseSector(start_sector, end_sector, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC));
	if (ret != 0) {
		LOG_ERR("Erase sectors %u–%u failed: %d", start_sector, end_sector, ret);
		goto err;
	}

	/*
	 * It used to verify the sector erasure after IAP_EraseSector call.
	 */
	ret = iap_to_errno(IAP_BlankCheckSector(start_sector, end_sector));
	if (ret != 0) {
		LOG_ERR("sectors check %u–%u failed: %d", start_sector, end_sector, ret);
	}

err:
	k_sem_give(&data->write_lock);

	return ret;
}

static int flash_lpc84x_iap_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;

	*size = (uint64_t)cfg->flash_size;

	return 0;
}

static const struct flash_parameters *flash_lpc84x_iap_get_parameters(const struct device *dev)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;

	return &cfg->flash_param;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_lpc84x_iap_pages_layout(const struct device *dev,
					  const struct flash_pages_layout **layout,
					  size_t *layout_size)
{
	const struct flash_lpc84x_iap_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = FLASH_LAYOUT_SIZE;
}
#endif

static DEVICE_API(flash, flash_lpc84x_iap_api) = {
	.read = flash_lpc84x_iap_read,
	.write = flash_lpc84x_iap_write,
	.erase = flash_lpc84x_iap_erase,
	.get_parameters = flash_lpc84x_iap_get_parameters,
	.get_size = flash_lpc84x_iap_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_lpc84x_iap_pages_layout,
#endif
};

static int flash_lpc84x_iap_init(const struct device *dev)
{
	const struct flash_lpc84x_iap_config *cfg = dev->config;
	struct flash_lpc84x_iap_data *data = dev->data;
	int ret = 0;

	DEVICE_MMIO_NAMED_MAP(dev, regs, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, flash_base, K_MEM_CACHE_NONE);

	/* Disable PDRUNCFG bit in the Syscon, that disabling the bit powers up the peripheral
	 * POWER_DisablePD(kPDRUNCFG_PD_FLASH);
	 *
	 * @note:
	 * Flash is powered at reset (PDRUNCFG[FLASH_PD] = 0 by default).
	 * No explicit power-up call is needed here. If deep power-down
	 * support is added in the future, add POWER_DisablePD(kPDRUNCFG_PD_FLASH)
	 * and the matching wake-up hook.
	 */
	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->clock_dev);
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable flash clock : %d", ret);
		return ret;
	}

	k_sem_init(&data->write_lock, 1, 1);

	/*
	 *  Configure flash wait states for the system clock frequency.
	 *  0 = 1 clock (up to 18 MHz)
	 *  1 = 2 clocks (up to 24 MHz)
	 *  2 = 3 clocks (up to 30 MHz)
	 *  FLASHTIM +1 is equal to the number of system clocks used for flash access.
	 */
	IAP_ConfigAccessFlashTime(cfg->flashcfg_time);

	return ret;
}

#define LPC84X_FLASH_IAP_INIT(inst)                                                                \
                                                                                                   \
	static const struct flash_lpc84x_iap_config flash_lpc84x_iap_config_##inst = {             \
		DEVICE_MMIO_NAMED_ROM_INIT(regs, DT_DRV_INST(inst)),                               \
		DEVICE_MMIO_NAMED_ROM_INIT(flash_base, SOC_NV_FLASH_CHILD_NODE(inst)),             \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
		.flash_size = DT_REG_SIZE(SOC_NV_FLASH_CHILD_NODE(inst)),                          \
		.erase_size = DT_PROP(SOC_NV_FLASH_CHILD_NODE(inst), erase_block_size),            \
		.page_size = DT_PROP(SOC_NV_FLASH_CHILD_NODE(inst), write_block_size),             \
		.flashcfg_time = DT_INST_PROP(inst, flash_access_time),                            \
		.flash_param =                                                                     \
			{                                                                          \
				.write_block_size =                                                \
					DT_PROP(SOC_NV_FLASH_CHILD_NODE(inst), write_block_size), \
				.erase_value = FLASH_ERASE_VALUE,                                  \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,						   \
			(.layout = {								   \
			 .pages_count = DT_REG_SIZE(SOC_NV_FLASH_CHILD_NODE(inst)) /		\
			 DT_PROP(SOC_NV_FLASH_CHILD_NODE(inst), erase_block_size),		\
			 .pages_size = DT_PROP(SOC_NV_FLASH_CHILD_NODE(inst), erase_block_size), \
			 }))};         \
                                                                                                   \
	static struct flash_lpc84x_iap_data flash_lpc84x_iap_data_##inst;                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, flash_lpc84x_iap_init, NULL, &flash_lpc84x_iap_data_##inst,    \
			      &flash_lpc84x_iap_config_##inst, POST_KERNEL,                        \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_lpc84x_iap_api);

DT_INST_FOREACH_STATUS_OKAY(LPC84X_FLASH_IAP_INIT)
