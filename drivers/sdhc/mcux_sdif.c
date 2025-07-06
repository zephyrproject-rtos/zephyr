/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_sdif

#include <zephyr/drivers/sdhc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <fsl_sdif.h>

LOG_MODULE_REGISTER(sdif, CONFIG_SDHC_LOG_LEVEL);

enum mcux_sdif_callback_status {
	TRANSFER_CMD_COMPLETE = BIT(0),
	TRANSFER_CMD_FAILED = BIT(1),
	TRANSFER_DATA_COMPLETE = BIT(2),
	TRANSFER_DATA_FAILED = BIT(3),
};

#define TRANSFER_CMD_FLAGS (TRANSFER_CMD_COMPLETE | TRANSFER_CMD_FAILED)
#define TRANSFER_DATA_FLAGS (TRANSFER_DATA_COMPLETE | TRANSFER_DATA_FAILED)

#define MCUX_SDIF_RESET_TIMEOUT_VALUE (1000000U)

#define MCUX_SDIF_DEFAULT_TIMEOUT (5000U)

#define MCUX_SDIF_F_MAX MHZ(50)
#define MCUX_SDIF_F_MIN KHZ(400)

struct mcux_sdif_config {
	SDIF_Type *base;
	const struct pinctrl_dev_config *pincfg;
	uint32_t response_timeout;
	uint32_t cd_debounce_clocks;
	uint32_t data_timeout;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_sdif_data {
	volatile uint32_t transfer_status;
	sdif_handle_t transfer_handle;
	struct k_sem transfer_sem;
	struct k_mutex access_mutex;
#ifdef CONFIG_MCUX_SDIF_DMA_SUPPORT
	uint32_t *sdif_dma_descriptor;
#endif /* CONFIG_MCUX_SDIF_DMA_SUPPORT */
};


static void mcux_sdif_transfer_complete(SDIF_Type *base, void *handle,
	status_t status, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct mcux_sdif_data *data = dev->data;

	if (status == kStatus_SDIF_DataTransferFail) {
		data->transfer_status |= TRANSFER_DATA_FAILED;
	} else if (status == kStatus_SDIF_DataTransferSuccess) {
		data->transfer_status |= TRANSFER_DATA_COMPLETE;
	} else if (status == kStatus_SDIF_SendCmdFail) {
		data->transfer_status |= TRANSFER_CMD_FAILED;
	} else if (status == kStatus_SDIF_SendCmdSuccess) {
		data->transfer_status |= TRANSFER_CMD_COMPLETE;
	} else {
		__ASSERT(false, "Unknown status code from SD interrupt");
	}
	k_sem_give(&data->transfer_sem);
}


/* SDIF IRQ handler not exposed in SDK header, so declare it here */
extern void SDIO_DriverIRQHandler(void);

/*
 * MCUX SDIF interrupt service routine
 */
static int mcux_sdif_isr(const struct device *dev)
{
	SDIO_DriverIRQHandler();
	return 0;
}

static int mcux_sdif_reset(const struct device *dev)
{
	const struct mcux_sdif_config *config = dev->config;
	struct mcux_sdif_data *data = dev->data;

	k_mutex_lock(&data->access_mutex, K_FOREVER);
	/* Disable all interrupts */
	SDIF_DisableInterrupt(config->base, kSDIF_AllInterruptStatus);

	/* Release all bus lines */
	(void)SDIF_Reset(config->base, kSDIF_ResetAll, MCUX_SDIF_RESET_TIMEOUT_VALUE);

	/* clear all interrupt/DMA status */
	SDIF_ClearInterruptStatus(config->base, kSDIF_AllInterruptStatus);
	SDIF_ClearInternalDMAStatus(config->base, kSDIF_DMAAllStatus);
	k_mutex_unlock(&data->access_mutex);
	return 0;
}

static int mcux_sdif_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	memset(props, 0, sizeof(*props));
	props->f_max = MCUX_SDIF_F_MAX;
	props->f_min = MCUX_SDIF_F_MIN;
	props->power_delay = 500;
	props->host_caps.high_spd_support = true;
	props->host_caps.suspend_res_support = true;
	props->host_caps.vol_330_support = true;
	props->host_caps.bus_8_bit_support = true;
	props->max_current_330 = 1024;
	return 0;
}

static int mcux_sdif_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct mcux_sdif_config *config = dev->config;
	uint32_t src_clk_hz, bus_clk_hz;

	if (clock_control_get_rate(config->clock_dev,
				config->clock_subsys,
				&src_clk_hz)) {
		return -EINVAL;
	}

	/* If clock is set to zero, we should gate clock */
	if (ios->clock != 0 &&
			(ios->clock <= MCUX_SDIF_F_MAX) &&
			(ios->clock >= MCUX_SDIF_F_MIN)) {
		bus_clk_hz = SDIF_SetCardClock(config->base, src_clk_hz, ios->clock);
		if (bus_clk_hz == 0) {
			return -ENOTSUP;
		}
		LOG_DBG("SDIF clock set to %d", bus_clk_hz);
	} else if (ios->clock != 0) {
		/* Invalid clock setting */
		return -ENOTSUP;
	}

	if (ios->bus_mode != SDHC_BUSMODE_PUSHPULL) {
		return -ENOTSUP;
	}

	SDIF_EnableCardPower(config->base, ios->power_mode == SDHC_POWER_ON);

	switch (ios->bus_width) {
	case SDHC_BUS_WIDTH1BIT:
		SDIF_SetCardBusWidth(config->base, kSDIF_Bus1BitWidth);
		break;
	case SDHC_BUS_WIDTH4BIT:
		SDIF_SetCardBusWidth(config->base, kSDIF_Bus4BitWidth);
		break;
	case SDHC_BUS_WIDTH8BIT:
		SDIF_SetCardBusWidth(config->base, kSDIF_Bus8BitWidth);
		break;
	default:
		return -ENOTSUP;
	}

	if (ios->signal_voltage != SD_VOL_3_3_V) {
		return -ENOTSUP;
	}
	return 0;
}

/*
 * Early system init for SDHC
 */
static int mcux_sdif_init(const struct device *dev)
{
	const struct mcux_sdif_config *config = dev->config;
	struct mcux_sdif_data *data = dev->data;
	sdif_transfer_callback_t sdif_cb = {
		.TransferComplete = mcux_sdif_transfer_complete,
	};
	int ret;
	sdif_config_t host_config = {0};

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	host_config.responseTimeout = config->response_timeout;
	host_config.cardDetDebounce_Clock = config->cd_debounce_clocks;
	host_config.dataTimeout = config->data_timeout;
	SDIF_Init(config->base, &host_config);

	SDIF_TransferCreateHandle(config->base, &data->transfer_handle,
		&sdif_cb, (void *)dev);
	config->irq_config_func(dev);

	k_mutex_init(&data->access_mutex);
	k_sem_init(&data->transfer_sem, 0, 1);
	return 0;
}

static int mcux_sdif_get_card_present(const struct device *dev)
{
	const struct mcux_sdif_config *config = dev->config;

	return SDIF_DetectCardInsert(config->base, false);
}


static int mcux_sdif_transfer(const struct device *dev,
			struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	const struct mcux_sdif_config *config = dev->config;
	struct mcux_sdif_data *dev_data = dev->data;
	status_t error;
	sdif_transfer_t transfer = {0};
	sdif_command_t sdif_cmd = {0};
	sdif_data_t sdif_data;

#ifdef CONFIG_MCUX_SDIF_DMA_SUPPORT
	sdif_dma_config_t dma_config = {
		.enableFixBurstLen = false,
		.mode = kSDIF_DualDMAMode,
		.dmaDesBufferStartAddr = dev_data->sdif_dma_descriptor,
		.dmaDesBufferLen = (CONFIG_MCUX_SDIF_DMA_BUFFER_SIZE / 4),
		.dmaDesSkipLen = 0
	};
#endif /* CONFIG_MCUX_SDIF_DMA_SUPPORT */

	if (cmd->opcode == SD_GO_IDLE_STATE) {
		/*
		 * Special handling for CMD0- we want to initialize the card
		 * with 80 clocks, so we will use the SDIF_SendCardActive api
		 * to ensure that CMD0 is sent while the SEND_INITIALIZATION
		 * bit is set in the CMD register.
		 */
		if (!SDIF_SendCardActive(config->base, MCUX_SDIF_DEFAULT_TIMEOUT)) {
			LOG_ERR("Card clock init failed");
			return -EIO;
		}
		return 0;
	}

	/* Copy Zephyr data fields to SDIF struct */
	sdif_cmd.index = cmd->opcode;
	sdif_cmd.argument = cmd->arg;
	/* Lower 4 bits hold native SD response type */
	sdif_cmd.responseType = (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);
	transfer.command = &sdif_cmd;

	if (data) {
		transfer.data = &sdif_data;
		memset(&sdif_data, 0, sizeof(sdif_data));
		sdif_data.blockSize = data->block_size;
		sdif_data.blockCount = data->blocks;
		/*
		 * Determine command type. Note that the driver is expected
		 * to handle CMD12 and CMD23 for multiblock I/O.
		 */
		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
			sdif_data.enableAutoCommand12 = true;
			sdif_data.txData = data->data;
			break;
		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			sdif_data.enableAutoCommand12 = true;
			sdif_data.rxData = data->data;
			break;
		case SD_APP_SEND_SCR:
		case SD_SWITCH:
		case SD_APP_SEND_NUM_WRITTEN_BLK:
			sdif_data.rxData = data->data;
			break;
		default:
			return -ENOTSUP;
		}
	}
	dev_data->transfer_status = 0U;
	k_sem_reset(&dev_data->transfer_sem);
	do {
#ifdef CONFIG_MCUX_SDIF_DMA_SUPPORT
		error = SDIF_TransferNonBlocking(config->base,
			&dev_data->transfer_handle, &dma_config, &transfer);
#else
		error = SDIF_TransferNonBlocking(config->base,
			&dev_data->transfer_handle, NULL, &transfer);
#endif /* CONFIG_MCUX_SDIF_DMA_SUPPORT */
	} while (error == kStatus_SDIF_SyncCmdTimeout && cmd->timeout_ms--);

	if (error != kStatus_Success) {
		return -EIO;
	}
	/* Wait for the command to complete */
	while ((dev_data->transfer_status & TRANSFER_CMD_FLAGS) == 0U) {
		if (k_sem_take(&dev_data->transfer_sem, K_MSEC(cmd->timeout_ms))) {
			return -ETIMEDOUT;
		}
	}
	if (dev_data->transfer_status & TRANSFER_CMD_FAILED) {
		return -EIO;
	}
	/* If data was sent, wait for that to complete */
	if (data) {
		while ((dev_data->transfer_status & TRANSFER_DATA_FLAGS) == 0) {
			if (k_sem_take(&dev_data->transfer_sem, K_MSEC(data->timeout_ms))) {
				return -ETIMEDOUT;
			}
		}
		if (dev_data->transfer_status & TRANSFER_DATA_FAILED) {
			return -EIO;
		}
	}
	/* Record command response */
	memcpy(cmd->response, sdif_cmd.response, sizeof(cmd->response));
	if (data) {
		/* Record bytes transferred */
		data->bytes_xfered = dev_data->transfer_handle.transferredWords;
	}

	return 0;
}

static int mcux_sdif_card_busy(const struct device *dev)
{
	const struct mcux_sdif_config *config = dev->config;

	return (SDIF_GetControllerStatus(config->base) & SDIF_STATUS_DATA_BUSY_MASK) ?
		1 : 0;
}

/* Stops transmission of data using CMD12, after failed command */
static void mcux_sdif_stop_transmission(const struct device *dev)
{
	const struct mcux_sdif_config *config = dev->config;
	struct mcux_sdif_data *data = dev->data;

	sdif_command_t cmd = {0};
	sdif_transfer_t transfer = {
		.command = &cmd,
		.data = NULL,
	};

	cmd.index = SD_STOP_TRANSMISSION;
	cmd.argument = 0;
	cmd.type = kCARD_CommandTypeAbort;
	cmd.responseType = SD_RSP_TYPE_R1b;

	/* Disable transmit interrupt, since we are using blocking transfer */
	SDIF_DisableInterrupt(config->base, kSDIF_AllInterruptStatus);
	SDIF_ClearInterruptStatus(config->base, kSDIF_AllInterruptStatus);

	LOG_WRN("Transfer failed, sending CMD12");
	SDIF_TransferNonBlocking(config->base, &data->transfer_handle, NULL,
		&transfer);
}

static int mcux_sdif_request(const struct device *dev,
			struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	int ret;
	int busy_timeout = MCUX_SDIF_DEFAULT_TIMEOUT;
	struct mcux_sdif_data *dev_data = dev->data;

	ret = k_mutex_lock(&dev_data->access_mutex, K_MSEC(cmd->timeout_ms));
	if (ret) {
		LOG_ERR("Could not access card");
		return -EBUSY;
	}
	do {
		ret = mcux_sdif_transfer(dev, cmd, data);
		if (data && ret) {
			/* Send CMD12 to stop transmission after error */
			mcux_sdif_stop_transmission(dev);
			while (busy_timeout > 0) {
				if (!mcux_sdif_card_busy(dev)) {
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
	} while (ret != 0 && (cmd->retries-- > 0));
	k_mutex_unlock(&dev_data->access_mutex);
	return ret;
}

static DEVICE_API(sdhc, sdif_api) = {
	.reset = mcux_sdif_reset,
	.get_host_props = mcux_sdif_get_host_props,
	.set_io = mcux_sdif_set_io,
	.get_card_present = mcux_sdif_get_card_present,
	.request = mcux_sdif_request,
	.card_busy = mcux_sdif_card_busy,
};

#ifdef CONFIG_MCUX_SDIF_DMA_SUPPORT
#define MCUX_SDIF_DMA_DESCRIPTOR_DEFINE(n)					\
	static uint32_t	mcux_sdif_dma_descriptor_##n				\
		[CONFIG_MCUX_SDIF_DMA_BUFFER_SIZE / 4] __aligned(4);
#define MCUX_SDIF_DMA_DESCRIPTOR_INIT(n)					\
	.sdif_dma_descriptor = mcux_sdif_dma_descriptor_##n,
#else
#define MCUX_SDIF_DMA_DESCRIPTOR_DEFINE(n)
#define MCUX_SDIF_DMA_DESCRIPTOR_INIT(n)
#endif /* CONFIG_MCUX_SDIF_DMA_SUPPORT */


#define MCUX_SDIF_INIT(n)							\
	static void sdif_##n##_irq_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			mcux_sdif_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static const struct mcux_sdif_config sdif_##n##_config = {		\
		.base = (SDIF_Type *) DT_INST_REG_ADDR(n),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.response_timeout = DT_INST_PROP(n, response_timeout),		\
		.cd_debounce_clocks = DT_INST_PROP(n, cd_debounce_clocks),	\
		.data_timeout = DT_INST_PROP(n, data_timeout),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.irq_config_func = sdif_##n##_irq_config_func,			\
	};									\
										\
	MCUX_SDIF_DMA_DESCRIPTOR_DEFINE(n);					\
										\
	static struct mcux_sdif_data sdif_##n##_data = {			\
		MCUX_SDIF_DMA_DESCRIPTOR_INIT(n)				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
		&mcux_sdif_init,						\
		NULL,								\
		&sdif_##n##_data,						\
		&sdif_##n##_config,						\
		POST_KERNEL,							\
		CONFIG_SDHC_INIT_PRIORITY,					\
		&sdif_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_SDIF_INIT)
