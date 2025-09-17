/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdio

#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc_stm32, CONFIG_SDHC_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

BUILD_ASSERT((CONFIG_SDHC_BUFFER_ALIGNMENT % sizeof(uint32_t)) == 0U);

#define SDIO_OCR_SDIO_S18R BIT(24) /* SDIO OCR bit indicating support for 1.8V switching */

struct sdhc_stm32_config {
	bool hw_flow_control;              /* flag for enabling hardware flow control */
	bool support_1_8_v;                /* flag indicating support for 1.8V signaling */
	unsigned int max_freq;             /* Max bus frequency in Hz */
	unsigned int min_freq;             /* Min bus frequency in Hz */
	uint8_t bus_width;                 /* Width of the SDIO bus */
	uint16_t clk_div;                  /* Clock divider value to configure SDIO clock speed */
	uint32_t power_delay_ms;           /* power delay prop for the host in milliseconds */
	uint32_t reg_addr;                 /* Base address of the SDIO peripheral register block */
	SDIO_HandleTypeDef *hsd;           /* Pointer to SDIO HAL handle */
	const struct stm32_pclken *pclken; /* Pointer to peripheral clock configuration */
	const struct pinctrl_dev_config *pcfg; /* Pointer to pin control configuration */
	struct gpio_dt_spec sdhi_on_gpio;  /* Power pin to control the regulators used by card.*/
	struct gpio_dt_spec cd_gpio;       /* Card detect GPIO pin */
	irq_config_func_t irq_config_func; /* IRQ config function */
};

struct sdhc_stm32_data {
	struct k_mutex bus_mutex;      /* Sync between commands */
	struct sdhc_io host_io;        /* Input/Output host configuration */
	uint32_t cmd_index;            /* current command opcode */
	struct sdhc_host_props props;  /* current host properties */
	struct k_sem device_sync_sem;  /* Sync between device communication messages */
	void *sdio_dma_buf;            /* DMA buffer for SDIO data transfer */
	uint32_t total_transfer_bytes; /* number of bytes transferred */
};

/*
 * Power on the card.
 *
 * This function toggles a GPIO to control the internal regulator used
 * by the card device. It handles GPIO configuration and timing delays.
 *
 * @param dev Device structure pointer.
 *
 * @return 0 on success, non-zero code on failure.
 */
static int sdhi_power_on(const struct device *dev)
{
	int ret;
	const struct sdhc_stm32_config *config = dev->config;

	if (!device_is_ready(config->sdhi_on_gpio.port)) {
		LOG_ERR("Card is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->sdhi_on_gpio, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Card configuration failed, ret:%d", ret);
		return ret;
	}

	k_msleep(config->power_delay_ms);
	return ret;
}

/**
 * Logs detailed SDIO error types using Zephyr's logging subsystem.
 *
 * This helper function queries the error status of an SDIO operation
 * and reports specific error types using `LOG_ERR()`.
 * In addition to logging, it also resets the `ErrorCode` field
 * of the `SDIO_HandleTypeDef` to `HAL_SDIO_ERROR_NONE`.
 *
 * @param hsd Pointer to the SDIO handle.
 */
static void sdhc_stm32_log_err_type(SDIO_HandleTypeDef *hsd)
{
	uint32_t error_code = HAL_SDIO_GetError(hsd);

	if ((error_code & HAL_SDIO_ERROR_TIMEOUT) != 0U) {
		LOG_ERR("SDIO Timeout");
	}

	if ((error_code & HAL_SDIO_ERROR_DATA_TIMEOUT) != 0U) {
		LOG_ERR("SDIO Data Timeout");
	}

	if ((error_code & HAL_SDIO_ERROR_DATA_CRC_FAIL) != 0U) {
		LOG_ERR("SDIO Data CRC");
	}

	if ((error_code & HAL_SDIO_ERROR_TX_UNDERRUN) != 0U) {
		LOG_ERR("SDIO FIFO Transmit Underrun");
	}

	if ((error_code & HAL_SDIO_ERROR_RX_OVERRUN) != 0U) {
		LOG_ERR("SDIO FIFO Receive Overrun");
	}

	if ((error_code & HAL_SDIO_ERROR_INVALID_CALLBACK) != 0U) {
		LOG_ERR("SDIO Invalid Callback");
	}

	if ((error_code & SDMMC_ERROR_ADDR_MISALIGNED) != 0U) {
		LOG_ERR("SDIO Misaligned address");
	}

	if ((error_code & SDMMC_ERROR_WRITE_PROT_VIOLATION) != 0U) {
		LOG_ERR("Attempt to program a write protected block");
	}

	if ((error_code & SDMMC_ERROR_ILLEGAL_CMD) != 0U) {
		LOG_ERR("Command is not legal for the card state");
	}

	hsd->ErrorCode = HAL_SDIO_ERROR_NONE;
}

/**
 * @brief  No-operation callback for SDIO card identification.
 * @param  hsd: Pointer to an SDIO_HandleTypeDef structure that contains
 *         the configuration information for SDIO module.
 * @retval HAL status
 */
static HAL_StatusTypeDef noop_identify_card_callback(SDIO_HandleTypeDef *hsd)
{
	ARG_UNUSED(hsd);
	return HAL_OK;
}

/**
 * Initializes the SDIO peripheral with the configuration specified.
 *
 * This includes deinitializing any previous configuration, and applying
 * parameters like clock edge, power saving, clock divider, hardware
 * flow control and bus width.
 *
 * @param dev Pointer to the device structure for the SDIO peripheral.
 *
 * @return 0 on success, err code on failure.
 */
static int sdhc_stm32_sd_init(const struct device *dev)
{
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	SDIO_HandleTypeDef *hsd = config->hsd;

	data->host_io.bus_width = config->bus_width;
	hsd->Instance = (MMC_TypeDef *) config->reg_addr;

	if (HAL_SDIO_DeInit(hsd) != HAL_OK) {
		LOG_ERR("Failed to de-initialize the SDIO device");
		return -EIO;
	}

	hsd->Init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
	hsd->Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd->Init.ClockDiv = config->clk_div;

	if (config->hw_flow_control) {
		hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	} else {
		hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	}

	if (data->host_io.bus_width == SDHC_BUS_WIDTH4BIT) {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_4B;
	} else if (data->host_io.bus_width == SDHC_BUS_WIDTH8BIT) {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_8B;
	} else {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_1B;
	}

	if (HAL_SDIO_RegisterIdentifyCardCallback(config->hsd,
						  noop_identify_card_callback) != HAL_OK) {
		LOG_ERR("Register identify card callback failed");
		return -EIO;
	}

	if (HAL_SDIO_Init(hsd) != HAL_OK) {
		return -EIO;
	}
	return 0;
}

static int sdhc_stm32_activate(const struct device *dev)
{
	int ret;
	const struct sdhc_stm32_config *config = (struct sdhc_stm32_config *)dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk,
					    (clock_control_subsys_t)(uintptr_t) &config->pclken[1],
					    NULL) != 0) {
			LOG_ERR("Failed to enable SDHC domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)(uintptr_t)&config->pclken[0]) != 0) {
		return -EIO;
	}

	return 0;
}

static uint32_t sdhc_stm32_go_idle_state(const struct device *dev)
{
	const struct sdhc_stm32_config *config = dev->config;

	return SDMMC_CmdGoIdleState(config->hsd->Instance);
}

static int sdhc_stm32_rw_direct(const struct device *dev, struct sdhc_command *cmd)
{
	HAL_StatusTypeDef res;
	const struct sdhc_stm32_config *config = dev->config;
	bool direction = cmd->arg >> SDIO_CMD_ARG_RW_SHIFT;
	bool raw_flag = cmd->arg >> SDIO_DIRECT_CMD_ARG_RAW_SHIFT;

	uint8_t func = (cmd->arg >> SDIO_CMD_ARG_FUNC_NUM_SHIFT) & 0x7;
	uint32_t reg_addr = (cmd->arg >> SDIO_CMD_ARG_REG_ADDR_SHIFT) & SDIO_CMD_ARG_REG_ADDR_MASK;

	HAL_SDIO_DirectCmd_TypeDef arg = {
		.Reg_Addr = reg_addr,
		.ReadAfterWrite = raw_flag,
		.IOFunctionNbr = func
	};

	if (direction == SDIO_IO_WRITE) {
		uint8_t data_in = cmd->arg & SDIO_DIRECT_CMD_DATA_MASK;

		res = HAL_SDIO_WriteDirect(config->hsd, &arg, data_in);
	} else {
		res = HAL_SDIO_ReadDirect(config->hsd, &arg, (uint8_t *)&cmd->response);
	}

	return res == HAL_OK ? 0 : -EIO;
}

static int sdhc_stm32_rw_extended(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	HAL_StatusTypeDef res;
	struct sdhc_stm32_data *dev_data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	bool direction = cmd->arg >> SDIO_CMD_ARG_RW_SHIFT;
	bool increment = cmd->arg & BIT(SDIO_EXTEND_CMD_ARG_OP_CODE_SHIFT);
	bool is_block_mode = cmd->arg & BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT);
	uint8_t func = (cmd->arg >> SDIO_CMD_ARG_FUNC_NUM_SHIFT) & 0x7;
	uint32_t reg_addr = (cmd->arg >> SDIO_CMD_ARG_REG_ADDR_SHIFT) & SDIO_CMD_ARG_REG_ADDR_MASK;

	if (data->data == NULL) {
		LOG_ERR("Invalid NULL data buffer passed to CMD53");
		return -EINVAL;
	}

	HAL_SDIO_ExtendedCmd_TypeDef arg = {
		.Reg_Addr = reg_addr,
		.IOFunctionNbr = func,
		.Block_Mode = is_block_mode ? SDMMC_SDIO_MODE_BLOCK : HAL_SDIO_MODE_BYTE,
		.OpCode = increment
	};

	config->hsd->block_size = is_block_mode ? data->block_size : 0;
	dev_data->total_transfer_bytes = data->blocks * data->block_size;

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_SUPPORT)) {
		dev_data->sdio_dma_buf = k_aligned_alloc(CONFIG_SDHC_BUFFER_ALIGNMENT,
							data->blocks * data->block_size);
		if (dev_data->sdio_dma_buf == NULL) {
			LOG_ERR("DMA buffer allocation failed");
			return -ENOMEM;
		}
	}

	if (direction == SDIO_IO_WRITE) {
		if (IS_ENABLED(CONFIG_SDHC_STM32_POLLING_SUPPORT)) {
			res = HAL_SDIO_WriteExtended(config->hsd, &arg, data->data,
							dev_data->total_transfer_bytes,
							data->timeout_ms);
		} else {
			memcpy(dev_data->sdio_dma_buf, data->data,
				dev_data->total_transfer_bytes);
			sys_cache_data_flush_range(dev_data->sdio_dma_buf,
							dev_data->total_transfer_bytes);
			res = HAL_SDIO_WriteExtended_DMA(config->hsd, &arg,
							dev_data->sdio_dma_buf,
							dev_data->total_transfer_bytes);
		}
	} else {
		if (IS_ENABLED(CONFIG_SDHC_STM32_POLLING_SUPPORT)) {
			res = HAL_SDIO_ReadExtended(config->hsd, &arg, data->data,
							dev_data->total_transfer_bytes,
							data->timeout_ms);
		} else {
			sys_cache_data_flush_range(dev_data->sdio_dma_buf,
						dev_data->total_transfer_bytes);
			res = HAL_SDIO_ReadExtended_DMA(config->hsd, &arg, dev_data->sdio_dma_buf,
							dev_data->total_transfer_bytes);
		}
	}

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_SUPPORT)) {
		/* Wait for whole transfer to complete */
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(CONFIG_SD_CMD_TIMEOUT)) != 0) {
			k_free(dev_data->sdio_dma_buf);
			return -ETIMEDOUT;
		}

		if (direction == SDIO_IO_READ) {
			sys_cache_data_invd_range(dev_data->sdio_dma_buf,
						dev_data->total_transfer_bytes);
			memcpy(data->data, dev_data->sdio_dma_buf, data->block_size * data->blocks);
		}

		k_free(dev_data->sdio_dma_buf);
	}

	return res == HAL_OK ? 0 : -EIO;
}

static int sdhc_stm32_switch_to_1_8v(const struct device *dev)
{
	uint32_t res;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	/* Check if host supports 1.8V signaling */
	if (!data->props.host_caps.vol_180_support) {
		LOG_ERR("Host does not support 1.8V signaling");
		return -ENOTSUP;
	}

	res = SDMMC_CmdVoltageSwitch(config->hsd->Instance);
	if (res != 0) {
		LOG_ERR("CMD11 failed: %#x", res);
		return -EIO;
	}

	LOG_DBG("Successfully switched to 1.8V signaling");
	return 0;
}

static int sdhc_stm32_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	int res = 0;
	uint32_t sdmmc_res = 0U;
	struct sdhc_stm32_data *dev_data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	__ASSERT_NO_MSG(cmd != NULL);

	if (k_mutex_lock(&dev_data->bus_mutex, K_MSEC(cmd->timeout_ms))) {
		return -EBUSY;
	}

	if (HAL_SDIO_GetState(config->hsd) != HAL_SDIO_STATE_READY) {
		LOG_ERR("SDIO Card is busy");
		k_mutex_unlock(&dev_data->bus_mutex);
		return -ETIMEDOUT;
	}

	(void)pm_device_runtime_get(dev);

	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	dev_data->cmd_index = cmd->opcode;
	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		sdmmc_res = sdhc_stm32_go_idle_state(dev);
		if (sdmmc_res != 0U) {
			res = -EIO;
		}
		break;

	case SD_SELECT_CARD:
		sdmmc_res = SDMMC_CmdSelDesel(config->hsd->Instance, cmd->arg);
		if (sdmmc_res != 0U) {
			res = -EIO;
			break;
		}
		/* Clear unused flags to avoid SDIO card identification issues */
		cmd->response[0U] &=
			~(SD_R1_ERASE_SKIP | SD_R1_CSD_OVERWRITE | SD_R1_ERASE_PARAM);
		break;

	case SD_SEND_RELATIVE_ADDR:
		sdmmc_res = SDMMC_CmdSetRelAdd(config->hsd->Instance, (uint16_t *)&cmd->response);
		if (sdmmc_res != 0U) {
			res = -EIO;
			break;
		}
		/*
		 * Restore RCA by reversing the double 16-bit right shift from
		 * Zephyr subsys and SDMMC_CmdSetRelAdd
		 */
		cmd->response[0] = cmd->response[0] << 16;
		break;

	case SDIO_SEND_OP_COND:
		sdmmc_res = SDMMC_CmdSendOperationcondition(config->hsd->Instance, cmd->arg,
								(uint32_t *)&cmd->response);
		if (sdmmc_res != 0U) {
			res = -EIO;
		}
		break;

	case SDIO_RW_DIRECT:
		res = sdhc_stm32_rw_direct(dev, cmd);
		break;

	case SDIO_RW_EXTENDED:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_rw_extended(dev, cmd, data);
		break;

	case SD_VOL_SWITCH:
		res = sdhc_stm32_switch_to_1_8v(dev);
		break;

	default:
		LOG_DBG("Unsupported Command, opcode:%d", cmd->opcode);
		res = -ENOTSUP;
	}

	if (res != 0) {
		LOG_DBG("Command Failed, opcode:%d", cmd->opcode);
		sdhc_stm32_log_err_type(config->hsd);
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);
	k_mutex_unlock(&dev_data->bus_mutex);

	return res;
}

static int sdhc_stm32_set_io(const struct device *dev, struct sdhc_io *ios)
{
	int res = 0;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_io *host_io = &data->host_io;
	struct sdhc_host_props *props = &data->props;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	if ((ios->clock != 0) && (host_io->clock != ios->clock)) {
		if ((ios->clock > props->f_max) || (ios->clock < props->f_min)) {
			LOG_ERR("Invalid clock frequency, domain (%u, %u)",
				props->f_min, props->f_max);
			res = -EINVAL;
			goto out;
		}
		if (HAL_SDIO_ConfigFrequency(config->hsd, (uint32_t)ios->clock) != HAL_OK) {
			LOG_ERR("Failed to set clock to %d", ios->clock);
			res = -EIO;
			goto out;
		}
		host_io->clock = ios->clock;
		LOG_DBG("Clock set to %d", ios->clock);
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		(void)SDMMC_PowerState_OFF(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	} else {
		(void)SDMMC_PowerState_ON(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	}

	if ((ios->bus_width != 0) && (host_io->bus_width != ios->bus_width)) {
		uint32_t bus_width_reg_value;

		if (ios->bus_width == SDHC_BUS_WIDTH8BIT) {
			bus_width_reg_value = SDMMC_BUS_WIDE_8B;
		} else if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			bus_width_reg_value = SDMMC_BUS_WIDE_4B;
		} else {
			bus_width_reg_value = SDMMC_BUS_WIDE_1B;
		}

		MODIFY_REG(config->hsd->Instance->CLKCR, SDMMC_CLKCR_WIDBUS, bus_width_reg_value);
		host_io->bus_width = ios->bus_width;
	}

out:
	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res;
}

static void sdhc_stm32_init_props(const struct device *dev)
{
	const struct sdhc_stm32_config *sdhc_config = (const struct sdhc_stm32_config *)dev->config;
	struct sdhc_stm32_data *data = dev->data;
	struct sdhc_host_props *props = &data->props;

	memset(props, 0, sizeof(struct sdhc_host_props));

	props->f_min = sdhc_config->min_freq;
	props->f_max = sdhc_config->max_freq;
	props->power_delay = sdhc_config->power_delay_ms;
	props->host_caps.vol_330_support = true;
	props->host_caps.vol_180_support = sdhc_config->support_1_8_v;
	props->host_caps.bus_8_bit_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH8BIT);
	props->host_caps.bus_4_bit_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH4BIT);
}

static int sdhc_stm32_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_stm32_data *data = dev->data;

	memcpy(props, &data->props, sizeof(struct sdhc_host_props));

	return 0;
}

static int sdhc_stm32_get_card_present(const struct device *dev)
{
	int res = 0;
	struct sdhc_stm32_data *dev_data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	/* If a CD pin is configured, use it for card detection */
	if (config->cd_gpio.port != NULL) {
		res = gpio_pin_get_dt(&config->cd_gpio);
		return res;
	}

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&dev_data->bus_mutex, K_FOREVER);

	/* Card is considered present if the read command did not time out */
	if (SDMMC_CmdSendOperationcondition(config->hsd->Instance, 0, NULL) != 0) {
		res = -EIO;
		sdhc_stm32_log_err_type(config->hsd);
	}
	k_mutex_unlock(&dev_data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res == 0;
}

static int sdhc_stm32_card_busy(const struct device *dev)
{
	const struct sdhc_stm32_config *config = dev->config;

	return HAL_SDIO_GetState(config->hsd) == HAL_SDIO_STATE_BUSY;
}

static int sdhc_stm32_reset(const struct device *dev)
{
	HAL_StatusTypeDef res;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Resetting Host controller */
	(void)SDMMC_PowerState_OFF(config->hsd->Instance);
	k_msleep(data->props.power_delay);
	(void)SDMMC_PowerState_ON(config->hsd->Instance);
	k_msleep(data->props.power_delay);

	/* Resetting card */
	res = HAL_SDIO_CardReset(config->hsd);
	if (res != HAL_OK) {
		LOG_ERR("Card reset failed");
	}

	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res == HAL_OK ? 0 : -EIO;
}

static DEVICE_API(sdhc, sdhc_stm32_api) = {
	.request = sdhc_stm32_request,
	.set_io = sdhc_stm32_set_io,
	.get_host_props = sdhc_stm32_get_host_props,
	.get_card_present = sdhc_stm32_get_card_present,
	.card_busy = sdhc_stm32_card_busy,
	.reset = sdhc_stm32_reset,
};

void sdhc_stm32_event_isr(const struct device *dev)
{
	uint32_t icr_clear_flag = 0;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	if (__HAL_SDIO_GET_FLAG(config->hsd,
				SDMMC_FLAG_DATAEND | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
				SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR)) {
		k_sem_give(&data->device_sync_sem);
	}

	if ((config->hsd->Instance->STA & SDMMC_STA_DCRCFAIL) != 0U) {
		icr_clear_flag |= SDMMC_ICR_DCRCFAILC;
	}
	if ((config->hsd->Instance->STA & SDMMC_STA_DTIMEOUT) != 0U) {
		icr_clear_flag |= SDMMC_ICR_DTIMEOUTC;
	}
	if ((config->hsd->Instance->STA & SDMMC_STA_TXUNDERR) != 0U) {
		icr_clear_flag |= SDMMC_ICR_TXUNDERRC;
	}
	if ((config->hsd->Instance->STA & SDMMC_STA_RXOVERR) != 0U) {
		icr_clear_flag |= SDMMC_ICR_RXOVERRC;
	}
	if (icr_clear_flag) {
		LOG_ERR("SDMMC interrupt err flag raised: 0x%08X", icr_clear_flag);
		config->hsd->Instance->ICR = icr_clear_flag;
	}

	HAL_SDIO_IRQHandler(config->hsd);
}

static int sdhc_stm32_init(const struct device *dev)
{
	int ret;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	if (config->sdhi_on_gpio.port != NULL) {
		if (sdhi_power_on(dev) != 0) {
			LOG_ERR("Failed to power card on");
			return -ENODEV;
		}
	}

	if (config->cd_gpio.port != NULL) {
		if (!device_is_ready(config->cd_gpio.port)) {
			LOG_ERR("Card detect GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->cd_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Couldn't configure card-detect pin; (%d)", ret);
			return ret;
		}
	}

	ret = sdhc_stm32_activate(dev);
	if (ret != 0) {
		LOG_ERR("Clock and GPIO could not be initialized for the SDHC module, err=%d", ret);
		return ret;
	}

	ret = sdhc_stm32_sd_init(dev);
	if (ret != 0) {
		LOG_ERR("SDIO Init Failed");
		sdhc_stm32_log_err_type(config->hsd);
		return ret;
	}

	LOG_INF("SDIO Init Passed Successfully");

	sdhc_stm32_init_props(dev);

	config->irq_config_func();
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	k_mutex_init(&data->bus_mutex);

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int sdhc_stm32_suspend(const struct device *dev)
{
	int ret;
	const struct sdhc_stm32_config *cfg = (struct sdhc_stm32_config *)dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Disable device clock. */
	ret = clock_control_off(clk, (clock_control_subsys_t)(uintptr_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed to disable SDHC clock during PM suspend process");
		return ret;
	}

	/* Move pins to sleep state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
	if (ret == -ENOENT) {
		/* Warn but don't block suspend */
		LOG_WRN_ONCE("SDHC pinctrl sleep state not available");
		return 0;
	}

	return ret;
}

static int sdhc_stm32_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return sdhc_stm32_activate(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return sdhc_stm32_suspend(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define STM32_SDHC_IRQ_HANDLER(index)	\
	static void sdhc_stm32_irq_config_func_##index(void)					\
	{											\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, event, irq),				\
			    DT_INST_IRQ_BY_NAME(index, event, priority), sdhc_stm32_event_isr,	\
			    DEVICE_DT_INST_GET(index), 0);					\
		irq_enable(DT_INST_IRQ_BY_NAME(index, event, irq));				\
	}

#define SDHC_STM32_INIT(index)									\
												\
	STM32_SDHC_IRQ_HANDLER(index)								\
												\
	static SDIO_HandleTypeDef hsd_##index;							\
												\
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);	\
												\
	PINCTRL_DT_INST_DEFINE(index);								\
												\
	static const struct sdhc_stm32_config sdhc_stm32_cfg_##index = {			\
		.hsd = &hsd_##index,								\
		.reg_addr = DT_INST_REG_ADDR(index),						\
		.irq_config_func = sdhc_stm32_irq_config_func_##index,				\
		.pclken = pclken_##index,							\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),					\
		.hw_flow_control = DT_INST_PROP(index, hw_flow_control),			\
		.clk_div = DT_INST_PROP(index, clk_div),					\
		.bus_width = DT_INST_PROP(index, bus_width),					\
		.power_delay_ms = DT_INST_PROP(index, power_delay_ms),				\
		.support_1_8_v = DT_INST_PROP(index, support_1_8_v),				\
		.sdhi_on_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), sdhi_on_gpios, {0}),	\
		.cd_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), cd_gpios, {0}),		\
		.min_freq = DT_INST_PROP(index, min_bus_freq),					\
		.max_freq = DT_INST_PROP(index, max_bus_freq),					\
	};											\
												\
	static struct sdhc_stm32_data sdhc_stm32_data_##index;					\
												\
	PM_DEVICE_DT_INST_DEFINE(index, sdhc_stm32_pm_action);					\
												\
	DEVICE_DT_INST_DEFINE(index, &sdhc_stm32_init, NULL, &sdhc_stm32_data_##index,		\
				&sdhc_stm32_cfg_##index, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,\
				&sdhc_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_STM32_INIT)
