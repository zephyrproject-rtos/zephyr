/*
 * Based on sdmmc_stm32,c:
 * Copyright (c) 2020 Amarula Solutions.
 *
 * eMMC updates:
 * Copyright (c) 2021 Logitech, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_emmc

#include <devicetree.h>
#include <drivers/disk.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <soc.h>
#include <stm32_ll_rcc.h>

LOG_MODULE_REGISTER(stm32_emmc, CONFIG_SDMMC_LOG_LEVEL);

struct stm32_sdmmc_priv {
	MMC_HandleTypeDef hsd;
	int status;
	struct {
		const char *name;
		const struct device *port;
		int pin;
		int flags;
	} rst;
	struct stm32_pclken pclken;
	struct {
		const struct soc_gpio_pinctrl *list;
		size_t len;
	} pinctrl;
};

static int stm32_sdmmc_clock_enable(struct stm32_sdmmc_priv *priv)
{
	const struct device *sdmmc_clock;

#if CONFIG_SOC_SERIES_STM32L4X
	LL_RCC_PLLSAI1_Disable();
	/* Configure PLLSA11 to enable 48M domain */
	LL_RCC_PLLSAI1_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSI,
					LL_RCC_PLLM_DIV_1,
					8, LL_RCC_PLLSAI1Q_DIV_8);

	/* Enable PLLSA1 */
	LL_RCC_PLLSAI1_Enable();

	/*  Enable PLLSAI1 output mapped on 48MHz domain clock */
	LL_RCC_PLLSAI1_EnableDomain_48M();

	/* Wait for PLLSA1 ready flag */
	while (LL_RCC_PLLSAI1_IsReady() != 1)
		;

	LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC1_CLKSOURCE_PLLSAI1);
#endif

	sdmmc_clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Enable the APB clock for stm32_sdmmc */
	return clock_control_on(sdmmc_clock,
			(clock_control_subsys_t *)&priv->pclken);
}

static int stm32_emmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	if (priv->status == DISK_STATUS_OK) {
		return 0;
	}


	err = stm32_sdmmc_clock_enable(priv);
	if (err) {
		LOG_ERR("failed to init clocks");
		return err;
	}

	err = HAL_MMC_Init(&priv->hsd);
	if (err != HAL_OK) {
		LOG_ERR("failed to init stm32_sdmmc");
		return -EIO;
	}

	priv->status = DISK_STATUS_OK;
	return 0;
}

static int stm32_emmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int stm32_emmc_access_read(struct disk_info *disk, uint8_t *data_buf,
				  uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	err = HAL_MMC_ReadBlocks(&priv->hsd, data_buf, start_sector,
				 num_sector, 30000);
	if (err != HAL_OK) {
		LOG_ERR("sd read block failed %d", err);
		return -EIO;
	}

	while (HAL_MMC_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER)
		;

	return 0;
}

static int stm32_emmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	err = HAL_MMC_WriteBlocks(&priv->hsd, (uint8_t *)data_buf, start_sector,
				 num_sector, 30000);
	if (err != HAL_OK) {
		LOG_ERR("sd write block failed %d", err);
		return -EIO;
	}
	while (HAL_MMC_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER)
		;

	return 0;
}

static int stm32_emmc_access_ioctl(struct disk_info *disk, uint8_t cmd,
				    void *buff)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	HAL_MMC_CardInfoTypeDef info;
	int err;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		err = HAL_MMC_GetCardInfo(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockNbr;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		err = HAL_MMC_GetCardInfo(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockSize;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff = 1;
		break;
	case DISK_IOCTL_CTRL_SYNC:
		/* we use a blocking API, so nothing to do for sync */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct disk_operations stm32_emmc_ops = {
	.init = stm32_emmc_access_init,
	.status = stm32_emmc_access_status,
	.read = stm32_emmc_access_read,
	.write = stm32_emmc_access_write,
	.ioctl = stm32_emmc_access_ioctl,
};

static struct disk_info stm32_sdmmc_info = {
	.name = CONFIG_SDMMC_VOLUME_NAME,
	.ops = &stm32_emmc_ops,
};

static int stm32_sdmmc_reset_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->rst.name) {
		return 0;
	}

	priv->rst.port = device_get_binding(priv->rst.name);
	if (!priv->rst.port) {
		return -ENODEV;
	}

	err = gpio_pin_configure(priv->rst.port, priv->rst.pin,
				 priv->rst.flags | GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	k_sleep(K_MSEC(50));

	return 0;
}

static int stm32_sdmmc_reset_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->rst.name) {
		return 0;
	}

	gpio_pin_configure(priv->rst.port, priv->rst.pin, GPIO_DISCONNECTED);
	return 0;
}

static int disk_stm32_emmc_init(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	/* Configure dt provided device signals when available */
	err = stm32_dt_pinctrl_configure(priv->pinctrl.list,
					 priv->pinctrl.len,
					 (uint32_t)priv->hsd.Instance);
	if (err < 0) {
		return err;
	}

	err = stm32_sdmmc_reset_init(priv);
	if (err) {
		return err;
	}

	priv->status = DISK_STATUS_UNINIT;

	stm32_sdmmc_info.dev = dev;
	err = disk_access_register(&stm32_sdmmc_info);
	if (err) {
		goto err_reset;
	}
	return 0;

err_reset:
	stm32_sdmmc_reset_uninit(priv);
	return err;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static const struct soc_gpio_pinctrl sdmmc_pins_1[] =
						ST_STM32_DT_INST_PINCTRL(0, 0);

static struct stm32_sdmmc_priv stm32_sdmmc_priv_1 = {
	.hsd = {
		.Instance = (SDMMC_TypeDef *)DT_INST_REG_ADDR(0),
	},
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	.rst = {
		.name = DT_INST_GPIO_LABEL(0, reset_gpios),
		.pin = DT_INST_GPIO_PIN(0, reset_gpios),
		.flags = DT_INST_GPIO_FLAGS(0, reset_gpios),
	},
#endif
	.pclken = {
		.bus = DT_INST_CLOCKS_CELL(0, bus),
		.enr = DT_INST_CLOCKS_CELL(0, bits),
	},
	.pinctrl = {
		.list = sdmmc_pins_1,
		.len = ARRAY_SIZE(sdmmc_pins_1)
	}
};

DEVICE_DT_INST_DEFINE(0, disk_stm32_emmc_init, NULL,
		    &stm32_sdmmc_priv_1, NULL, POST_KERNEL,
		    CONFIG_SDMMC_INIT_PRIORITY,
		    NULL);
#endif
