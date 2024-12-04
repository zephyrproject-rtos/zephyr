/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_stm32_hal, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_mdio

#define ADIN1100_REG_VALUE_MASK		GENMASK(15, 0)

struct mdio_stm32_data {
	struct k_sem sem;
	ETH_HandleTypeDef heth;
};

struct mdio_stm32_config {
	const struct pinctrl_dev_config *pincfg;
};

static int mdio_stm32_read(const struct device *dev, uint8_t prtad,
			   uint8_t regad, uint16_t *data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	uint32_t read;
	int ret;

	k_sem_take(&dev_data->sem, K_FOREVER);

	ret = HAL_ETH_ReadPHYRegister(heth, prtad, regad, &read);

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	*data = read & ADIN1100_REG_VALUE_MASK;

	return ret;
}

static int mdio_stm32_write(const struct device *dev, uint8_t prtad,
			    uint8_t regad, uint16_t data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int ret;

	k_sem_take(&dev_data->sem, K_FOREVER);

	ret = HAL_ETH_WritePHYRegister(heth, prtad, regad, data);

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return ret;
}

static void mdio_stm32_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void mdio_stm32_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int mdio_stm32_init(const struct device *dev)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	const struct mdio_stm32_config *const config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&dev_data->sem, 1, 1);

	return 0;
}

static DEVICE_API(mdio, mdio_stm32_api) = {
	.read = mdio_stm32_read,
	.write = mdio_stm32_write,
	.bus_enable = mdio_stm32_bus_enable,
	.bus_disable = mdio_stm32_bus_disable,
};

#define MDIO_STM32_HAL_DEVICE(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static struct mdio_stm32_data mdio_stm32_data_##inst = {				\
		.heth = {.Instance = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst))},		\
	};											\
	static struct mdio_stm32_config mdio_stm32_config_##inst = {				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
	};											\
	DEVICE_DT_INST_DEFINE(inst, &mdio_stm32_init, NULL,					\
				&mdio_stm32_data_##inst, &mdio_stm32_config_##inst,		\
				POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,				\
				&mdio_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_STM32_HAL_DEVICE)
