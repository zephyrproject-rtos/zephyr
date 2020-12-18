/*
 * Copyright (c) 2020 Amarula Solutions.
 * Copyright (c) 2021 Filip Zawadiak <fzawadiak@gmail.com>
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

typedef void (*irq_config_func_t)(const struct device *dev);

struct stm32_sdmmc_priv {
	SD_HandleTypeDef hsd;
	struct k_sem sem;
	struct k_sem sync;
	int status;
	struct k_work work;
	struct gpio_callback cd_cb;
	irq_config_func_t irq_config;
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
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
#if defined(CONFIG_DISK_ACCESS_STM32_CLOCK_SOURCE_PLL1_Q)
	LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC_CLKSOURCE_PLL1Q);
#elif defined(CONFIG_DISK_ACCESS_STM32_CLOCK_SOURCE_PLL2_R)
	LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC_CLKSOURCE_PLL2R);
#endif /* CONFIG_SOC_SERIES_STM32H7X */
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

	/* Initialize semaphore */
	k_sem_init(&priv->sem, 1, 1);
	k_sem_init(&priv->sync, 0, 1);

	/* Run IRQ init */
	priv->irq_config(dev);

	priv->hsd.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	priv->hsd.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	priv->hsd.Init.BusWide = SDMMC_BUS_WIDE_4B;
	priv->hsd.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	priv->hsd.Init.ClockDiv = 1;
#ifdef CONFIG_SOC_SERIES_STM32H7X
	priv->hsd.Init.TranceiverPresent = SDMMC_TRANSCEIVER_NOT_PRESENT;
#endif

	err = HAL_SD_Init(&priv->hsd);
	if (err != HAL_OK) {
		LOG_ERR("failed to init stm32_sdmmc");
		return -EIO;
	}

#ifdef CONFIG_SOC_SERIES_STM32H7X
	err = HAL_SD_ConfigWideBusOperation(&priv->hsd, SDMMC_BUS_WIDE_4B);
	if (err != HAL_OK) {
		LOG_ERR("failed to enable wide bus stm32_sdmmc");
		return -EIO;
	}

	err = HAL_SD_ConfigSpeedBusOperation(&priv->hsd, SDMMC_SPEED_MODE_HIGH);
	if (err != HAL_OK) {
		LOG_ERR("failed to enable high speed bus stm32_sdmmc");
		return -EIO;
	}
#endif
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

	k_sem_take(&priv->sem, K_FOREVER);

#ifdef STM32_SDMMC_USE_DMA
	/* Clean cache first to be secure when buffers are not */
	/* 32 byte aligned and allocated on stack */
	uint32_t aligned_buf = (uint32_t)data_buf & ~0x1F;

	SCB_CleanDCache_by_Addr((uint32_t *)aligned_buf,
		num_sector*512 + ((uint32_t)data_buf - aligned_buf));

	err = HAL_SD_ReadBlocks_DMA(&priv->hsd, data_buf, start_sector,
				num_sector);
#else
	err = HAL_SD_ReadBlocks_IT(&priv->hsd, data_buf, start_sector,
				num_sector);
#endif /* STM32_SDMMC_USE_DMA */

	if (err != HAL_OK) {
		k_sem_give(&priv->sem);
		LOG_ERR("sd read block failed %d", err);
		return -EIO;
	}

	k_sem_take(&priv->sync, K_FOREVER);

#ifdef STM32_SDMMC_USE_DMA
	/* Invalidate buffers */
	SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_buf,
		num_sector*512 + ((uint32_t)data_buf - aligned_buf));
#endif /* STM32_SDMMC_USE_DMA */

	if (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER) {
		k_sem_give(&priv->sem);
		return -EIO;
	}

	k_sem_give(&priv->sem);

	return 0;
}

static int stm32_sdmmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_sem_take(&priv->sem, K_FOREVER);

#ifdef STM32_SDMMC_USE_DMA
	/* Flush cache to RAM to make sure DMA sees the same data */
	uint32_t aligned_buf = (uint32_t)data_buf & ~0x1F;

	SCB_CleanDCache_by_Addr((uint32_t *)aligned_buf,
		num_sector*512 + ((uint32_t)data_buf - aligned_buf));

	err = HAL_SD_WriteBlocks_DMA(&priv->hsd, (uint8_t *)data_buf, start_sector,
				     num_sector);
#else
	err = HAL_SD_WriteBlocks_IT(&priv->hsd, (uint8_t *)data_buf, start_sector,
				     num_sector);
#endif
	if (err != HAL_OK) {
		k_sem_give(&priv->sem);
		LOG_ERR("sd write block failed %d", err);
		return -EIO;
	}

	k_sem_take(&priv->sync, K_FOREVER);

	if (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER) {
		k_sem_give(&priv->sem);
		return -EIO;
	}

	k_sem_give(&priv->sem);

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
		k_sem_take(&priv->sem, K_FOREVER);
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
		k_sem_give(&priv->sem);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockNbr;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		k_sem_take(&priv->sem, K_FOREVER);
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
		k_sem_give(&priv->sem);
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

static void disk_stm32_sdmmc_isr(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;

	HAL_SD_IRQHandler(&priv->hsd);
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
	CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	k_sem_give(&priv->sync);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
	CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	k_sem_give(&priv->sync);
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
	CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	k_sem_give(&priv->sync);
}

#define STM32_SDMMC_GPIO_CONFIG(id, func)				\
		.name = DT_INST_GPIO_LABEL(id, func##_gpios),		\
		.pin = DT_INST_GPIO_PIN(id, func##_gpios),		\
		.flags = DT_INST_GPIO_FLAGS(id, func##_gpios),		\

#define STM32_SDMMC_GPIO(id, func)					\
	.func = {							\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(id, func##_gpios),	\
		(STM32_SDMMC_GPIO_CONFIG(id, func)),			\
		(NULL))							\
	}

#define STM32_SDMMC_INIT(id)						\
									\
static void sdmmc_irqconfig_##id(const struct device *dev)		\
{									\
	IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority),	\
		    disk_stm32_sdmmc_isr, DEVICE_DT_INST_GET(id), 0);	\
	irq_enable(DT_INST_IRQN(0));					\
}									\
									\
static const struct soc_gpio_pinctrl sdmmc_pins_##id[] =		\
	ST_STM32_DT_INST_PINCTRL(id, 0);				\
									\
static struct stm32_sdmmc_priv stm32_sdmmc_priv_##id = {		\
	.hsd = {							\
		.Instance = (SDMMC_TypeDef *)DT_INST_REG_ADDR(id),	\
	},								\
	.pclken = {							\
		.bus = DT_INST_CLOCKS_CELL(id, bus),			\
		.enr = DT_INST_CLOCKS_CELL(id, bits),			\
	},								\
	.pinctrl = {							\
		.list = sdmmc_pins_##id,				\
		.len = ARRAY_SIZE(sdmmc_pins_##id)			\
	},								\
	.irq_config = sdmmc_irqconfig_##id,				\
	STM32_SDMMC_GPIO(id, cd),					\
	STM32_SDMMC_GPIO(id, pe),					\
};									\
									\
DEVICE_DT_INST_DEFINE(id, disk_stm32_sdmmc_init, device_pm_control_nop,	\
		    &stm32_sdmmc_priv_##id, NULL, APPLICATION,		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		    NULL)

DT_INST_FOREACH_STATUS_OKAY(STM32_SDMMC_INIT);
