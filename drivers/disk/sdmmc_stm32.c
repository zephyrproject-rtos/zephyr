/*
 * Copyright (c) 2020 Amarula Solutions.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdmmc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/disk/sdmmc_stm32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <stm32_ll_rcc.h>

LOG_MODULE_REGISTER(stm32_sdmmc, CONFIG_SDMMC_LOG_LEVEL);

#define STM32_SDMMC_USE_DMA DT_NODE_HAS_PROP(DT_DRV_INST(0), dmas)

#if STM32_SDMMC_USE_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <stm32_ll_dma.h>
/* Uses a shared DMA channel for read & write */
#define STM32_SDMMC_USE_DMA_SHARED DT_INST_DMAS_HAS_NAME(0, txrx)
#endif

#ifndef MMC_TypeDef
#define MMC_TypeDef SDMMC_TypeDef
#endif

#ifndef SDMMC_BUS_WIDE_1B
#define SDMMC_BUS_WIDE_1B SDIO_BUS_WIDE_1B
#endif

#ifndef SDMMC_BUS_WIDE_4B
#define SDMMC_BUS_WIDE_4B SDIO_BUS_WIDE_4B
#endif

#ifndef SDMMC_BUS_WIDE_8B
#define SDMMC_BUS_WIDE_8B SDIO_BUS_WIDE_8B
#endif

#ifndef SDMMC_CLOCK_EDGE_RISING
#define SDMMC_CLOCK_EDGE_RISING SDIO_CLOCK_EDGE_RISING
#endif

#ifndef SDMMC_CLOCK_POWER_SAVE_ENABLE
#define SDMMC_CLOCK_POWER_SAVE_ENABLE SDIO_CLOCK_POWER_SAVE_ENABLE
#endif

#ifndef SDMMC_CLOCK_POWER_SAVE_DISABLE
#define SDMMC_CLOCK_POWER_SAVE_DISABLE SDIO_CLOCK_POWER_SAVE_DISABLE
#endif

#ifndef SDMMC_HARDWARE_FLOW_CONTROL_DISABLE
#define SDMMC_HARDWARE_FLOW_CONTROL_DISABLE SDIO_HARDWARE_FLOW_CONTROL_DISABLE
#endif

typedef void (*irq_config_func_t)(const struct device *dev);

#if STM32_SDMMC_USE_DMA

static const uint32_t table_priority[] = {
	DMA_PRIORITY_LOW,
	DMA_PRIORITY_MEDIUM,
	DMA_PRIORITY_HIGH,
	DMA_PRIORITY_VERY_HIGH
};

struct sdmmc_dma_stream {
	const struct device *dev;
	uint32_t channel;
	uint32_t channel_nb;
	DMA_TypeDef *reg;
	struct dma_config cfg;
};
#endif

#ifdef CONFIG_SDMMC_STM32_EMMC
typedef MMC_HandleTypeDef HandleTypeDef;
typedef HAL_MMC_CardInfoTypeDef CardInfoTypeDef;
#else
typedef SD_HandleTypeDef HandleTypeDef;
typedef HAL_SD_CardInfoTypeDef CardInfoTypeDef;
#endif

struct stm32_sdmmc_priv {
	irq_config_func_t irq_config;
	struct k_sem thread_lock;
	struct k_sem sync;
	HandleTypeDef hsd;
	int status;
	struct k_work work;
	struct gpio_callback cd_cb;
	struct gpio_dt_spec cd;
	struct gpio_dt_spec pe;
	struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pcfg;
	const struct reset_dt_spec reset;

#if STM32_SDMMC_USE_DMA
#if STM32_SDMMC_USE_DMA_SHARED
	struct sdmmc_dma_stream dma_txrx;
	DMA_HandleTypeDef dma_txrx_handle;
#else
	struct sdmmc_dma_stream dma_rx;
	struct sdmmc_dma_stream dma_tx;
	DMA_HandleTypeDef dma_tx_handle;
	DMA_HandleTypeDef dma_rx_handle;
#endif
#endif /* STM32_SDMMC_USE_DMA */
};

#ifdef CONFIG_SDMMC_STM32_HWFC
static void stm32_sdmmc_fc_enable(struct stm32_sdmmc_priv *priv)
{
	MMC_TypeDef *sdmmcx = priv->hsd.Instance;

	sdmmcx->CLKCR |= SDMMC_CLKCR_HWFC_EN;
}
#endif

static void stm32_sdmmc_isr(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;

#ifdef CONFIG_SDMMC_STM32_EMMC
	HAL_MMC_IRQHandler(&priv->hsd);
#else
	HAL_SD_IRQHandler(&priv->hsd);
#endif
}

#define DEFINE_HAL_CALLBACK(name)                                                                  \
	void name(HandleTypeDef *hsd)                                                           \
	{                                                                                          \
		struct stm32_sdmmc_priv *priv = CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);   \
                                                                                                   \
		priv->status = hsd->ErrorCode;                                                     \
                                                                                                   \
		k_sem_give(&priv->sync);                                                           \
	}

#ifdef CONFIG_SDMMC_STM32_EMMC
DEFINE_HAL_CALLBACK(HAL_MMC_TxCpltCallback);
DEFINE_HAL_CALLBACK(HAL_MMC_RxCpltCallback);
DEFINE_HAL_CALLBACK(HAL_MMC_ErrorCallback);
#else
DEFINE_HAL_CALLBACK(HAL_SD_TxCpltCallback);
DEFINE_HAL_CALLBACK(HAL_SD_RxCpltCallback);
DEFINE_HAL_CALLBACK(HAL_SD_ErrorCallback);
#endif

static int stm32_sdmmc_clock_enable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

	/* HSI48 Clock is enabled through using the device tree */
	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clock,
					    (clock_control_subsys_t)&priv->pclken[1],
					    NULL) != 0) {
			LOG_ERR("Failed to enable SDMMC domain clock");
			return -EIO;
		}
	}

	if (IS_ENABLED(CONFIG_SDMMC_STM32_CLOCK_CHECK)) {
		uint32_t sdmmc_clock_rate;

		if (clock_control_get_rate(clock,
					   (clock_control_subsys_t)&priv->pclken[1],
					   &sdmmc_clock_rate) != 0) {
			LOG_ERR("Failed to get SDMMC domain clock rate");
			return -EIO;
		}

		if (sdmmc_clock_rate != MHZ(48)) {
			LOG_ERR("SDMMC Clock is not 48MHz (%d)", sdmmc_clock_rate);
			return -ENOTSUP;
		}
	}

	/* Enable the APB clock for stm32_sdmmc */
	return clock_control_on(clock, (clock_control_subsys_t)&priv->pclken[0]);
}

#if !defined(CONFIG_SDMMC_STM32_EMMC)
static int stm32_sdmmc_clock_disable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	return clock_control_off(clock, (clock_control_subsys_t)&priv->pclken[0]);
}
#endif

#if STM32_SDMMC_USE_DMA

static void stm32_sdmmc_dma_cb(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);

	}

	HAL_DMA_IRQHandler(hdma);
}

static int stm32_sdmmc_configure_dma(DMA_HandleTypeDef *handle, struct sdmmc_dma_stream *dma)
{
	int ret;

	if (!device_is_ready(dma->dev)) {
		LOG_ERR("Failed to get dma dev");
		return -ENODEV;
	}

	dma->cfg.user_data = handle;

	/* Reserve the channel in the DMA subsystem, even though we use the HAL API.
	 * See the usage of STM32_DMA_HAL_OVERRIDE.
	 */
	ret = dma_config(dma->dev, dma->channel, &dma->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to conig");
		return ret;
	}

	handle->Instance                 = STM32_DMA_GET_INSTANCE(dma->reg, dma->channel_nb);
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v1)
	handle->Init.Channel             = dma->cfg.dma_slot * DMA_CHANNEL_1;
	handle->Init.PeriphInc           = DMA_PINC_DISABLE;
	handle->Init.MemInc              = DMA_MINC_ENABLE;
	handle->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	handle->Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	handle->Init.Mode                = DMA_PFCTRL;
	handle->Init.Priority            = table_priority[dma->cfg.channel_priority];
	handle->Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	handle->Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	handle->Init.MemBurst            = DMA_MBURST_INC4;
	handle->Init.PeriphBurst         = DMA_PBURST_INC4;
#else
	BUILD_ASSERT(STM32_SDMMC_USE_DMA_SHARED == 1, "Only txrx is supported on this family");
	/* handle->Init.Direction is not initialised here on purpose.
	 * Since the channel is reused for both directions, the direction is
	 * configured before each read/write call.
	 */
	handle->Init.Request             = dma->cfg.dma_slot;
	handle->Init.PeriphInc           = DMA_PINC_DISABLE;
	handle->Init.MemInc              = DMA_MINC_ENABLE;
	handle->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	handle->Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	handle->Init.Mode                = DMA_NORMAL;
	handle->Init.Priority            = table_priority[dma->cfg.channel_priority];
#endif

	return ret;
}

static int stm32_sdmmc_dma_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	LOG_DBG("using dma");

#if STM32_SDMMC_USE_DMA_SHARED
	err = stm32_sdmmc_configure_dma(&priv->dma_txrx_handle, &priv->dma_txrx);
	if (err) {
		LOG_ERR("failed to init shared DMA");
		return err;
	}
	__HAL_LINKDMA(&priv->hsd, hdmatx, priv->dma_txrx_handle);
	__HAL_LINKDMA(&priv->hsd, hdmarx, priv->dma_txrx_handle);
#else
	err = stm32_sdmmc_configure_dma(&priv->dma_tx_handle, &priv->dma_tx);
	if (err) {
		LOG_ERR("failed to init tx dma");
		return err;
	}
	__HAL_LINKDMA(&priv->hsd, hdmatx, priv->dma_tx_handle);
	HAL_DMA_Init(&priv->dma_tx_handle);

	err = stm32_sdmmc_configure_dma(&priv->dma_rx_handle, &priv->dma_rx);
	if (err) {
		LOG_ERR("failed to init rx dma");
		return err;
	}
	__HAL_LINKDMA(&priv->hsd, hdmarx, priv->dma_rx_handle);
	HAL_DMA_Init(&priv->dma_rx_handle);
#endif /* STM32_SDMMC_USE_DMA_SHARED */

	return err;
}

static int stm32_sdmmc_dma_deinit(struct stm32_sdmmc_priv *priv)
{
	int ret;

	/* Since we use STM32_DMA_HAL_OVERRIDE, the only purpose of dma_stop
	 * is to notify the DMA subsystem that the channel is no longer in use.
	 * Calling this before or after HAL_DMA_DeInit makes no difference.
	 * There is no possibility of runtime failures apart from providing an
	 * invalid channel ID, which is already validated by the setup.
	 */
#if STM32_SDMMC_USE_DMA_SHARED
	struct sdmmc_dma_stream *dma_txrx = &priv->dma_txrx;

	ret = dma_stop(dma_txrx->dev, dma_txrx->channel);
	__ASSERT(ret == 0, "Shared DMA channel index corrupted");
#else
	struct sdmmc_dma_stream *dma_tx = &priv->dma_tx;
	struct sdmmc_dma_stream *dma_rx = &priv->dma_rx;

	ret = dma_stop(dma_tx->dev, dma_tx->channel);
	__ASSERT(ret == 0, "TX DMA channel index corrupted");
	HAL_DMA_DeInit(&priv->dma_tx_handle);

	ret = dma_stop(dma_rx->dev, dma_rx->channel);
	__ASSERT(ret == 0, "RX DMA channel index corrupted");
	HAL_DMA_DeInit(&priv->dma_rx_handle);
#endif
	return 0;
}

#endif

/* Forward declarations */
static int stm32_sdmmc_pwr_on(struct stm32_sdmmc_priv *priv);
static void stm32_sdmmc_pwr_off(struct stm32_sdmmc_priv *priv);
static int stm32_sdmmc_card_detect_init(struct stm32_sdmmc_priv *priv);
static bool stm32_sdmmc_card_present(struct stm32_sdmmc_priv *priv);
static int stm32_sdmmc_card_detect_uninit(struct stm32_sdmmc_priv *priv);

static int stm32_sdmmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	err = stm32_sdmmc_pwr_on(priv);
	if (err) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(priv->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		stm32_sdmmc_pwr_off(priv);
		return err;
	}

#if !defined(CONFIG_SDMMC_STM32_EMMC)
	err = stm32_sdmmc_card_detect_init(priv);
	if (err) {
		stm32_sdmmc_pwr_off(priv);
		return err;
	}
#endif

	if (!stm32_sdmmc_card_present(priv)) {
		priv->status = DISK_STATUS_NOMEDIA;
		err = -ENODEV;
		goto error;
	}

	priv->status = DISK_STATUS_UNINIT;

#if STM32_SDMMC_USE_DMA
	err = stm32_sdmmc_dma_init(priv);
	if (err) {
		LOG_ERR("DMA init failed");
		goto error;
	}
#endif

	err = stm32_sdmmc_clock_enable(priv);
	if (err) {
		LOG_ERR("failed to init clocks");
		goto error;
	}

	err = reset_line_toggle_dt(&priv->reset);
	if (err) {
		LOG_ERR("failed to reset peripheral");
		goto error;
	}

#ifdef CONFIG_SDMMC_STM32_EMMC
	err = HAL_MMC_Init(&priv->hsd);
#else
	err = HAL_SD_Init(&priv->hsd);
#endif
	if (err != HAL_OK) {
		LOG_ERR("failed to init stm32_sdmmc (ErrorCode 0x%X)", priv->hsd.ErrorCode);
		err = -EIO;
		goto error;
	}

#ifdef CONFIG_SDMMC_STM32_HWFC
	stm32_sdmmc_fc_enable(priv);
#endif

	priv->status = DISK_STATUS_OK;
	return 0;
error:
	stm32_sdmmc_card_detect_uninit(priv);
	stm32_sdmmc_pwr_off(priv);
	return err;
}

static int stm32_sdmmc_access_deinit(struct stm32_sdmmc_priv *priv)
{
	int err = 0;

#if STM32_SDMMC_USE_DMA
	err = stm32_sdmmc_dma_deinit(priv);
	if (err) {
		LOG_ERR("DMA deinit failed");
		return err;
	}
#endif

#if defined(CONFIG_SDMMC_STM32_EMMC)
	err = HAL_MMC_DeInit(&priv->hsd);
#else
	err = HAL_SD_DeInit(&priv->hsd);
	stm32_sdmmc_clock_disable(priv);
#endif
	if (err != HAL_OK) {
		LOG_ERR("failed to deinit stm32_sdmmc (ErrorCode 0x%X)", priv->hsd.ErrorCode);
		return err;
	}

#if !defined(CONFIG_SDMMC_STM32_EMMC)
	stm32_sdmmc_card_detect_uninit(priv);
#endif
	stm32_sdmmc_pwr_off(priv);

	priv->status = DISK_STATUS_UNINIT;
	return 0;
}

static int stm32_sdmmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int stm32_sdmmc_is_card_in_transfer(HandleTypeDef *hsd)
{
#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_GetCardState(hsd) == HAL_MMC_CARD_TRANSFER;
#else
	return HAL_SD_GetCardState(hsd) == HAL_SD_CARD_TRANSFER;
#endif
}

static int stm32_sdmmc_read_blocks(HandleTypeDef *hsd, uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
#if STM32_SDMMC_USE_DMA || IS_ENABLED(DT_PROP(DT_DRV_INST(0), idma))

#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_ReadBlocks_DMA(hsd, data_buf, start_sector, num_sector);
#else
	return HAL_SD_ReadBlocks_DMA(hsd, data_buf, start_sector, num_sector);
#endif

#else

#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_ReadBlocks_IT(hsd, data_buf, start_sector, num_sector);
#else
	return HAL_SD_ReadBlocks_IT(hsd, data_buf, start_sector, num_sector);
#endif

#endif
}

static int stm32_sdmmc_access_read(struct disk_info *disk, uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_sem_take(&priv->thread_lock, K_FOREVER);

#if STM32_SDMMC_USE_DMA_SHARED
	/* Initialise the shared DMA channel for the current direction */
	priv->dma_txrx_handle.Init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
	if (HAL_DMA_Init(&priv->dma_txrx_handle) != HAL_OK) {
		err = -EIO;
		goto end;
	}
#endif

	err = stm32_sdmmc_read_blocks(&priv->hsd, data_buf, start_sector, num_sector);
	if (err != HAL_OK) {
		LOG_ERR("sd read block failed %d", err);
		err = -EIO;
		goto end;
	}

	k_sem_take(&priv->sync, K_FOREVER);

#if STM32_SDMMC_USE_DMA_SHARED
	HAL_DMA_DeInit(&priv->dma_txrx_handle);
#endif

	if (priv->status != DISK_STATUS_OK) {
		LOG_ERR("sd read error %d", priv->status);
		err = -EIO;
		goto end;
	}

	while (!stm32_sdmmc_is_card_in_transfer(&priv->hsd)) {
	}

end:
	k_sem_give(&priv->thread_lock);
	return err;
}

static int stm32_sdmmc_write_blocks(HandleTypeDef *hsd,
				    uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
#if STM32_SDMMC_USE_DMA || IS_ENABLED(DT_PROP(DT_DRV_INST(0), idma))

#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_WriteBlocks_DMA(hsd, data_buf, start_sector, num_sector);
#else
	return HAL_SD_WriteBlocks_DMA(hsd, data_buf, start_sector, num_sector);
#endif

#else

#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_WriteBlocks_IT(hsd, data_buf, start_sector, num_sector);
#else
	return HAL_SD_WriteBlocks_IT(hsd, data_buf, start_sector, num_sector);
#endif

#endif
}

static int stm32_sdmmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_sem_take(&priv->thread_lock, K_FOREVER);

#if STM32_SDMMC_USE_DMA_SHARED
	/* Initialise the shared DMA channel for the current direction */
	priv->dma_txrx_handle.Init.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
	if (HAL_DMA_Init(&priv->dma_txrx_handle) != HAL_OK) {
		err = -EIO;
		goto end;
	}
#endif

	err = stm32_sdmmc_write_blocks(&priv->hsd, (uint8_t *)data_buf, start_sector, num_sector);
	if (err != HAL_OK) {
		LOG_ERR("sd write block failed %d", err);
		err = -EIO;
		goto end;
	}

	k_sem_take(&priv->sync, K_FOREVER);

#if STM32_SDMMC_USE_DMA_SHARED
	HAL_DMA_DeInit(&priv->dma_txrx_handle);
#endif

	if (priv->status != DISK_STATUS_OK) {
		LOG_ERR("sd write error %d", priv->status);
		err = -EIO;
		goto end;
	}

	while (!stm32_sdmmc_is_card_in_transfer(&priv->hsd)) {
	}

end:
	k_sem_give(&priv->thread_lock);
	return err;
}

static int stm32_sdmmc_get_card_info(HandleTypeDef *hsd, CardInfoTypeDef *info)
{
#ifdef CONFIG_SDMMC_STM32_EMMC
	return HAL_MMC_GetCardInfo(hsd, info);
#else
	return HAL_SD_GetCardInfo(hsd, info);
#endif
}

static int stm32_sdmmc_access_ioctl(struct disk_info *disk, uint8_t cmd,
				    void *buff)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	CardInfoTypeDef info;
	int err;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		err = stm32_sdmmc_get_card_info(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockNbr;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		err = stm32_sdmmc_get_card_info(&priv->hsd, &info);
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
	case DISK_IOCTL_CTRL_INIT:
		return stm32_sdmmc_access_init(disk);
	case DISK_IOCTL_CTRL_DEINIT:
		return stm32_sdmmc_access_deinit(priv);
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
	.name = DT_INST_PROP_OR(0, disk_name, "SD"),
	.ops = &stm32_sdmmc_ops,
};


#ifdef CONFIG_SDMMC_STM32_EMMC
static bool stm32_sdmmc_card_present(struct stm32_sdmmc_priv *priv)
{
	return true;
}
#else /* CONFIG_SDMMC_STM32_EMMC */
/*
 * Check if the card is present or not. If no card detect gpio is set, assume
 * the card is present. If reading the gpio fails for some reason, assume the
 * card is there.
 */
static bool stm32_sdmmc_card_present(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->cd.port) {
		return true;
	}

	err = gpio_pin_get_dt(&priv->cd);
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

	if (!priv->cd.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&priv->cd)) {
		return -ENODEV;
	}

	gpio_init_callback(&priv->cd_cb, stm32_sdmmc_cd_callback,
			   1 << priv->cd.pin);

	err = gpio_add_callback(priv->cd.port, &priv->cd_cb);
	if (err) {
		return err;
	}

	err = gpio_pin_configure_dt(&priv->cd, GPIO_INPUT);
	if (err) {
		goto remove_callback;
	}

	err = gpio_pin_interrupt_configure_dt(&priv->cd, GPIO_INT_EDGE_BOTH);
	if (err) {
		goto unconfigure_pin;
	}
	return 0;

unconfigure_pin:
	gpio_pin_configure_dt(&priv->cd, GPIO_DISCONNECTED);
remove_callback:
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return err;
}

static int stm32_sdmmc_card_detect_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->cd.port) {
		return 0;
	}

	gpio_pin_interrupt_configure_dt(&priv->cd, GPIO_INT_MODE_DISABLED);
	gpio_pin_configure_dt(&priv->cd, GPIO_DISCONNECTED);
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return 0;
}
#endif /* !CONFIG_SDMMC_STM32_EMMC */

static int stm32_sdmmc_pwr_on(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->pe.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&priv->pe)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&priv->pe, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	k_sleep(K_MSEC(50));

	return 0;
}

static void stm32_sdmmc_pwr_off(struct stm32_sdmmc_priv *priv)
{
	int ret;

	if (!priv->pe.port) {
		return;
	}

	/* PINCTRL sleep mode when powered down */
	ret = pinctrl_apply_state(priv->pcfg, PINCTRL_STATE_SLEEP);
	if (ret != 0 && ret != ENOTSUP) {
		LOG_WRN("Failed to configure pins for sleep (%d)", ret);
	}
	gpio_pin_configure_dt(&priv->pe, GPIO_OUTPUT_INACTIVE);
}

static int disk_stm32_sdmmc_init(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(priv->reset.dev)) {
		LOG_ERR("reset control device not ready");
		return -ENODEV;
	}

	priv->irq_config(dev);

	/* Initialize semaphores */
	k_sem_init(&priv->thread_lock, 1, 1);
	k_sem_init(&priv->sync, 0, 1);

#if !defined(CONFIG_SDMMC_STM32_EMMC)
	k_work_init(&priv->work, stm32_sdmmc_cd_handler);
#endif

	/* Ensure off by default */
	stm32_sdmmc_pwr_off(priv);

	stm32_sdmmc_info.dev = dev;
	return disk_access_register(&stm32_sdmmc_info);
}

void stm32_sdmmc_get_card_cid(const struct device *dev, uint32_t cid[4])
{
	const struct stm32_sdmmc_priv *priv = dev->data;

	memcpy(cid, &priv->hsd.CID, sizeof(priv->hsd.CID));
}

#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0))

#if STM32_SDMMC_USE_DMA

#define SDMMC_DMA_CHANNEL_INIT(dir, dir_cap)				\
	.dev = DEVICE_DT_GET(STM32_DMA_CTLR(0, dir)),			\
	.channel = DT_INST_DMAS_CELL_BY_NAME(0, dir, channel),		\
	.channel_nb = DT_DMAS_CELL_BY_NAME(				\
			DT_DRV_INST(0), dir, channel),			\
	.reg = (DMA_TypeDef *)DT_REG_ADDR(				\
			DT_PHANDLE_BY_NAME(DT_DRV_INST(0), dmas, dir)),	\
	.cfg = {							\
		.dma_slot = STM32_DMA_SLOT(0, dir, slot),		\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				STM32_DMA_CHANNEL_CONFIG(0, dir)),	\
		.dma_callback = stm32_sdmmc_dma_cb,			\
		.linked_channel = STM32_DMA_HAL_OVERRIDE,		\
	},								\


#define SDMMC_DMA_CHANNEL(dir, DIR)					\
.dma_##dir = {								\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(0, dir),			\
		 (SDMMC_DMA_CHANNEL_INIT(dir, DIR)),			\
		 (NULL))						\
	},

#else
#define SDMMC_DMA_CHANNEL(dir, DIR)
#endif

PINCTRL_DT_INST_DEFINE(0);

static void stm32_sdmmc_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		stm32_sdmmc_isr, DEVICE_DT_INST_GET(0),
		0);
	irq_enable(DT_INST_IRQN(0));
}

#if DT_INST_PROP(0, bus_width) == 1
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_1B
#elif DT_INST_PROP(0, bus_width) == 4
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_4B
#elif DT_INST_PROP(0, bus_width) == 8
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_8B
#endif /* DT_INST_PROP(0, bus_width) */

static struct stm32_pclken pclken_sdmmc[] = STM32_DT_INST_CLOCKS(0);

static struct stm32_sdmmc_priv stm32_sdmmc_priv_1 = {
	.irq_config = stm32_sdmmc_irq_config_func,
	.hsd = {
		.Instance = (MMC_TypeDef *)DT_INST_REG_ADDR(0),
		.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING,
#ifdef SDIO_CLOCK_BYPASS_DISABLE
		.Init.ClockBypass = DT_INST_PROP(0, clk_bypass)
						? SDIO_CLOCK_BYPASS_ENABLE
						: SDIO_CLOCK_BYPASS_DISABLE,
#elif defined(SDMMC_CLOCK_BYPASS_DISABLE)
		.Init.ClockBypass = DT_INST_PROP(0, clk_bypass)
						? SDMMC_CLOCK_BYPASS_ENABLE
						: SDMMC_CLOCK_BYPASS_DISABLE,
#endif
		.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE,
		.Init.BusWide = SDMMC_BUS_WIDTH,
		.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE,
		.Init.ClockDiv = DT_INST_PROP_OR(0, clk_div, 0),
	},
#if DT_INST_NODE_HAS_PROP(0, cd_gpios)
	.cd = GPIO_DT_SPEC_INST_GET(0, cd_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, pwr_gpios)
	.pe = GPIO_DT_SPEC_INST_GET(0, pwr_gpios),
#endif
	.pclken = pclken_sdmmc,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.reset = RESET_DT_SPEC_INST_GET(0),
#if STM32_SDMMC_USE_DMA_SHARED
	SDMMC_DMA_CHANNEL(txrx, TXRX)
#else
	SDMMC_DMA_CHANNEL(rx, RX)
	SDMMC_DMA_CHANNEL(tx, TX)
#endif
};

DEVICE_DT_INST_DEFINE(0, disk_stm32_sdmmc_init, NULL,
		    &stm32_sdmmc_priv_1, NULL, POST_KERNEL,
		    CONFIG_SD_INIT_PRIORITY,
		    NULL);
#endif
