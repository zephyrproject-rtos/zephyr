/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdio.h>
#include <zephyr/sys/device_mmio.h>

LOG_MODULE_REGISTER(mdio_stm32_hal2, CONFIG_MDIO_LOG_LEVEL);
#define DT_DRV_COMPAT st_stm32_mdio

struct mdio_stm32_data {
	DEVICE_MMIO_RAM;
	struct k_mutex mutex;
	hal_eth_handle_t heth;
};

struct mdio_stm32_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pincfg;
};

static int mdio_stm32_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_status_t ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = HAL_ETH_MDIO_C22ReadData(heth, prtad, regad, data);

	k_mutex_unlock(&dev_data->mutex);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static int mdio_stm32_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_status_t ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = HAL_ETH_MDIO_C22WriteData(heth, prtad, regad, data);

	k_mutex_unlock(&dev_data->mutex);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static int mdio_stm32_init(const struct device *dev)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	const struct mdio_stm32_config *const config = dev->config;
	hal_eth_handle_t *heth = &dev_data->heth;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	heth->instance = (hal_eth_t)DEVICE_MMIO_GET(dev);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&dev_data->mutex);

	return 0;
}

static DEVICE_API(mdio, mdio_stm32_api) = {
	.read = mdio_stm32_read,
	.write = mdio_stm32_write,
};

#define MDIO_STM32_HAL2_DEVICE(inst)                                                               \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct mdio_stm32_data mdio_stm32_data_##inst;                                      \
	static const struct mdio_stm32_config mdio_stm32_config_##inst = {                         \
		DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(inst)),                                        \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mdio_stm32_init, NULL, &mdio_stm32_data_##inst,               \
			      &mdio_stm32_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_STM32_HAL2_DEVICE)
