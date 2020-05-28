/*
 * Copyright (c) 2019 Kwon Tae-young <tykwon@m2i.co.kr>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_eeprom

#include <drivers/eeprom.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_stm32);

K_MUTEX_DEFINE(lock);

struct eeprom_stm32_config {
	uint32_t addr;
	size_t size;
};

static int eeprom_stm32_read(struct device *dev, off_t offset, void *buf,
				size_t len)
{
	const struct eeprom_stm32_config *config = dev->config;
	uint8_t *pbuf = buf;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	while (len) {
		*pbuf = *(__IO uint8_t*)(config->addr + offset);

		pbuf++;
		offset++;
		len--;
	}

	k_mutex_unlock(&lock);

	return 0;
}

static int eeprom_stm32_write(struct device *dev, off_t offset,
				const void *buf, size_t len)
{
	const struct eeprom_stm32_config *config = dev->config;
	const uint8_t *pbuf = buf;
	HAL_StatusTypeDef ret = HAL_OK;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	HAL_FLASHEx_DATAEEPROM_Unlock();

	while (len) {
		ret = HAL_FLASHEx_DATAEEPROM_Program(
						FLASH_TYPEPROGRAMDATA_BYTE,
						config->addr + offset, *pbuf);
		if (ret) {
			LOG_ERR("failed to write to EEPROM (err %d)", ret);
			HAL_FLASHEx_DATAEEPROM_Lock();
			k_mutex_unlock(&lock);
			return ret;
		}

		pbuf++;
		offset++;
		len--;
	}

	ret = HAL_FLASHEx_DATAEEPROM_Lock();
	if (ret) {
		LOG_ERR("failed to lock EEPROM (err %d)", ret);
		k_mutex_unlock(&lock);
		return ret;
	}

	k_mutex_unlock(&lock);

	return ret;
}

static size_t eeprom_stm32_size(struct device *dev)
{
	const struct eeprom_stm32_config *config = dev->config;

	return config->size;
}

static int eeprom_stm32_init(struct device *dev)
{
	return 0;
}

static const struct eeprom_driver_api eeprom_stm32_api = {
	.read = eeprom_stm32_read,
	.write = eeprom_stm32_write,
	.size = eeprom_stm32_size,
};

static const struct eeprom_stm32_config eeprom_config = {
	.addr = DT_INST_REG_ADDR(0),
	.size = DT_INST_REG_SIZE(0),
};

DEVICE_AND_API_INIT(eeprom_stm32, DT_INST_LABEL(0),
		    &eeprom_stm32_init, NULL,
		    &eeprom_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_stm32_api);
