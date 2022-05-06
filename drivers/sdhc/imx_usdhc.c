/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_imx_usdhc

#include <zephyr/zephyr.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#define PINCTRL_STATE_SLOW PINCTRL_STATE_PRIV_START
#define PINCTRL_STATE_MED (PINCTRL_STATE_PRIV_START + 1U)
#define PINCTRL_STATE_FAST (PINCTRL_STATE_PRIV_START + 2U)
#define PINCTRL_STATE_NOPULL (PINCTRL_STATE_PRIV_START + 3U)
#endif

LOG_MODULE_REGISTER(usdhc, CONFIG_SDHC_LOG_LEVEL);

#include <fsl_usdhc.h>
#include <fsl_cache.h>

enum transfer_callback_status {
	TRANSFER_CMD_COMPLETE = BIT(0),
	TRANSFER_CMD_FAILED = BIT(1),
	TRANSFER_DATA_COMPLETE = BIT(2),
	TRANSFER_DATA_FAILED = BIT(3),
};

#define TRANSFER_CMD_FLAGS (TRANSFER_CMD_COMPLETE | TRANSFER_CMD_FAILED)
#define TRANSFER_DATA_FLAGS (TRANSFER_DATA_COMPLETE | TRANSFER_DATA_FAILED)

/* USDHC tuning constants */
#define IMX_USDHC_STANDARD_TUNING_START (10U)
#define IMX_USDHC_TUNING_STEP (2U)
#define IMX_USDHC_STANDARD_TUNING_COUNTER (60U)
/* Default transfer timeout in ms for tuning */
#define IMX_USDHC_DEFAULT_TIMEOUT (5000U)

struct usdhc_host_transfer {
	usdhc_transfer_t *transfer;
	k_timeout_t command_timeout;
	k_timeout_t data_timeout;
};

struct usdhc_config {
	USDHC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t nusdhc;
	const struct gpio_dt_spec pwr_gpio;
	const struct gpio_dt_spec detect_gpio;
	bool detect_dat3;
	bool no_180_vol;
	uint32_t data_timeout;
	uint32_t read_watermark;
	uint32_t write_watermark;
	uint32_t max_current_330;
	uint32_t max_current_300;
	uint32_t max_current_180;
	uint32_t power_delay_ms;
	uint32_t min_bus_freq;
	uint32_t max_bus_freq;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
	void (*irq_config_func)(const struct device *dev);
};

struct usdhc_data {
	struct sdhc_host_props props;
	bool card_present;
	struct k_sem transfer_sem;
	volatile uint32_t transfer_status;
	usdhc_handle_t transfer_handle;
	struct sdhc_io host_io;
	struct k_mutex access_mutex;
	uint8_t usdhc_rx_dummy[64] __aligned(32);
#ifdef CONFIG_IMX_USDHC_DMA_SUPPORT
	uint32_t *usdhc_dma_descriptor; /* ADMA descriptor table (noncachable) */
	uint32_t dma_descriptor_len; /* DMA descriptor table length in words */
#endif
};

static void transfer_complete_cb(USDHC_Type *usdhc, usdhc_handle_t *handle,
	status_t status, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct usdhc_data *data = dev->data;

	if (status == kStatus_USDHC_TransferDataFailed) {
		data->transfer_status |= TRANSFER_DATA_FAILED;
	} else if (status == kStatus_USDHC_TransferDataComplete) {
		data->transfer_status |= TRANSFER_DATA_COMPLETE;
	} else if (status == kStatus_USDHC_SendCommandFailed) {
		data->transfer_status |= TRANSFER_CMD_FAILED;
	} else if (status == kStatus_USDHC_SendCommandSuccess) {
		data->transfer_status |= TRANSFER_CMD_COMPLETE;
	}
	k_sem_give(&data->transfer_sem);
}

static int imx_usdhc_dat3_pull(const struct usdhc_config *cfg, bool pullup)
{
	int ret = 0U;

#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_NOPULL);
	if (ret) {
		LOG_ERR("No DAT3 floating state defined, but dat3 detect selected");
		return ret;
	}
#else
	/* Call board specific function to pull down DAT3 */
	imxrt_usdhc_dat3_pull(pullup);
#endif
#ifdef CONFIG_IMX_USDHC_DAT3_PWR_TOGGLE
	if (!pullup) {
		/* Power off the card to clear DAT3 legacy status */
		if (cfg->pwr_gpio.port) {
			ret = gpio_pin_set_dt(&cfg->pwr_gpio, 0);
			if (ret) {
				return ret;
			}
			/* Delay for card power off to complete */
			k_busy_wait(1000);
			ret = gpio_pin_set_dt(&cfg->pwr_gpio, 1);
			/* Delay for power on */
			k_busy_wait(1000);
			if (ret) {
				return ret;
			}
		}
	}
#endif
	return ret;
}

/*
 * Reset SDHC after command error
 */
static void imx_usdhc_error_recovery(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	uint32_t status = USDHC_GetPresentStatusFlags(cfg->base);

	if (status & kUSDHC_CommandInhibitFlag) {
		/* Reset command line */
		USDHC_Reset(cfg->base, kUSDHC_ResetCommand, 100U);
	}
	if (((status & (uint32_t)kUSDHC_DataInhibitFlag) != 0U) ||
		(USDHC_GetAdmaErrorStatusFlags(cfg->base) != 0U)) {
		/* Reset data line */
		USDHC_Reset(cfg->base, kUSDHC_DataInhibitFlag, 100U);
	}
}

/*
 * Initialize SDHC host properties for use in get_host_props api call
 */
static void imx_usdhc_init_host_props(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *data = dev->data;
	usdhc_capability_t caps;
	struct sdhc_host_props *props = &data->props;

	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_max = cfg->max_bus_freq;
	props->f_min = cfg->min_bus_freq;
	props->max_current_330 = cfg->max_current_330;
	props->max_current_180 = cfg->max_current_180;
	props->power_delay = cfg->power_delay_ms;
	/* Read host capabilities */
	USDHC_GetCapability(cfg->base, &caps);
	if (cfg->no_180_vol) {
		props->host_caps.vol_180_support = false;
	} else {
		props->host_caps.vol_180_support = (bool)(caps.flags & kUSDHC_SupportV180Flag);
	}
	props->host_caps.vol_300_support = (bool)(caps.flags & kUSDHC_SupportV300Flag);
	props->host_caps.vol_330_support = (bool)(caps.flags & kUSDHC_SupportV330Flag);
	props->host_caps.suspend_res_support = (bool)(caps.flags & kUSDHC_SupportSuspendResumeFlag);
	props->host_caps.sdma_support = (bool)(caps.flags & kUSDHC_SupportDmaFlag);
	props->host_caps.high_spd_support = (bool)(caps.flags & kUSDHC_SupportHighSpeedFlag);
	props->host_caps.adma_2_support = (bool)(caps.flags & kUSDHC_SupportAdmaFlag);
	props->host_caps.max_blk_len = (bool)(caps.maxBlockLength);
	props->host_caps.ddr50_support = (bool)(caps.flags & kUSDHC_SupportDDR50Flag);
	props->host_caps.sdr104_support = (bool)(caps.flags & kUSDHC_SupportSDR104Flag);
	props->host_caps.sdr50_support = (bool)(caps.flags & kUSDHC_SupportSDR50Flag);
}

/*
 * Reset USDHC controller
 */
static int imx_usdhc_reset(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	/* Switch to default I/O voltage of 3.3V */
	UDSHC_SelectVoltage(cfg->base, false);
	USDHC_EnableDDRMode(cfg->base, false, 0U);
#if defined(FSL_FEATURE_USDHC_HAS_SDR50_MODE) && (FSL_FEATURE_USDHC_HAS_SDR50_MODE)
	USDHC_EnableStandardTuning(cfg->base, 0, 0, false);
	USDHC_EnableAutoTuning(cfg->base, false);
#endif

#if FSL_FEATURE_USDHC_HAS_HS400_MODE
	/* Disable HS400 mode */
	USDHC_EnableHS400Mode(cfg->base, false);
	/* Disable DLL */
	USDHC_EnableStrobeDLL(cfg->base, false);
#endif

	/* Reset data/command/tuning circuit */
	return USDHC_Reset(cfg->base, kUSDHC_ResetAll, 100U) == true ? 0 : -ETIMEDOUT;
}

/* Wait for USDHC to gate clock when it is disabled */
static inline void imx_usdhc_wait_clock_gate(USDHC_Type *base)
{
	uint32_t timeout = 1000;

	while (timeout--) {
		if (base->PRES_STATE & USDHC_PRES_STATE_SDOFF_MASK) {
			break;
		}
	}
	if (timeout == 0) {
		LOG_WRN("SD clock did not gate in time");
	}
}

/*
 * Set SDHC io properties
 */
static int imx_usdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *data = dev->data;
	uint32_t src_clk_hz, bus_clk;
	struct sdhc_io *host_io = &data->host_io;

	LOG_DBG("SDHC I/O: bus width %d, clock %dHz, card power %s, voltage %s",
		ios->bus_width,
		ios->clock,
		ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V"
		);

	if (clock_control_get_rate(cfg->clock_dev,
				cfg->clock_subsys,
				&src_clk_hz)) {
		return -EINVAL;
	}

	if (ios->clock && (ios->clock > data->props.f_max || ios->clock < data->props.f_min)) {
		return -EINVAL;
	}

	/* Set host clock */
	if (host_io->clock != ios->clock) {
		if (ios->clock != 0) {
			/* Enable the clock output */
			bus_clk = USDHC_SetSdClock(cfg->base, src_clk_hz, ios->clock);
			if (bus_clk == 0) {
				return -ENOTSUP;
			}
		}
		host_io->clock = ios->clock;
	}


	/* Set bus width */
	if (host_io->bus_width != ios->bus_width) {
		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			USDHC_SetDataBusWidth(cfg->base, kUSDHC_DataBusWidth1Bit);
			break;
		case SDHC_BUS_WIDTH4BIT:
			USDHC_SetDataBusWidth(cfg->base, kUSDHC_DataBusWidth4Bit);
			break;
		case SDHC_BUS_WIDTH8BIT:
			USDHC_SetDataBusWidth(cfg->base, kUSDHC_DataBusWidth8Bit);
			break;
		default:
			return -ENOTSUP;
		}
		host_io->bus_width = ios->bus_width;
	}

	/* Set host signal voltage */
	if (ios->signal_voltage != host_io->signal_voltage) {
		switch (ios->signal_voltage) {
		case SD_VOL_3_3_V:
		case SD_VOL_3_0_V:
			UDSHC_SelectVoltage(cfg->base, false);
			break;
		case SD_VOL_1_8_V:
			/**
			 * USDHC peripheral deviates from SD spec here.
			 * The host controller specification claims
			 * the "SD clock enable" bit can be used to gate the SD
			 * clock by clearing it. The USDHC controller does not
			 * provide this bit, only a way to force the SD clock
			 * on. We will instead delay 10 ms to allow the clock
			 * to be gated for enough time, then force it on for
			 * 10 ms, then allow it to be gated again.
			 */
			/* Switch to 1.8V */
			UDSHC_SelectVoltage(cfg->base, true);
			/* Wait 10 ms- clock will be gated during this period */
			k_msleep(10);
			/* Force the clock on */
			USDHC_ForceClockOn(cfg->base, true);
			/* Keep the clock on for a moment, so SD will recognize it */
			k_msleep(10);
			/* Stop forcing clock on */
			USDHC_ForceClockOn(cfg->base, false);
			break;
		default:
			return -ENOTSUP;
		}
		/* Save new host voltage */
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set card power */
	if ((host_io->power_mode != ios->power_mode) && (cfg->pwr_gpio.port)) {
		if (host_io->power_mode == SDHC_POWER_ON) {
			/* Send 74 clock cycles if SD card is just powering on */
			USDHC_SetCardActive(cfg->base, 0xFFFF);
		}
		if (cfg->pwr_gpio.port) {
			if (ios->power_mode == SDHC_POWER_OFF) {
				gpio_pin_set_dt(&cfg->pwr_gpio, 0);
			} else if (ios->power_mode == SDHC_POWER_ON) {
				gpio_pin_set_dt(&cfg->pwr_gpio, 1);
			}
		}
		host_io->power_mode = ios->power_mode;
	}

	/* Set I/O timing */
	if (host_io->timing != ios->timing) {
		switch (ios->timing) {
		case SDHC_TIMING_LEGACY:
		case SDHC_TIMING_HS:
			break;
		case SDHC_TIMING_SDR12:
		case SDHC_TIMING_SDR25:
#ifdef CONFIG_PINCTRL
			pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_SLOW);
#else
			imxrt_usdhc_pinmux(cfg->nusdhc, false, 0, 7);
#endif
			break;
		case SDHC_TIMING_SDR50:
#ifdef CONFIG_PINCTRL
			pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_MED);
#else
			imxrt_usdhc_pinmux(cfg->nusdhc, false, 2, 7);
#endif
			break;
		case SDHC_TIMING_SDR104:
		case SDHC_TIMING_DDR50:
		case SDHC_TIMING_DDR52:
		case SDHC_TIMING_HS200:
		case SDHC_TIMING_HS400:
#ifdef CONFIG_PINCTRL
			pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_FAST);
#else
			imxrt_usdhc_pinmux(cfg->nusdhc, false, 3, 7);
#endif
			break;
		default:
			return -ENOTSUP;
		}
		host_io->timing = ios->timing;
	}

	return 0;
}

/*
 * Internal transfer function, used by tuning and request apis
 */
static int imx_usdhc_transfer(const struct device *dev,
	struct usdhc_host_transfer *request)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *dev_data = dev->data;
	status_t error;
#ifdef CONFIG_IMX_USDHC_DMA_SUPPORT
	usdhc_adma_config_t dma_config = {0};

	/* Configure DMA */
	dma_config.admaTable = dev_data->usdhc_dma_descriptor;
	dma_config.admaTableWords = dev_data->dma_descriptor_len;
#if !(defined(FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN) && FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN)
	dma_config.burstLen = kUSDHC_EnBurstLenForINCR;
#endif
	dma_config.dmaMode = kUSDHC_DmaModeAdma2;
#endif /* CONFIG_IMX_USDHC_DMA_SUPPORT */

	/* Reset transfer status */
	dev_data->transfer_status = 0U;
	/* Reset semaphore */
	k_sem_reset(&dev_data->transfer_sem);
#ifdef CONFIG_IMX_USDHC_DMA_SUPPORT
	error = USDHC_TransferNonBlocking(cfg->base, &dev_data->transfer_handle,
			&dma_config, request->transfer);
#else
	error = USDHC_TransferNonBlocking(cfg->base, &dev_data->transfer_handle,
			NULL, request->transfer);
#endif
	if (error == kStatus_USDHC_ReTuningRequest) {
		return -EAGAIN;
	} else if (error != kStatus_Success) {
		return -EIO;
	}
	/* Wait for event to occur */
	while ((dev_data->transfer_status & (TRANSFER_CMD_FLAGS | TRANSFER_DATA_FLAGS)) == 0) {
		if (k_sem_take(&dev_data->transfer_sem, request->command_timeout)) {
			return -ETIMEDOUT;
		}
	}
	if (dev_data->transfer_status & TRANSFER_CMD_FAILED) {
		return -EIO;
	}
	/* If data was sent, wait for that to complete */
	if (request->transfer->data) {
		while ((dev_data->transfer_status & TRANSFER_DATA_FLAGS) == 0) {
			if (k_sem_take(&dev_data->transfer_sem, request->data_timeout)) {
				return -ETIMEDOUT;
			}
		}
		if (dev_data->transfer_status & TRANSFER_DATA_FAILED) {
			return -EIO;
		}
	}
	return 0;
}

/* Stops transmission after failed command with CMD12 */
static void imx_usdhc_stop_transmission(const struct device *dev)
{
	usdhc_command_t stop_cmd = {0};
	struct usdhc_host_transfer request;
	usdhc_transfer_t transfer;

	/* Send CMD12 to stop transmission */
	stop_cmd.index = SD_STOP_TRANSMISSION;
	stop_cmd.argument = 0U;
	stop_cmd.type = kCARD_CommandTypeAbort;
	stop_cmd.responseType = SD_RSP_TYPE_R1b;
	transfer.command = &stop_cmd;
	transfer.data = NULL;

	request.transfer = &transfer;
	request.command_timeout = K_MSEC(IMX_USDHC_DEFAULT_TIMEOUT);
	request.data_timeout = K_MSEC(IMX_USDHC_DEFAULT_TIMEOUT);

	imx_usdhc_transfer(dev, &request);
}

/*
 * Return 0 if card is not busy, 1 if it is
 */
static int imx_usdhc_card_busy(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;

	return (USDHC_GetPresentStatusFlags(cfg->base)
		& (kUSDHC_Data0LineLevelFlag |
		kUSDHC_Data1LineLevelFlag |
		kUSDHC_Data2LineLevelFlag |
		kUSDHC_Data3LineLevelFlag))
		? 0 : 1;
}

/*
 * Execute card tuning
 */
static int imx_usdhc_execute_tuning(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *dev_data = dev->data;
	usdhc_command_t cmd = {0};
	usdhc_data_t data = {0};
	struct usdhc_host_transfer request;
	usdhc_transfer_t transfer;
	int ret;
	bool retry_tuning = true;

	cmd.index = SD_SEND_TUNING_BLOCK;
	cmd.argument = 0;
	cmd.responseType = SD_RSP_TYPE_R1;

	data.blockSize = sizeof(dev_data->usdhc_rx_dummy);
	data.blockCount = 1;
	data.rxData = (uint32_t *)dev_data->usdhc_rx_dummy;
	data.dataType = kUSDHC_TransferDataTuning;

	transfer.command = &cmd;
	transfer.data = &data;

	/* Reset tuning circuit */
	USDHC_Reset(cfg->base, kUSDHC_ResetTuning, 100U);
	/* Disable standard tuning */
	USDHC_EnableStandardTuning(cfg->base, IMX_USDHC_STANDARD_TUNING_START,
		IMX_USDHC_TUNING_STEP, false);
	/*
	 * Tuning fail found on some SOCs is caused by the different of delay
	 * cell, so we need to increase the tuning counter to cover the
	 * adjustable tuning window
	 */
	USDHC_SetStandardTuningCounter(cfg->base, IMX_USDHC_STANDARD_TUNING_COUNTER);
	/* Reenable standard tuning */
	USDHC_EnableStandardTuning(cfg->base, IMX_USDHC_STANDARD_TUNING_START,
		IMX_USDHC_TUNING_STEP, true);

	request.command_timeout = K_MSEC(IMX_USDHC_DEFAULT_TIMEOUT);
	request.data_timeout = K_MSEC(IMX_USDHC_DEFAULT_TIMEOUT);
	request.transfer = &transfer;

	while (true) {
		ret = imx_usdhc_transfer(dev, &request);
		if (ret) {
			return ret;
		}
		/* Delay 1ms */
		k_busy_wait(1000);

		/* Wait for execute tuning bit to clear */
		if (USDHC_GetExecuteStdTuningStatus(cfg->base) != 0) {
			continue;
		}
		/* If tuning had error, retry tuning */
		if ((USDHC_CheckTuningError(cfg->base) != 0U) && retry_tuning) {
			retry_tuning = false;
			/* Enable standard tuning */
			USDHC_EnableStandardTuning(cfg->base,
				IMX_USDHC_STANDARD_TUNING_START,
				IMX_USDHC_TUNING_STEP, true);
			USDHC_SetTuningDelay(cfg->base,
				IMX_USDHC_STANDARD_TUNING_START, 0U, 0U);
		} else {
			break;
		}
	}

	/* Check tuning result */
	if (USDHC_CheckStdTuningResult(cfg->base) == 0) {
		return -EIO;
	}

	/* Enable auto tuning */
	USDHC_EnableAutoTuning(cfg->base, true);
	return 0;
}

/*
 * Send CMD or CMD/DATA via SDHC
 */
static int imx_usdhc_request(const struct device *dev, struct sdhc_command *cmd,
	struct sdhc_data *data)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *dev_data = dev->data;
	usdhc_command_t host_cmd = {0};
	usdhc_data_t host_data = {0};
	struct usdhc_host_transfer request;
	usdhc_transfer_t transfer;
	int busy_timeout = IMX_USDHC_DEFAULT_TIMEOUT;
	int ret = 0;
	int retries = (int)cmd->retries;

	host_cmd.index = cmd->opcode;
	host_cmd.argument = cmd->arg;
	/* Mask out part of response type field used for SPI commands */
	host_cmd.responseType = (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);
	transfer.command = &host_cmd;
	if (cmd->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		request.command_timeout = K_FOREVER;
	} else {
		request.command_timeout = K_MSEC(cmd->timeout_ms);
	}

	if (data) {
		host_data.blockSize = data->block_size;
		host_data.blockCount = data->blocks;
		/*
		 * Determine type of command. Note that driver is expected to
		 * handle CMD12 and CMD23 for reading and writing blocks
		 */
		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
			host_data.enableAutoCommand12 = true;
			host_data.txData = data->data;
			break;
		case SD_WRITE_MULTIPLE_BLOCK:
			if (dev_data->host_io.timing == SDHC_TIMING_SDR104) {
				/* Card uses UHS104, so it must support CMD23 */
				host_data.enableAutoCommand23 = true;
			} else {
				/* No CMD23 support */
				host_data.enableAutoCommand12 = true;
			}
			host_data.txData = data->data;
			break;
		case SD_READ_SINGLE_BLOCK:
			host_data.enableAutoCommand12 = true;
			host_data.rxData = data->data;
			break;
		case SD_READ_MULTIPLE_BLOCK:
			if (dev_data->host_io.timing == SDHC_TIMING_SDR104) {
				/* Card uses UHS104, so it must support CMD23 */
				host_data.enableAutoCommand23 = true;
			} else {
				/* No CMD23 support */
				host_data.enableAutoCommand12 = true;
			}
			host_data.rxData = data->data;
			break;
		case SD_APP_SEND_SCR:
		case SD_SWITCH:
		case SD_APP_SEND_NUM_WRITTEN_BLK:
			host_data.rxData = data->data;
			break;
		default:
			return -ENOTSUP;

		}
		transfer.data = &host_data;
		if (data->timeout_ms == SDHC_TIMEOUT_FOREVER) {
			request.data_timeout = K_FOREVER;
		} else {
			request.data_timeout = K_MSEC(data->timeout_ms);
		}
	} else {
		transfer.data = NULL;
		request.data_timeout = K_NO_WAIT;
	}
	request.transfer = &transfer;

	/* Ensure we have exclusive access to SD card before sending request */
	if (k_mutex_lock(&dev_data->access_mutex, request.command_timeout) != 0) {
		return -EBUSY;
	}
	while (retries >= 0) {
		ret = imx_usdhc_transfer(dev, &request);
		if (ret && data) {
			/*
			 * Disable and clear interrupts. If the data transmission
			 * completes later, we will encounter issues because
			 * the USDHC driver expects data to be present in the
			 * current transmission, but CMD12 does not contain data
			 */
			USDHC_DisableInterruptSignal(cfg->base, kUSDHC_CommandFlag |
				kUSDHC_DataFlag | kUSDHC_DataDMAFlag);
			USDHC_ClearInterruptStatusFlags(cfg->base, kUSDHC_CommandFlag |
				kUSDHC_DataFlag | kUSDHC_DataDMAFlag);
			/* Stop transmission with CMD12 in case of data error */
			imx_usdhc_stop_transmission(dev);
			/* Wait for card to go idle */
			while (busy_timeout > 0) {
				if (!imx_usdhc_card_busy(dev)) {
					break;
				}
				/* Wait 125us before polling again */
				k_busy_wait(125);
				busy_timeout -= 125;
			}
			if (busy_timeout <= 0) {
				LOG_DBG("Card did not idle after CMD12");
				k_mutex_unlock(&dev_data->access_mutex);
				return -ETIMEDOUT;
			}
		}
		if (ret == -EAGAIN) {
			/* Retry, card made a tuning request */
			if (dev_data->host_io.timing == SDHC_TIMING_SDR50 ||
				dev_data->host_io.timing == SDHC_TIMING_SDR104) {
				/* Retune card */
				LOG_DBG("Card made tuning request, retune");
				ret = imx_usdhc_execute_tuning(dev);
				if (ret) {
					LOG_DBG("Card failed to tune");
					k_mutex_unlock(&dev_data->access_mutex);
					return ret;
				}
			}
		}
		if (ret) {
			imx_usdhc_error_recovery(dev);
			retries--;
		} else {
			break;
		}
	}

	/* Release access on card */
	k_mutex_unlock(&dev_data->access_mutex);
	/* Record command response */
	memcpy(cmd->response, host_cmd.response, sizeof(cmd->response));
	if (data) {
		/* Record number of bytes xfered */
		data->bytes_xfered = dev_data->transfer_handle.transferredWords;
	}
	return ret;
}

/*
 * Get card presence
 */
static int imx_usdhc_get_card_present(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *data = dev->data;

	if (cfg->detect_dat3) {
		/*
		 * If card is already present, do not retry detection.
		 * Power line toggling would reset SD card
		 */
		if (!data->card_present) {
			/* Detect card presence with DAT3 line pull */
			imx_usdhc_dat3_pull(cfg, false);
			USDHC_CardDetectByData3(cfg->base, true);
			/* Delay to ensure host has time to detect card */
			k_busy_wait(1000);
			data->card_present = USDHC_DetectCardInsert(cfg->base);
			/* Clear card detection and pull */
			imx_usdhc_dat3_pull(cfg, true);
			USDHC_CardDetectByData3(cfg->base, false);
		}
	} else if (cfg->detect_gpio.port) {
		data->card_present = gpio_pin_get_dt(&cfg->detect_gpio) > 0;
	} else {
		LOG_WRN("No card presence method configured, assuming card is present");
		data->card_present = true;
	}
	return ((int)data->card_present);
}

/*
 * Get host properties
 */
static int imx_usdhc_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	struct usdhc_data *data = dev->data;

	memcpy(props, &data->props, sizeof(struct sdhc_host_props));
	return 0;
}

static int imx_usdhc_isr(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *data = dev->data;

	USDHC_TransferHandleIRQ(cfg->base, &data->transfer_handle);
	return 0;
}

/*
 * Perform early system init for SDHC
 */
static int imx_usdhc_init(const struct device *dev)
{
	const struct usdhc_config *cfg = dev->config;
	struct usdhc_data *data = dev->data;
	usdhc_config_t host_config = {0};
	int ret;
	const usdhc_transfer_callback_t callbacks = {
		.TransferComplete = transfer_complete_cb,
	};


#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif
	USDHC_TransferCreateHandle(cfg->base, &data->transfer_handle,
		&callbacks, (void *)dev);
	cfg->irq_config_func(dev);


	host_config.dataTimeout = cfg->data_timeout;
	host_config.endianMode = kUSDHC_EndianModeLittle;
	host_config.readWatermarkLevel = cfg->read_watermark;
	host_config.writeWatermarkLevel = cfg->write_watermark;
	USDHC_Init(cfg->base, &host_config);
	/* Read host controller properties */
	imx_usdhc_init_host_props(dev);
	/* Set power GPIO low, so card starts powered off */
	if (cfg->pwr_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->pwr_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	} else {
		LOG_WRN("No power control GPIO defined. Without power control,\n"
			"the SD card may fail to communicate with the host");
	}
	if (cfg->detect_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->detect_gpio, GPIO_INPUT);
		if (ret) {
			return ret;
		}
	}
	k_mutex_init(&data->access_mutex);
	memset(&data->host_io, 0, sizeof(data->host_io));
	return k_sem_init(&data->transfer_sem, 0, 1);
}

static const struct sdhc_driver_api usdhc_api = {
	.reset = imx_usdhc_reset,
	.request = imx_usdhc_request,
	.set_io = imx_usdhc_set_io,
	.get_card_present = imx_usdhc_get_card_present,
	.execute_tuning = imx_usdhc_execute_tuning,
	.card_busy = imx_usdhc_card_busy,
	.get_host_props = imx_usdhc_get_host_props,
};

#ifdef CONFIG_PINCTRL
#define IMX_USDHC_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define IMX_USDHC_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define IMX_USDHC_PINCTRL_DEFINE(n)
#define IMX_USDHC_PINCTRL_INIT(n)
#endif

#ifdef CONFIG_IMX_USDHC_DMA_SUPPORT
#define IMX_USDHC_DMA_BUFFER_DEFINE(n)						\
	static uint32_t	__aligned(32)						\
		usdhc_##n##_dma_descriptor[CONFIG_IMX_USDHC_DMA_BUFFER_SIZE / 4]\
		__attribute__((__section__(".nocache")));
#define IMX_USDHC_DMA_BUFFER_INIT(n)						\
	.usdhc_dma_descriptor = usdhc_##n##_dma_descriptor,			\
	.dma_descriptor_len = CONFIG_IMX_USDHC_DMA_BUFFER_SIZE / 4,
#else
#define IMX_USDHC_DMA_BUFFER_DEFINE(n)
#define IMX_USDHC_DMA_BUFFER_INIT(n)
#endif /* CONFIG_IMX_USDHC_DMA_SUPPORT */



#define IMX_USDHC_INIT(n)							\
	static void usdhc_##n##_irq_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			imx_usdhc_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	IMX_USDHC_PINCTRL_DEFINE(n)						\
										\
	static const struct usdhc_config usdhc_##n##_config = {			\
		.base = (USDHC_Type *) DT_INST_REG_ADDR(n),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.nusdhc = n,							\
		.pwr_gpio = GPIO_DT_SPEC_INST_GET_OR(n, pwr_gpios, {0}),	\
		.detect_gpio = GPIO_DT_SPEC_INST_GET_OR(n, cd_gpios, {0}),	\
		.data_timeout = DT_INST_PROP(n, data_timeout),			\
		.detect_dat3 = DT_INST_PROP(n, detect_dat3),			\
		.no_180_vol = DT_INST_PROP(n, no_1_8_v),			\
		.read_watermark = DT_INST_PROP(n, read_watermark),		\
		.write_watermark = DT_INST_PROP(n, write_watermark),		\
		.max_current_330 = DT_INST_PROP(n, max_current_330),		\
		.max_current_180 = DT_INST_PROP(n, max_current_180),		\
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),			\
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),			\
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),		\
		.irq_config_func = usdhc_##n##_irq_config_func,			\
		IMX_USDHC_PINCTRL_INIT(n)					\
	};									\
										\
										\
	IMX_USDHC_DMA_BUFFER_DEFINE(n)						\
										\
	static struct usdhc_data usdhc_##n##_data = {				\
		.card_present = false,						\
		IMX_USDHC_DMA_BUFFER_INIT(n)					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			&imx_usdhc_init,					\
			NULL,							\
			&usdhc_##n##_data,					\
			&usdhc_##n##_config,					\
			POST_KERNEL,						\
			CONFIG_SDHC_INIT_PRIORITY,				\
			&usdhc_api);

DT_INST_FOREACH_STATUS_OKAY(IMX_USDHC_INIT)
