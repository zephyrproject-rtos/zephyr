/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/sys_log.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include "flash_priv.h"

#include "fsl_common.h"
#include "fsl_flash.h"

struct flash_priv {
	flash_config_t config;
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
	int key;

	addr = offset + priv->config.PFlashBlockBase;

	key = irq_lock();
	rc = FLASH_Erase(&priv->config, addr, len, kFLASH_ApiEraseKey);
	irq_unlock(key);

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
	addr = offset + priv->config.PFlashBlockBase;

	memcpy(data, (void *) addr, len);

	return 0;
}

static int flash_mcux_write(struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	u32_t addr;
	status_t rc;
	int key;

	addr = offset + priv->config.PFlashBlockBase;

	key = irq_lock();
	rc = FLASH_Program(&priv->config, addr, (u32_t *) data, len);
	irq_unlock(key);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

static int flash_mcux_write_protection(struct device *dev, bool enable)
{
	return -EIO;
}

static struct flash_priv flash_data;

static const struct flash_driver_api flash_mcux_api = {
	.write_protection = flash_mcux_write_protection,
	.erase = flash_mcux_erase,
	.write = flash_mcux_write,
	.read = flash_mcux_read,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
	.write_block_size = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE,
};

static int flash_mcux_init(struct device *dev)
{
	struct flash_priv *priv = dev->driver_data;
	status_t rc;

	rc = FLASH_Init(&priv->config);

	return (rc == kStatus_Success) ? 0 : -EIO;
}

DEVICE_AND_API_INIT(flash_mcux, CONFIG_SOC_FLASH_MCUX_DEV_NAME,
			flash_mcux_init, &flash_data, NULL, POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_mcux_api);

