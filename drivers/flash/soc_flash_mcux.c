/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include "flash_priv.h"

#include "fsl_common.h"
#ifdef CONFIG_HAS_MCUX_IAP
#include "fsl_iap.h"
#else
#include "fsl_flash.h"
#endif

#if DT_NODE_HAS_STATUS(DT_INST(0, nxp_kinetis_ftfa), okay)
#define DT_DRV_COMPAT nxp_kinetis_ftfa
#elif DT_NODE_HAS_STATUS(DT_INST(0, nxp_kinetis_ftfe), okay)
#define DT_DRV_COMPAT nxp_kinetis_ftfe
#elif DT_NODE_HAS_STATUS(DT_INST(0, nxp_kinetis_ftfl), okay)
#define DT_DRV_COMPAT nxp_kinetis_ftfl
#elif DT_NODE_HAS_STATUS(DT_INST(0, nxp_lpc_iap), okay)
#define DT_DRV_COMPAT nxp_lpc_iap
#else
#error No matching compatible for soc_flash_mcux.c
#endif

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

struct flash_priv {
	flash_config_t config;
	/*
	 * HACK: flash write protection is managed in software.
	 */
	struct k_sem write_lock;
	u32_t pflash_block_base;
};

/*
 * Interrupt vectors could be executed from flash hence the need for locking.
 * The underlying MCUX driver takes care of copying the functions to SRAM.
 *
 * For more information, see the application note below on Read-While-Write
 * http://cache.freescale.com/files/32bit/doc/app_note/AN4695.pdf
 *
 */

static int flash_mcux_erase(struct device *dev, off_t offset, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	u32_t addr;
	status_t rc;
	unsigned int key;

	if (k_sem_take(&priv->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

	addr = offset + priv->pflash_block_base;

	key = irq_lock();
	rc = FLASH_Erase(&priv->config, addr, len, kFLASH_ApiEraseKey);
	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

static int flash_mcux_read(struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	u32_t addr;

	/*
	 * The MCUX supports different flash chips whose valid ranges are
	 * hidden below the API: until the API export these ranges, we can not
	 * do any generic validation
	 */
	addr = offset + priv->pflash_block_base;

	memcpy(data, (void *) addr, len);

	return 0;
}

static int flash_mcux_write(struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	u32_t addr;
	status_t rc;
	unsigned int key;

	if (k_sem_take(&priv->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

	addr = offset + priv->pflash_block_base;

	key = irq_lock();
	rc = FLASH_Program(&priv->config, addr, (uint8_t *) data, len);
	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

static int flash_mcux_write_protection(struct device *dev, bool enable)
{
	struct flash_priv *priv = dev->driver_data;
	int rc = 0;

	if (enable) {
		rc = k_sem_take(&priv->write_lock, K_FOREVER);
	} else {
		k_sem_give(&priv->write_lock);
	}

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_mcux_pages_layout(struct device *dev,
									const struct flash_pages_layout **layout,
									size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static struct flash_priv flash_data;

static const struct flash_driver_api flash_mcux_api = {
	.write_protection = flash_mcux_write_protection,
	.erase = flash_mcux_erase,
	.write = flash_mcux_write,
	.read = flash_mcux_read,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mcux_pages_layout,
#endif
#if DT_NODE_HAS_PROP(SOC_NV_FLASH_NODE, write_block_size)
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
#else
	.write_block_size = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE,
#endif
};

static int flash_mcux_init(struct device *dev)
{
	struct flash_priv *priv = dev->driver_data;
	uint32_t pflash_block_base;
	status_t rc;

	k_sem_init(&priv->write_lock, 0, 1);

	rc = FLASH_Init(&priv->config);

#ifdef CONFIG_HAS_MCUX_IAP
	FLASH_GetProperty(&priv->config, kFLASH_PropertyPflashBlockBaseAddr,
			  &pflash_block_base);
#else
	FLASH_GetProperty(&priv->config, kFLASH_PropertyPflash0BlockBaseAddr,
			  &pflash_block_base);
#endif
	priv->pflash_block_base = (u32_t) pflash_block_base;

	return (rc == kStatus_Success) ? 0 : -EIO;
}

DEVICE_AND_API_INIT(flash_mcux, DT_INST_LABEL(0),
			flash_mcux_init, &flash_data, NULL, POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_mcux_api);
