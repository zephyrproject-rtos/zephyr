/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_sdio

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>

#include <am_mcu_apollo.h>

LOG_MODULE_REGISTER(ambiq_sdio, CONFIG_SDHC_LOG_LEVEL);

#define CACHABLE_START_ADDR SSRAM_BASEADDR

struct ambiq_sdio_config {
	SDIO_Type *pSDHC;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
	uint32_t inst;
	uint8_t tx_delay;
	uint8_t rx_delay;
	uint32_t max_bus_freq;
	uint32_t min_bus_freq;
	uint32_t power_delay_ms;
};

struct ambiq_sdio_data {
	am_hal_card_t card;
	am_hal_card_host_t *host;
	sdhc_interrupt_cb_t sdio_cb;
	void *sdio_cb_user_data;
	struct k_mutex access_mutex;
#ifdef CONFIG_AMBIQ_SDIO_ASYNC
	struct k_sem *async_sem;
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */
};

#ifdef CONFIG_AMBIQ_SDIO_ASYNC
/* Semaphore for SDIO0 Async Data Transfer */
static K_SEM_DEFINE(sdio_async_sem_0, 0, 1);

/*
 * SDIO0 Async Data Transfer Callback Function
 */
static void ambiq_sdio_event_cb_0(am_hal_host_evt_t *pEvt)
{
	am_hal_card_host_t *pHost = (am_hal_card_host_t *)pEvt->pCtx;

	if (AM_HAL_EVT_XFER_COMPLETE == pEvt->eType &&
	    pHost->AsyncCmdData.dir == AM_HAL_DATA_DIR_READ) {
		LOG_DBG("Last Read Xfered block %d\n", pEvt->ui32BlkCnt);
		k_sem_give(&sdio_async_sem_0);
	} else if (AM_HAL_EVT_XFER_COMPLETE == pEvt->eType &&
		   pHost->AsyncCmdData.dir == AM_HAL_DATA_DIR_WRITE) {
		LOG_DBG("Last Write Xfered block %d\n", pEvt->ui32BlkCnt);
		k_sem_give(&sdio_async_sem_0);
	} else if (AM_HAL_EVT_CARD_INT == pEvt->eType) {
		LOG_DBG("SDIO Card Interrupt\n");
	}
}

/* Semaphore for SDIO1 Async Data Transfer */
static K_SEM_DEFINE(sdio_async_sem_1, 0, 1);

/*
 * SDIO1 Async Data Transfer Callback Function
 */
static void ambiq_sdio_event_cb_1(am_hal_host_evt_t *pEvt)
{
	am_hal_card_host_t *pHost = (am_hal_card_host_t *)pEvt->pCtx;

	if (AM_HAL_EVT_XFER_COMPLETE == pEvt->eType &&
	    pHost->AsyncCmdData.dir == AM_HAL_DATA_DIR_READ) {
		LOG_DBG("Last Read Xfered block %d\n", pEvt->ui32BlkCnt);
		k_sem_give(&sdio_async_sem_1);
	} else if (AM_HAL_EVT_XFER_COMPLETE == pEvt->eType &&
		   pHost->AsyncCmdData.dir == AM_HAL_DATA_DIR_WRITE) {
		LOG_DBG("Last Write Xfered block %d\n", pEvt->ui32BlkCnt);
		k_sem_give(&sdio_async_sem_1);
	} else if (AM_HAL_EVT_CARD_INT == pEvt->eType) {
		LOG_DBG("SDIO Card Interrupt\n");
	}
}
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */

/*
 * Ambiq SDIO interrupt service routine
 */
static void ambiq_sdio_isr(const struct device *dev)
{
	uint32_t ui32IntStatus;
	struct ambiq_sdio_data *data = dev->data;

	am_hal_sdhc_intr_status_get(data->host->pHandle, &ui32IntStatus, true);
	am_hal_sdhc_intr_status_clear(data->host->pHandle, ui32IntStatus);
	am_hal_sdhc_interrupt_service(data->host->pHandle, ui32IntStatus);
	if (data->sdio_cb != NULL && data->host->Evt.eType == AM_HAL_EVT_CARD_INT) {
		data->sdio_cb(dev, SDHC_INT_SDIO, data->sdio_cb_user_data);
	}
}

/*
 * Ambiq SDIO host controller software reset
 */
static int ambiq_sdio_reset(const struct device *dev)
{
	const struct ambiq_sdio_config *config = dev->config;
	uint32_t ui32Status = 0;
	int ret = 0;

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	ret = pm_device_runtime_get(dev);

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	LOG_DBG("SDHC Software Reset");
	ui32Status = am_hal_sdhc_software_reset(config->pSDHC, AM_HAL_SDHC_SW_RESET_ALL);
	if (ui32Status) {
		LOG_ERR("SDHC software reset failed, ui32Status = %u", ui32Status);
		ret = -EIO;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return ret;
}

/*
 * Get Ambiq SDIO Host controller properties
 */
static int ambiq_sdio_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct ambiq_sdio_config *config = dev->config;

	LOG_DBG("SDHC get host props");
	memset(props, 0, sizeof(*props));
	props->f_max = config->max_bus_freq;
	props->f_min = config->min_bus_freq;
	props->power_delay = config->power_delay_ms;
	props->host_caps.suspend_res_support = true;
	props->host_caps.adma_2_support = true;
	props->host_caps.sdio_async_interrupt_support = true;
	props->host_caps.vol_180_support = true;
	props->host_caps.bus_4_bit_support = true;
	props->host_caps.bus_8_bit_support = true;
	props->host_caps.high_spd_support = true;
	props->host_caps.sdr50_support = true;
	props->host_caps.sdr104_support = true;
	props->host_caps.ddr50_support = true;
	props->host_caps.hs200_support = true;
	props->max_current_330 = 1020;
	props->max_current_300 = 1020;
	props->max_current_180 = 1020;
	props->is_spi = false;
	return 0;
}

/*
 * Set I/O properties of SDIO host controller
 */
static int ambiq_sdio_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct ambiq_sdio_config *config = dev->config;
	struct ambiq_sdio_data *data = dev->data;
	am_hal_host_bus_voltage_e eBusVoltage;
	am_hal_host_bus_width_e eBusWidth;
	am_hal_host_uhs_mode_e eUHSMode = AM_HAL_HOST_UHS_SDR50;
	uint32_t ui32Status = 0;
	int ret = 0;

	LOG_DBG("(SDIO clock_freq=%d, bus_width=%d, timing_mode=%d, bus_mode=%d)", ios->clock,
		ios->bus_width, ios->timing, ios->bus_mode);

	if (ios->clock != 0 && (ios->clock <= config->max_bus_freq) &&
	    (ios->clock >= config->min_bus_freq)) {
		data->card.cfg.ui32Clock = ios->clock;
	} else if (ios->clock != 0 && (ios->clock > config->max_bus_freq) &&
		   (ios->clock <= MMC_CLOCK_HS200)) {
		/*Limit HS200 frequency to config->max_bus_freq*/
		data->card.cfg.ui32Clock = config->max_bus_freq;
		eUHSMode = AM_HAL_HOST_UHS_SDR104;
	} else if (ios->clock != 0) {
		return -ENOTSUP;
	}

	if (ios->bus_mode != SDHC_BUSMODE_PUSHPULL) {
		return -ENOTSUP;
	}

	switch (ios->bus_width) {
	case SDHC_BUS_WIDTH1BIT:
		eBusWidth = AM_HAL_HOST_BUS_WIDTH_1;
		break;
	case SDHC_BUS_WIDTH4BIT:
		eBusWidth = AM_HAL_HOST_BUS_WIDTH_4;
		break;
	case SDHC_BUS_WIDTH8BIT:
		eBusWidth = AM_HAL_HOST_BUS_WIDTH_8;
		break;
	default:
		return -ENOTSUP;
	}

	switch (ios->signal_voltage) {
	case SD_VOL_3_3_V:
		eBusVoltage = AM_HAL_HOST_BUS_VOLTAGE_3_3;
		break;
	case SD_VOL_3_0_V:
		eBusVoltage = AM_HAL_HOST_BUS_VOLTAGE_3_0;
		break;
	case SD_VOL_1_8_V:
		eBusVoltage = AM_HAL_HOST_BUS_VOLTAGE_1_8;
		break;
	default:
		return -ENOTSUP;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	ret = pm_device_runtime_get(dev);

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	/* Change SDIO Host Bus Voltage */
	if (eBusVoltage != data->card.cfg.eIoVoltage) {
		data->card.cfg.eIoVoltage = eBusVoltage;
		ui32Status = data->host->ops->set_bus_voltage(data->host->pHandle, eBusVoltage);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
#ifdef CONFIG_PM_DEVICE_RUNTIME
			pm_device_runtime_put(dev);
#endif
			return -ENOTSUP;
		}
	}

	/* Change SDIO Host Bus Width */
	if (eBusWidth != data->card.cfg.eBusWidth) {
		data->card.cfg.eBusWidth = eBusWidth;
		ui32Status = data->host->ops->set_bus_width(data->host->pHandle, eBusWidth);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
#ifdef CONFIG_PM_DEVICE_RUNTIME
			pm_device_runtime_put(dev);
#endif
			return -ENOTSUP;
		}
	}

	/* Change SDIO Host Clock Speed */
	ui32Status = data->host->ops->set_bus_clock(data->host->pHandle, data->card.cfg.ui32Clock);
	if (ui32Status != AM_HAL_STATUS_SUCCESS) {
#ifdef CONFIG_PM_DEVICE_RUNTIME
		pm_device_runtime_put(dev);
#endif
		return -ENOTSUP;
	}

	if (ios->timing == SDHC_TIMING_DDR52) {
		LOG_DBG("MMC Card DDR50 Mode");
		/* DDR50 mode must be 4bit or 8bit width according to EMMC Spec */
		if (eBusWidth == AM_HAL_HOST_BUS_WIDTH_1) {
#ifdef CONFIG_PM_DEVICE_RUNTIME
			pm_device_runtime_put(dev);
#endif
			return -ENOTSUP;
		}
		eUHSMode = AM_HAL_HOST_UHS_DDR50;
	}

	/* Change SDIO Host UHS mode */
	if (eUHSMode != data->card.cfg.eUHSMode) {
		data->card.cfg.eUHSMode = eUHSMode;
		ui32Status = data->host->ops->set_uhs_mode(data->host->pHandle, eUHSMode);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
#ifdef CONFIG_PM_DEVICE_RUNTIME
			pm_device_runtime_put(dev);
#endif
			return -ENOTSUP;
		}
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return ret;
}

/*
 * Ambiq SDIO Host Initialization Function
 */
static int ambiq_sdio_init(const struct device *dev)
{
	const struct ambiq_sdio_config *config = dev->config;
	struct ambiq_sdio_data *data = dev->data;
	int ret;

	LOG_DBG("Ambiq SDIO Initialize Host #%d", config->inst);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	/* Get the uderlying SDHC host instance */
	data->host = am_hal_get_card_host(AM_HAL_SDHC_CARD_HOST + config->inst, true);

	if (data->host == NULL) {
		LOG_ERR("No such card host and stop");
		return -ENODEV;
	}

	config->irq_config_func(dev);

	/* Fill the Card default setting with the host's default value  */
	data->card.pHost = data->host;
	data->card.cfg.eBusWidth = data->host->eBusWidth;
	data->card.cfg.eIoVoltage = data->host->eBusVoltage;
	data->card.cfg.ui32Clock = data->host->ui32MinClock;
	data->card.cfg.eUHSMode = data->host->eUHSMode;

	/* Set AM_HAL_CARD_PWR_CTRL_SDHC_OFF as default SDIO power control policy  */
	data->card.eCardPwrCtrlPolicy = AM_HAL_CARD_PWR_CTRL_SDHC_OFF;
	data->card.eState = AM_HAL_CARD_STATE_PWRON;
	data->card.pCardPwrCtrlFunc = NULL;

#ifdef CONFIG_AMBIQ_SDIO_ASYNC
	/* Register callback function for Async data transfer  */
	if (config->inst == 0) {
		data->async_sem = &sdio_async_sem_0;
		am_hal_card_register_evt_callback(&data->card, ambiq_sdio_event_cb_0);
	} else if (config->inst == 1) {
		data->async_sem = &sdio_async_sem_1;
		am_hal_card_register_evt_callback(&data->card, ambiq_sdio_event_cb_1);
	} else {
		return -ENODEV;
	}
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */

	k_mutex_init(&data->access_mutex);

	return 0;
}

/*
 * Set TX & RX delay for Ambiq SDIO Timing Tuning
 */
static int ambiq_sdio_execute_tuning(const struct device *dev)
{
	const struct ambiq_sdio_config *config = dev->config;
	struct ambiq_sdio_data *data = dev->data;
	uint8_t ui8TxRxDelays[2] = {0};
	int ret = 0;

	/* SDIO TX delay setting range is 0 ~ 15 */
	if (config->tx_delay < 16) {
		ui8TxRxDelays[0] = config->tx_delay;
	} else {
		return -EINVAL;
	}

	/* SDIO RX delay setting range is 0 ~ 31 */
	if (config->rx_delay < 32) {
		ui8TxRxDelays[1] = config->rx_delay;
	} else {
		return -EINVAL;
	}

	/* Timing delay is disabled if both TX and RX delay are set into 0 */
	if (ui8TxRxDelays[0] != 0 || ui8TxRxDelays[1] != 0) {
#if defined(CONFIG_PM_DEVICE_RUNTIME)
		ret = pm_device_runtime_get(dev);

		if (ret < 0) {
			LOG_ERR("pm_device_runtime_get failed: %d", ret);
		}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

		am_hal_card_host_set_txrx_delay(data->host, ui8TxRxDelays);

#if defined(CONFIG_PM_DEVICE_RUNTIME)
		/* Use async put to avoid useless device suspension/resumption
		 * when doing consecutive transmission.
		 */
		ret = pm_device_runtime_put_async(dev, K_MSEC(2));

		if (ret < 0) {
			LOG_ERR("pm_device_runtime_put failed: %d", ret);
		}
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	}

	return ret;
}

/*
 * Check for Card Presence
 */
static int ambiq_sdio_get_card_present(const struct device *dev)
{
	struct ambiq_sdio_data *data = dev->data;
	int status = 0;
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	int ret = 0;

	ret = pm_device_runtime_get(dev);

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	LOG_DBG("Get card present status");
	status = data->host->ops->get_cd(data->host->pHandle);
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	return status;
}

/*
 * Check for Card Busy Status
 */
static int ambiq_sdio_card_busy(const struct device *dev)
{
	struct ambiq_sdio_data *data = dev->data;
	uint32_t ui32Status;
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	int ret = 0;

	ret = pm_device_runtime_get(dev);

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	ui32Status = data->host->ops->card_busy(data->host->pHandle, DEFAULT_GET_STATUS_TIMEOUT_MS);
	LOG_DBG("Check card busy status");
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return (ui32Status != AM_HAL_STATUS_SUCCESS) ? 1 : 0;
}

/*
 * Ambiq SDIO Command and Data Request Function
 */
static int ambiq_sdio_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	struct ambiq_sdio_data *dev_data = dev->data;
	uint32_t ui32Status = 0;
	int ret = 0;

	am_hal_card_cmd_t sdio_cmd = {0};
	am_hal_card_cmd_data_t cmd_data = {0};

	if (cmd) {
		sdio_cmd.ui8Idx = cmd->opcode;
		sdio_cmd.ui32Arg = cmd->arg;
		sdio_cmd.ui32RespType = cmd->response_type;
		sdio_cmd.bASync = false;
	} else {
		LOG_ERR("Invalid CMD");
		return -EINVAL;
	}

	if (data) {
		cmd_data.ui32BlkCnt = data->blocks;
		cmd_data.ui32BlkSize = data->block_size;
		cmd_data.pui8Buf = data->data;

		if (sdio_cmd.ui8Idx == MMC_CMD_READ_MULTIPLE_BLOCK ||
		    sdio_cmd.ui8Idx == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
			sdio_cmd.bAutoCMD23 = true;
		}

		if (sdio_cmd.ui8Idx == MMC_CMD_WRITE_SINGLE_BLOCK ||
		    sdio_cmd.ui8Idx == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
			cmd_data.dir = AM_HAL_DATA_DIR_WRITE;
		} else if ((sdio_cmd.ui8Idx == SDIO_CMD_IO_RW_EXTENDED) &&
			   (sdio_cmd.ui32Arg & BIT(SDIO_CMD_ARG_RW_SHIFT))) {
			cmd_data.dir = AM_HAL_DATA_DIR_WRITE;
		} else {
			cmd_data.dir = AM_HAL_DATA_DIR_READ;
		}
	}

	LOG_DBG("Send SDIO CMD%d", sdio_cmd.ui8Idx);
	LOG_DBG("CMD->Arg = 0x%x CMD->RespType = 0x%x", sdio_cmd.ui32Arg, sdio_cmd.ui32RespType);

	if (sdio_cmd.ui8Idx == 1) {
		LOG_DBG("Config CMD1 RespType");
		sdio_cmd.ui32RespType = MMC_RSP_R3;
	} else if (sdio_cmd.ui8Idx == 3) {
		LOG_DBG("Config CMD3 RespType");
		sdio_cmd.ui32RespType = MMC_RSP_R6;
	} else if ((sdio_cmd.ui8Idx == 52) || (sdio_cmd.ui8Idx == 53)) {
		LOG_DBG("Config CMD%d RespType", sdio_cmd.ui8Idx);
		sdio_cmd.ui32RespType = MMC_RSP_R5;
	} else if (sdio_cmd.ui8Idx == 6 || sdio_cmd.ui8Idx == 38) {
		LOG_DBG("Set CheckBusyCmd");
		sdio_cmd.bCheckBusyCmd = true;
		sdio_cmd.ui32RespType = MMC_RSP_R1b;
	} else if (sdio_cmd.ui8Idx == 17 || sdio_cmd.ui8Idx == 18 || sdio_cmd.ui8Idx == 24 ||
		   sdio_cmd.ui8Idx == 25) {
		sdio_cmd.ui32RespType = MMC_RSP_R1;
	}

#ifdef CONFIG_AMBIQ_SDIO_ASYNC
	if (data) {
		sdio_cmd.bASync = true;
		dev_data->host->AsyncCmd = sdio_cmd;
		dev_data->host->AsyncCmdData = cmd_data;
	}
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */

	ret = k_mutex_lock(&dev_data->access_mutex, K_MSEC(cmd->timeout_ms));
	if (ret) {
		LOG_ERR("Could not access card");
		return -EBUSY;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	ret = pm_device_runtime_get(dev);

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	if (data) {
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
		/* Clean Dcache before DMA write */
		if (cmd_data.dir == AM_HAL_DATA_DIR_WRITE &&
		    (uint32_t)(data->data) >= CACHABLE_START_ADDR) {
			sys_cache_data_flush_range(data->data, data->blocks * data->block_size);
		}
#endif

#ifdef CONFIG_AMBIQ_SDIO_ASYNC
		k_sem_reset(dev_data->async_sem);
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */
		ui32Status = dev_data->host->ops->execute_cmd(dev_data->host->pHandle, &sdio_cmd,
							      &cmd_data);
#ifdef CONFIG_AMBIQ_SDIO_ASYNC
		if ((ui32Status & 0xFFFF) == AM_HAL_STATUS_SUCCESS) {
			if (k_sem_take(dev_data->async_sem, K_MSEC(data->timeout_ms))) {
				return -ETIMEDOUT;
			}
		}
#endif /* CONFIG_AMBIQ_SDIO_ASYNC */

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
		/* Invalidate Dcache after DMA read */
		if (cmd_data.dir == AM_HAL_DATA_DIR_READ &&
		    (uint32_t)(data->data) >= CACHABLE_START_ADDR) {
			sys_cache_data_invd_range(data->data, data->blocks * data->block_size);
		}
#endif

	} else {
		ui32Status =
			dev_data->host->ops->execute_cmd(dev_data->host->pHandle, &sdio_cmd, NULL);
	}
	if ((ui32Status & 0xFFFF) != AM_HAL_STATUS_SUCCESS) {
		if ((ui32Status & 0xFFFF) == AM_HAL_STATUS_TIMEOUT) {
			LOG_DBG("CMD%d Timeout!", sdio_cmd.ui8Idx);
			ret = -ETIMEDOUT;
		} else {
			LOG_DBG("Failed to send CMD%d, ui32Status = 0x%x", sdio_cmd.ui8Idx,
				ui32Status);
			ret = -EIO;
		}
	}

	k_mutex_unlock(&dev_data->access_mutex);

	memcpy(cmd->response, sdio_cmd.ui32Resp, sizeof(cmd->response));

	LOG_DBG("Resp0 = 0x%x, Resp1 = 0x%x, Resp2 = 0x%x, Resp3 = 0x%x", cmd->response[0],
		cmd->response[1], cmd->response[2], cmd->response[3]);

	if (data) {
		data->bytes_xfered = (ui32Status >> 16) & 0xFFFF;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));

	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return ret;
}

/*
 * Enable Card Interrupts
 */
static int ambiq_sdio_card_interrupt_enable(const struct device *dev, sdhc_interrupt_cb_t callback,
					    int sources, void *user_data)
{
	struct ambiq_sdio_data *data = dev->data;
	uint32_t ui32Status = 0;

	data->sdio_cb = callback;
	data->sdio_cb_user_data = user_data;

	if (sources & SDHC_INT_SDIO) {
		/* Enable SDIO Card Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_enable(data->host->pHandle,
							    SDIO_INTSIG_CARDINTEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_enable(
			data->host->pHandle, SDIO_INTENABLE_CARDINTERRUPTSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	if (sources & SDHC_INT_INSERTED) {
		/* Enable Card Insert Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_enable(data->host->pHandle,
							    SDIO_INTSIG_CARDINSERTEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_enable(
			data->host->pHandle, SDIO_INTENABLE_CARDINSERTIONSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	if (sources & SDHC_INT_REMOVED) {
		/* Enable Card Removal Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_enable(data->host->pHandle,
							    SDIO_INTSIG_CARDREMOVALEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_enable(
			data->host->pHandle, SDIO_INTENABLE_CARDREMOVALSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

/*
 * Disable Card Interrupts
 */
static int ambiq_sdio_card_interrupt_disable(const struct device *dev, int sources)
{
	struct ambiq_sdio_data *data = dev->data;
	uint32_t ui32Status = 0;

	data->sdio_cb = NULL;
	data->sdio_cb_user_data = NULL;

	if (sources & SDHC_INT_SDIO) {
		/* Disable SDIO Card Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_disable(data->host->pHandle,
							     SDIO_INTSIG_CARDINTEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_disable(
			data->host->pHandle, SDIO_INTENABLE_CARDINTERRUPTSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	if (sources & SDHC_INT_INSERTED) {
		/* Disable Card Insert Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_disable(data->host->pHandle,
							     SDIO_INTSIG_CARDINSERTEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_disable(
			data->host->pHandle, SDIO_INTENABLE_CARDINSERTIONSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	if (sources & SDHC_INT_REMOVED) {
		/* Disable Card Removal Interrupt */
		ui32Status = am_hal_sdhc_intr_signal_disable(data->host->pHandle,
							     SDIO_INTSIG_CARDREMOVALEN_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
		ui32Status = am_hal_sdhc_intr_status_disable(
			data->host->pHandle, SDIO_INTENABLE_CARDREMOVALSTATUSENABLE_Msk);
		if (ui32Status != AM_HAL_STATUS_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static const struct sdhc_driver_api ambiq_sdio_api = {
	.reset = ambiq_sdio_reset,
	.request = ambiq_sdio_request,
	.set_io = ambiq_sdio_set_io,
	.get_card_present = ambiq_sdio_get_card_present,
	.execute_tuning = ambiq_sdio_execute_tuning,
	.card_busy = ambiq_sdio_card_busy,
	.get_host_props = ambiq_sdio_get_host_props,
	.enable_interrupt = ambiq_sdio_card_interrupt_enable,
	.disable_interrupt = ambiq_sdio_card_interrupt_disable,
};

#ifdef CONFIG_PM_DEVICE
/*
 * Ambiq SDIO peripheral power management function
 */
static int ambiq_sdio_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct ambiq_sdio_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = am_hal_card_pwrctrl_wakeup(&data->card);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = am_hal_card_pwrctrl_sleep(&data->card);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret != AM_HAL_STATUS_SUCCESS) {
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define AMBIQ_SDIO_INIT(n)                                                                         \
	static void sdio_##n##_irq_config_func(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ambiq_sdio_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct ambiq_sdio_config ambiq_sdio_config_##n = {                            \
		.pSDHC = (SDIO_Type *)DT_INST_REG_ADDR(n),                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.irq_config_func = sdio_##n##_irq_config_func,                                     \
		.inst = n,                                                                         \
		.tx_delay = DT_INST_PROP(n, txdelay),                                              \
		.rx_delay = DT_INST_PROP(n, rxdelay),                                              \
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),                                     \
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),                                     \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                                 \
	};                                                                                         \
                                                                                                   \
	static struct ambiq_sdio_data ambiq_sdio_data_##n;                                         \
	PM_DEVICE_DT_INST_DEFINE(n, ambiq_sdio_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(n, &ambiq_sdio_init, PM_DEVICE_DT_INST_GET(n), &ambiq_sdio_data_##n, \
			      &ambiq_sdio_config_##n, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,      \
			      &ambiq_sdio_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_SDIO_INIT)
