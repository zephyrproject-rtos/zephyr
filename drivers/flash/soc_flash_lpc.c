/*
 * Copyright (c) 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <errno.h>
#include <zephyr/init.h>
#include <soc.h>
#include "flash_priv.h"

#include "fsl_common.h"
#include "fsl_flashiap.h"


#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_iap_fmc11))
#define DT_DRV_COMPAT nxp_iap_fmc11
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_iap_fmc54))
#define DT_DRV_COMPAT nxp_iap_fmc54
#else
#error No matching compatible for soc_flash_lpc.c
#endif

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

struct flash_priv {
	/* HACK: flash write protection is managed in software. */
	struct k_sem write_lock;
	uint32_t pflash_block_base;
	uint32_t sector_size;
};

static const struct flash_parameters flash_lpc_parameters = {
#if DT_NODE_HAS_PROP(SOC_NV_FLASH_NODE, write_block_size)
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
#else
	.write_block_size = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE,
#endif
	.erase_value = 0xff,
};

static inline void prepare_erase_write(off_t offset, size_t len,
						uint32_t sector_size)
{
	uint32_t start;
	uint32_t stop;

	start = offset / sector_size;
	stop = (offset+len-1) / sector_size;
	FLASHIAP_PrepareSectorForWrite(start, stop);
}

static int flash_lpc_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_priv *priv = dev->data;
	status_t rc;
	unsigned int key;
	uint32_t start;
	uint32_t stop;
	uint32_t page_size;

	if (k_sem_take(&priv->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	key = irq_lock();
	prepare_erase_write(offset, len, priv->sector_size);
	page_size = flash_lpc_parameters.write_block_size;
	start = offset / page_size;
	stop = (offset+len-1) / page_size;
	rc = FLASHIAP_ErasePage(start, stop,
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_FLASHIAP_Success) ? 0 : -EINVAL;
}

static int flash_lpc_read(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_priv *priv = dev->data;
	uint32_t addr;

	addr = offset + priv->pflash_block_base;

	memcpy(data, (void *) addr, len);

	return 0;
}

static int flash_lpc_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_priv *priv = dev->data;
	uint32_t addr;
	status_t rc;
	unsigned int key;

	if (k_sem_take(&priv->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	addr = offset + priv->pflash_block_base;

	key = irq_lock();
	prepare_erase_write(offset, len, priv->sector_size);
	rc = FLASHIAP_CopyRamToFlash(addr, (uint32_t *) data, len,
				CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_FLASHIAP_Success) ? 0 : -EINVAL;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_lpc_pages_layout(const struct device *dev,
			const struct flash_pages_layout **layout,
			size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_lpc_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_lpc_parameters;
}

static struct flash_priv flash_data;

static DEVICE_API(flash, flash_lpc_api) = {
	.erase = flash_lpc_erase,
	.write = flash_lpc_write,
	.read = flash_lpc_read,
	.get_parameters = flash_lpc_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_lpc_pages_layout,
#endif
};

static int flash_lpc_init(const struct device *dev)
{
	struct flash_priv *priv = dev->data;

	k_sem_init(&priv->write_lock, 1, 1);

	priv->pflash_block_base = DT_REG_ADDR(SOC_NV_FLASH_NODE);

#if defined(FSL_FEATURE_SYSCON_FLASH_SECTOR_SIZE_BYTES)
	priv->sector_size = FSL_FEATURE_SYSCON_FLASH_SECTOR_SIZE_BYTES;
#else
	#error "Sector size not set"
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_lpc_init, NULL,
			&flash_data, NULL, POST_KERNEL,
			CONFIG_FLASH_INIT_PRIORITY, &flash_lpc_api);
