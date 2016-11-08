/*
 * Copyright (c) 2016 Linaro Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <misc/sys_log.h>
#include <nanokernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <errno.h>
#include <init.h>
#include <soc.h>

#include "fsl_common.h"
#include "fsl_flash.h"

struct flash_priv {
	flash_config_t config;
};

/*
 * Interrupt vectors could be executed from flash hence the need for locking.
 * The underlying KDSK driver takes care of copying the functions to SRAM.
 *
 * For more information, see the application note below on Read-While-Write
 * http://cache.freescale.com/files/32bit/doc/app_note/AN4695.pdf
 *
 */

static int flash_ksdk_erase(struct device *dev, off_t offset, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	uint32_t addr;
	status_t rc;
	int key;

	addr = offset + priv->config.PFlashBlockBase;

	key = irq_lock();
	rc = FLASH_Erase(&priv->config, addr, len, kFLASH_apiEraseKey);
	irq_unlock(key);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

static int flash_ksdk_read(struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	uint32_t addr;

	/*
	 * The KSDK supports different flash chips whose valid ranges are
	 * hidden below the API: until the API export these ranges, we can not
	 * do any generic validation
	 */
	addr = offset + priv->config.PFlashBlockBase;

	memcpy(data, (void *) addr, len);

	return 0;
}

static int flash_ksdk_write(struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_priv *priv = dev->driver_data;
	uint32_t addr;
	status_t rc;
	int key;

	addr = offset + priv->config.PFlashBlockBase;

	key = irq_lock();
	rc = FLASH_Program(&priv->config, addr, (uint32_t *) data, len);
	irq_unlock(key);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

static int flash_ksdk_write_protection(struct device *dev, bool enable)
{
	return -EIO;
}

static struct flash_priv flash_data;

static const struct flash_driver_api flash_ksdk_api = {
	.write_protection = flash_ksdk_write_protection,
	.erase = flash_ksdk_erase,
	.write = flash_ksdk_write,
	.read = flash_ksdk_read,
};

static int flash_ksdk_init(struct device *dev)
{
	struct flash_priv *priv = dev->driver_data;
	status_t rc;

	rc = FLASH_Init(&priv->config);

	return (rc == kStatus_Success) ? 0 : -EIO;
}

DEVICE_AND_API_INIT(flash_ksdk, CONFIG_SOC_FLASH_KSDK_DEV_NAME,
			flash_ksdk_init, &flash_data, NULL, POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_ksdk_api);

