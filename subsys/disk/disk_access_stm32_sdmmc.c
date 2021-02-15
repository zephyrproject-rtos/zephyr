/*
 * Copyright (c) 2020 Amarula Solutions.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdmmc

#include <devicetree.h>
#include <disk/disk_access.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <soc.h>
#include <stm32_ll_rcc.h>

LOG_MODULE_REGISTER(stm32_sdmmc);

struct stm32_sdmmc_priv {
	SD_HandleTypeDef hsd;
	int status;
	struct k_work work;
	struct gpio_callback cd_cb;
	struct {
		const char *name;
		const struct device *port;
		int pin;
		int flags;
	} cd;
	struct {
		const char *name;
		const struct device *port;
		int pin;
		int flags;
	} pe;
	struct stm32_pclken pclken;
	struct {
		const struct soc_gpio_pinctrl *list;
		size_t len;
	} pinctrl;
};

static int stm32_sdmmc_clock_enable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

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

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	if (!clock) {
		return -ENODEV;
	}

	/* Enable the APB clock for stm32_sdmmc */
	return clock_control_on(clock, (clock_control_subsys_t *)&priv->pclken);
}

static int stm32_sdmmc_clock_disable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	if (!clock) {
		return -ENODEV;
	}

	return clock_control_off(clock,
				 (clock_control_subsys_t *)&priv->pclken);
}

static int stm32_sdmmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	if (priv->status == DISK_STATUS_OK) {
		return 0;
	}

	if (priv->status == DISK_STATUS_NOMEDIA) {
		return -ENODEV;
	}

	err = stm32_sdmmc_clock_enable(priv);
	if (err) {
		LOG_ERR("failed to init clocks");
		return err;
	}

	err = HAL_SD_Init(&priv->hsd);
	if (err != HAL_OK) {
		LOG_ERR("failed to init stm32_sdmmc");
		return -EIO;
	}

	priv->status = DISK_STATUS_OK;
	return 0;
}

static void stm32_sdmmc_access_deinit(struct stm32_sdmmc_priv *priv)
{
	HAL_SD_DeInit(&priv->hsd);
	stm32_sdmmc_clock_disable(priv);
}

static int stm32_sdmmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int stm32_sdmmc_access_read(struct disk_info *disk, uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	err = HAL_SD_ReadBlocks(&priv->hsd, data_buf, start_sector,
				num_sector, 30000);
	if (err != HAL_OK) {
		LOG_ERR("sd read block failed %d", err);
		return -EIO;
	}

	while (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER)
		;

	return 0;
}

static int stm32_sdmmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	err = HAL_SD_WriteBlocks(&priv->hsd, (uint8_t *)data_buf, start_sector,
				 num_sector, 30000);
	if (err != HAL_OK) {
		LOG_ERR("sd write block failed %d", err);
		return -EIO;
	}
	while (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER)
		;

	return 0;
}

static int stm32_sdmmc_access_ioctl(struct disk_info *disk, uint8_t cmd,
				    void *buff)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	HAL_SD_CardInfoTypeDef info;
	int err;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockNbr;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
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

static const struct disk_operations stm32_sdmmc_ops = {
	.init = stm32_sdmmc_access_init,
	.status = stm32_sdmmc_access_status,
	.read = stm32_sdmmc_access_read,
	.write = stm32_sdmmc_access_write,
	.ioctl = stm32_sdmmc_access_ioctl,
};

static struct disk_info stm32_sdmmc_info = {
	.name = CONFIG_DISK_STM32_SDMMC_VOLUME_NAME,
	.ops = &stm32_sdmmc_ops,
};

/*
 * Check if the card is present or not. If no card detect gpio is set, assume
 * the card is present. If reading the gpio fails for some reason, assume the
 * card is there.
 */
static bool stm32_sdmmc_card_present(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->cd.name) {
		return true;
	}

	err = gpio_pin_get(priv->cd.port, priv->cd.pin);
	if (err < 0) {
		LOG_WRN("reading card detect failed %d", err);
		return true;
	}
	return err;
}

static void stm32_sdmmc_cd_handler(struct k_work *item)
{
	struct stm32_sdmmc_priv *priv = CONTAINER_OF(item,
						     struct stm32_sdmmc_priv,
						     work);

	if (stm32_sdmmc_card_present(priv)) {
		LOG_DBG("card inserted");
		priv->status = DISK_STATUS_UNINIT;
	} else {
		LOG_DBG("card removed");
		stm32_sdmmc_access_deinit(priv);
		priv->status = DISK_STATUS_NOMEDIA;
	}
}

static void stm32_sdmmc_cd_callback(const struct device *gpiodev,
				    struct gpio_callback *cb,
				    uint32_t pin)
{
	struct stm32_sdmmc_priv *priv = CONTAINER_OF(cb,
						     struct stm32_sdmmc_priv,
						     cd_cb);

	k_work_submit(&priv->work);
}

static int stm32_sdmmc_card_detect_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->cd.name) {
		return 0;
	}

	priv->cd.port = device_get_binding(priv->cd.name);
	if (!priv->cd.port) {
		return -ENODEV;
	}

	gpio_init_callback(&priv->cd_cb, stm32_sdmmc_cd_callback,
			   1 << priv->cd.pin);

	err = gpio_add_callback(priv->cd.port, &priv->cd_cb);
	if (err) {
		return err;
	}

	err = gpio_pin_configure(priv->cd.port, priv->cd.pin,
				 priv->cd.flags | GPIO_INPUT);
	if (err) {
		goto remove_callback;
	}

	err = gpio_pin_interrupt_configure(priv->cd.port, priv->cd.pin,
					   GPIO_INT_EDGE_BOTH);
	if (err) {
		goto unconfigure_pin;
	}
	return 0;

unconfigure_pin:
	gpio_pin_configure(priv->cd.port, priv->cd.pin, GPIO_DISCONNECTED);
remove_callback:
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return err;
}

static int stm32_sdmmc_card_detect_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->cd.name) {
		return 0;
	}

	gpio_pin_interrupt_configure(priv->cd.port, priv->cd.pin,
				     GPIO_INT_MODE_DISABLED);
	gpio_pin_configure(priv->cd.port, priv->cd.pin, GPIO_DISCONNECTED);
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return 0;
}

static int stm32_sdmmc_pwr_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->pe.name) {
		return 0;
	}

	priv->pe.port = device_get_binding(priv->pe.name);
	if (!priv->pe.port) {
		return -ENODEV;
	}

	err = gpio_pin_configure(priv->pe.port, priv->pe.pin,
				 priv->pe.flags | GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	k_sleep(K_MSEC(50));

	return 0;
}

static int stm32_sdmmc_pwr_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->pe.name) {
		return 0;
	}

	gpio_pin_configure(priv->pe.port, priv->pe.pin, GPIO_DISCONNECTED);
	return 0;
}

static int disk_stm32_sdmmc_init(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_work_init(&priv->work, stm32_sdmmc_cd_handler);

	/* Configure dt provided device signals when available */
	err = stm32_dt_pinctrl_configure(priv->pinctrl.list,
					 priv->pinctrl.len,
					 (uint32_t)priv->hsd.Instance);
	if (err < 0) {
		return err;
	}

	err = stm32_sdmmc_card_detect_init(priv);
	if (err) {
		return err;
	}

	err = stm32_sdmmc_pwr_init(priv);
	if (err) {
		goto err_card_detect;
	}

	if (stm32_sdmmc_card_present(priv)) {
		priv->status = DISK_STATUS_UNINIT;
	} else {
		priv->status = DISK_STATUS_NOMEDIA;
	}

	stm32_sdmmc_info.dev = dev;
	err = disk_access_register(&stm32_sdmmc_info);
	if (err) {
		goto err_pwr;
	}
	return 0;

err_pwr:
	stm32_sdmmc_pwr_uninit(priv);
err_card_detect:
	stm32_sdmmc_card_detect_uninit(priv);
	return err;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static const struct soc_gpio_pinctrl sdmmc_pins_1[] =
						ST_STM32_DT_INST_PINCTRL(0, 0);

static struct stm32_sdmmc_priv stm32_sdmmc_priv_1 = {
	.hsd = {
		.Instance = (SDMMC_TypeDef *)DT_INST_REG_ADDR(0),
	},
#if DT_INST_NODE_HAS_PROP(0, cd_gpios)
	.cd = {
		.name = DT_INST_GPIO_LABEL(0, cd_gpios),
		.pin = DT_INST_GPIO_PIN(0, cd_gpios),
		.flags = DT_INST_GPIO_FLAGS(0, cd_gpios),
	},
#endif
#if DT_INST_NODE_HAS_PROP(0, pwr_gpios)
	.pe = {
		.name = DT_INST_GPIO_LABEL(0, pwr_gpios),
		.pin = DT_INST_GPIO_PIN(0, pwr_gpios),
		.flags = DT_INST_GPIO_FLAGS(0, pwr_gpios),
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

DEVICE_DT_INST_DEFINE(0, disk_stm32_sdmmc_init, device_pm_control_nop,
		    &stm32_sdmmc_priv_1, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    NULL);
#endif
