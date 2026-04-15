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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdio.h>

#include "../eth_stm32_hal_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_stm32_hal, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_mdio

#define ADIN1100_REG_VALUE_MASK		GENMASK(15, 0)

struct mdio_stm32_data {
	struct k_sem sem;
};

struct mdio_stm32_config {
	const struct device *ethernet_dev;
	const struct pinctrl_dev_config *pincfg;
};

static int mdio_stm32_read(const struct device *dev, uint8_t prtad,
			   uint8_t regad, uint16_t *data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	const struct mdio_stm32_config *const config = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = config->ethernet_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	HAL_StatusTypeDef ret;
	uint32_t read;

	k_sem_take(&dev_data->sem, K_FOREVER);

#ifdef CONFIG_ETH_STM32_HAL_API_V2
	ret = HAL_ETH_ReadPHYRegister(heth, prtad, regad, &read);
#else
	heth->Init.PhyAddress = prtad;

	ret = HAL_ETH_ReadPHYRegister(heth, regad, &read);
#endif

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	*data = read & ADIN1100_REG_VALUE_MASK;

	return 0;
}

static int mdio_stm32_write(const struct device *dev, uint8_t prtad,
			    uint8_t regad, uint16_t data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	const struct mdio_stm32_config *const config = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = config->ethernet_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	HAL_StatusTypeDef ret;

	k_sem_take(&dev_data->sem, K_FOREVER);

#ifdef CONFIG_ETH_STM32_HAL_API_V2
	ret = HAL_ETH_WritePHYRegister(heth, prtad, regad, data);
#else
	heth->Init.PhyAddress = prtad;

	ret = HAL_ETH_WritePHYRegister(heth, regad, data);
#endif

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return 0;
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
};

#define MDIO_STM32_HAL_DEVICE(inst)                                                                \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct mdio_stm32_data mdio_stm32_data_##inst;                                      \
                                                                                                   \
	static struct mdio_stm32_config mdio_stm32_config_##inst = {                               \
		.ethernet_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                               \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mdio_stm32_init, NULL, &mdio_stm32_data_##inst,               \
			      &mdio_stm32_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_STM32_HAL_DEVICE)
