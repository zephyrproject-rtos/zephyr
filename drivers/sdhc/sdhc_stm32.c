/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdhc

#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc_stm32, CONFIG_SDHC_LOG_LEVEL);

#define SD_TIMEOUT         ((uint32_t)0x00100000U) /* Timeout value for SD card operations*/
#define STM32_SDIO_F_MIN   SDMMC_CLOCK_400KHZ
#define STM32_SDIO_F_MAX   (MHZ(208))  /* Maximum frequency supported by the SDIO interface */
#define SDIO_OCR_SDIO_S18R (1U << 24U) /* SDIO OCR bit indicating support for 1.8V switching */

struct sdhc_stm32_config {
	bool hw_flow_control;        /* flag for enabling hardware flow control */
	bool support_1_8_v;          /* flag indicating support for 1.8V signaling */
	unsigned int max_freq;       /* Max bus frequency */
	unsigned int min_freq;       /* Min bus frequency */
	uint8_t bus_width;           /* Width of the SDIO bus (1-bit or 4-bit mode) */
	uint16_t clk_div;            /* Clock divider value to configure SDIO clock speed */
	uint32_t power_delay_ms;     /* power delay prop for the host */
	SDIO_HandleTypeDef *hsd;     /* Pointer to SDIO HAL handle */
	struct stm32_pclken *pclken; /* Pointer to peripheral clock configuration */
	const struct pinctrl_dev_config *pcfg; /* Pointer to pin control configuration */
	struct gpio_dt_spec sdhi_on_gpio; /* Power pin to control the regulators used by card.*/
	struct gpio_dt_spec cd_gpio; /* Card detect GPIO pin */
};

struct sdhc_stm32_data {
	struct k_mutex bus_mutex;     /* Sync between commands */
	struct sdhc_io host_io;       /* Input/Output host configuration */
	uint32_t cmd_index;           /* current command opcode */
	struct sdhc_host_props props; /* currect host properties */
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
		LOG_ERR("Card is not ready.");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->sdhi_on_gpio, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Error %d: Card configuration failed.", ret);
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
 *
 * @param hsd Pointer to the SDIO handle.
 */
static void sdhc_stm32_log_err_type(SDIO_HandleTypeDef *hsd)
{
	uint32_t error_code = HAL_SDIO_GetError(hsd);

	if (error_code & HAL_SDIO_ERROR_TIMEOUT) {
		LOG_ERR("SDIO Timeout Error\n");
	}

	if (error_code & HAL_SDIO_ERROR_DATA_TIMEOUT) {
		LOG_ERR("SDIO Data Timeout Error\n");
	}

	if (error_code & HAL_SDIO_ERROR_DATA_CRC_FAIL) {
		LOG_ERR("SDIO Data CRC Error\n");
	}

	if (error_code & HAL_SDIO_ERROR_TX_UNDERRUN) {
		LOG_ERR("SDIO FIFO Transmit Underrun Error\n");
	}

	if (error_code & HAL_SDIO_ERROR_RX_OVERRUN) {
		LOG_ERR("SDIO FIFO Receive Overrun Error\n");
	}

	if (error_code & HAL_SDIO_ERROR_INVALID_CALLBACK) {
		LOG_ERR("SDIO Invalid Callback Error\n");
	}

	if (error_code & SDMMC_ERROR_ADDR_MISALIGNED) {
		LOG_ERR("SDIO Misaligned address Error\n");
	}

	if (error_code & SDMMC_ERROR_WRITE_PROT_VIOLATION) {
		LOG_ERR("Attempt to program a write protect block\n");
	}

	if (error_code & SDMMC_ERROR_ILLEGAL_CMD) {
		LOG_ERR("Command is not legal for the card state\n");
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
 * @return HAL_OK on success, HAL_ERROR on failure.
 */
static int sdhc_stm32_sd_init(const struct device *dev)
{
	int res;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	SDIO_HandleTypeDef *hsd = config->hsd;

	if (HAL_SDIO_DeInit(hsd) != HAL_OK) {
		LOG_ERR("Failed to de-initialize the SDIO device\n");
		return HAL_ERROR;
	}

	hsd->Init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
	hsd->Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd->Init.ClockDiv = config->clk_div;

	if (config->hw_flow_control) {
		hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	} else {
		hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	}

	if (data->host_io.bus_width == 4) {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_4B;
	} else if (data->host_io.bus_width == 8) {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_8B;
	} else {
		hsd->Init.BusWide = SDMMC_BUS_WIDE_1B;
	}

	res = HAL_SDIO_RegisterIdentifyCardCallback(config->hsd, noop_identify_card_callback);
	if (res != HAL_OK) {
		LOG_ERR("Register identify card callback failed.\n");
		return res;
	}

	return HAL_SDIO_Init(hsd);
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
		if (clock_control_configure(clk, (clock_control_subsys_t)&config->pclken[1],
					    NULL) != 0) {
			LOG_ERR("Failed to enable SDHC domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]) != 0) {
		return -EIO;
	}

	return ret;
}

static uint32_t sdhc_stm32_go_idle_state(const struct device *dev)
{
	const struct sdhc_stm32_config *config = dev->config;

	return SDMMC_CmdGoIdleState(config->hsd->Instance);
}

static int sdhc_stm32_rw_direct(const struct device *dev, struct sdhc_command *cmd)
{
	int res;
	const struct sdhc_stm32_config *config = dev->config;
	bool direction = (cmd->arg >> SDIO_CMD_ARG_RW_SHIFT);
	bool raw_flag = (cmd->arg >> SDIO_DIRECT_CMD_ARG_RAW_SHIFT);

	uint8_t func = (cmd->arg >> SDIO_CMD_ARG_FUNC_NUM_SHIFT) & 0x7;
	uint32_t reg_addr = (cmd->arg >> SDIO_CMD_ARG_REG_ADDR_SHIFT) & SDIO_CMD_ARG_REG_ADDR_MASK;

	HAL_SDIO_DirectCmd_TypeDef arg = {
		.Reg_Addr = reg_addr, .ReadAfterWrite = raw_flag, .IOFunctionNbr = func};

	if (direction == SDIO_IO_WRITE) {
		uint8_t data_in = cmd->arg & SDIO_DIRECT_CMD_DATA_MASK;

		res = HAL_SDIO_WriteDirect(config->hsd, &arg, data_in);
	} else {
		res = HAL_SDIO_ReadDirect(config->hsd, &arg, (uint8_t *)&cmd->response);
	}

	return res;
}

static int sdhc_stm32_rw_extended(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	int res;
	const struct sdhc_stm32_config *config = dev->config;
	bool direction = (cmd->arg >> SDIO_CMD_ARG_RW_SHIFT);
	bool increment = (cmd->arg & BIT(SDIO_EXTEND_CMD_ARG_OP_CODE_SHIFT));
	bool is_block_mode = (cmd->arg & BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT));
	uint8_t func = (cmd->arg >> SDIO_CMD_ARG_FUNC_NUM_SHIFT) & 0x7;
	uint32_t reg_addr = (cmd->arg >> SDIO_CMD_ARG_REG_ADDR_SHIFT) & SDIO_CMD_ARG_REG_ADDR_MASK;

	HAL_SDIO_ExtendedCmd_TypeDef arg = {.Reg_Addr = reg_addr,
					    .IOFunctionNbr = func,
					    .Block_Mode = is_block_mode ? SDMMC_SDIO_MODE_BLOCK
									: HAL_SDIO_MODE_BYTE,
					    .OpCode = increment};

	config->hsd->block_size = data->block_size;

	if (direction == SDIO_IO_WRITE) {
		if (data->data) {
			res = HAL_SDIO_WriteExtended(config->hsd, &arg, data->data,
						     ((data->blocks) * (data->block_size)),
						     data->timeout_ms);
		} else {
			LOG_ERR("Invalid data buffer passed to CMD53 (write).");
			res = -EINVAL;
		}
	} else {
		res = HAL_SDIO_ReadExtended(config->hsd, &arg, (uint8_t *)&cmd->response,
					    ((data->blocks) * (data->block_size)),
					    data->timeout_ms);
	}

	return res;
}

static int sdhc_stm32_switch_to_1_8v(const struct device *dev)
{
	uint32_t res = 0;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	/* Check if host supports 1.8V signaling */
	if (!data->props.host_caps.vol_180_support) {
		LOG_ERR("Host does not support 1.8V signaling");
		return -ENOTSUP;
	}

	res = SDMMC_CmdVoltageSwitch(config->hsd->Instance);
	if (res != HAL_OK) {
		LOG_ERR("CMD11 failed: %u", res);
		return -EIO;
	}

	LOG_INF("Successfully switched to 1.8V signaling");
	return res;
}

static int sdhc_stm32_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	int res;
	struct sdhc_stm32_data *dev_data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	__ASSERT(cmd != NULL, "Command is NULL.");

	if (!WAIT_FOR(HAL_SDIO_GetState(config->hsd) == HAL_SDIO_STATE_READY, SD_TIMEOUT,
		      k_usleep(1))) {
		LOG_ERR("SD Card is busy now");
		return HAL_ERROR;
	}

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&dev_data->bus_mutex, K_FOREVER);

	dev_data->cmd_index = cmd->opcode;
	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		res = sdhc_stm32_go_idle_state(dev);
		break;

	case SD_SELECT_CARD:
		res = SDMMC_CmdSelDesel(config->hsd->Instance, cmd->arg);
		break;

	case SD_SEND_RELATIVE_ADDR:
		res = SDMMC_CmdSetRelAdd(config->hsd->Instance, (uint16_t *)&cmd->response);
		if (res == HAL_OK) {
			cmd->response[0] = (uint32_t)(cmd->response[0] << 16);
		}
		break;

	case SDIO_SEND_OP_COND:
		res = SDMMC_CmdSendOperationcondition(config->hsd->Instance, cmd->arg,
						      (uint32_t *)&cmd->response);
		break;

	case SDIO_RW_DIRECT:
		res = sdhc_stm32_rw_direct(dev, cmd);
		break;

	case SDIO_RW_EXTENDED:
		__ASSERT(data != NULL, "data is NULL.");
		res = sdhc_stm32_rw_extended(dev, cmd, data);
		break;

	case SD_VOL_SWITCH:
		res = sdhc_stm32_switch_to_1_8v(dev);
		break;

	default:
		res = -ENOTSUP;
		LOG_DBG("Unsupported Command, opcode:%d\n.", cmd->opcode);
		break;
	}

	if (res != HAL_OK) {
		LOG_ERR("Command Failed, opcode:%d\n.", cmd->opcode);
		sdhc_stm32_log_err_type(config->hsd);
	}

	k_mutex_unlock(&dev_data->bus_mutex);
	(void)pm_device_runtime_put(dev);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return res;
}

static int sdhc_stm32_set_io(const struct device *dev, struct sdhc_io *ios)
{
	int res = 0;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_io *host_io = &data->host_io;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	if ((ios->clock != 0) && (host_io->clock != ios->clock)) {
		if ((ios->clock > STM32_SDIO_F_MAX) || (ios->clock < STM32_SDIO_F_MIN)) {
			LOG_ERR("Invalid Clock Frequency\n");
			k_mutex_unlock(&data->bus_mutex);
			(void)pm_device_runtime_put(dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			return -EINVAL;
		}
		res = HAL_SDIO_ConfigFrequency(config->hsd, (uint32_t)ios->clock);
		if (res != HAL_OK) {
			LOG_ERR("Set clock as %d failed\n", ios->clock);
		} else {
			host_io->clock = ios->clock;
			LOG_INF("set SDHC clock as %d\n", ios->clock);
		}
		k_msleep(10);
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		(void)SDMMC_PowerState_OFF(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	} else {
		(void)SDMMC_PowerState_ON(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	}

	if ((ios->bus_width != 0) && (host_io->bus_width != ios->bus_width)) {
		MODIFY_REG(config->hsd->Instance->CLKCR, SDMMC_CLKCR_WIDBUS,
			   (ios->bus_width == SDHC_BUS_WIDTH4BIT) ? SDMMC_BUS_WIDE_4B
								  : SDMMC_BUS_WIDE_1B);
		host_io->bus_width = ios->bus_width;
	}

	k_mutex_unlock(&data->bus_mutex);
	(void)pm_device_runtime_put(dev);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

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
	props->host_caps.vol_300_support = false;
	props->host_caps.bus_8_bit_support = (sdhc_config->bus_width == 8);
	props->host_caps.bus_4_bit_support = (sdhc_config->bus_width == 4);
	props->host_caps.hs200_support = false;
	props->host_caps.hs400_support = false;
	props->host_caps.high_spd_support = true;
	props->host_caps.sdr50_support = true;
	props->host_caps.sdio_async_interrupt_support = true;
	props->is_spi = false;
}

static int sdhc_stm32_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_stm32_data *data = dev->data;

	memcpy(props, &data->props, sizeof(struct sdhc_host_props));

	return 0;
}

static int sdhc_stm32_get_card_present(const struct device *dev)
{
	int res;
	struct sdhc_stm32_data *dev_data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&dev_data->bus_mutex, K_FOREVER);

	/* If a CD pin is configured, use it for card detection */
	if (config->cd_gpio.port != NULL) {
		if (!device_is_ready(config->cd_gpio.port)) {
			LOG_ERR("Card detect GPIO device not ready");
			return -ENODEV;
		}
		res = gpio_pin_get_dt(&config->cd_gpio);
	} else {
		/* Card is considered present if the read command did not time out */
		res = SDMMC_CmdSendOperationcondition(config->hsd->Instance, 0, NULL);
		if (res != HAL_OK) {
			sdhc_stm32_log_err_type(config->hsd);
		}
	}

	k_mutex_unlock(&dev_data->bus_mutex);
	(void)pm_device_runtime_put(dev);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return (res == HAL_OK);
}

static int sdhc_stm32_card_busy(const struct device *dev)
{
	const struct sdhc_stm32_config *config = dev->config;

	HAL_SDIO_StateTypeDef state = HAL_SDIO_GetState(config->hsd);

	return (state == HAL_SDIO_STATE_BUSY);
}

static int sdhc_stm32_reset(const struct device *dev)
{
	int res;
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
		LOG_ERR("Card reset failed.\n");
	}

	k_mutex_unlock(&data->bus_mutex);
	(void)pm_device_runtime_put(dev);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return res;
}

static DEVICE_API(sdhc, sdhc_stm32_api) = {
	.request = sdhc_stm32_request,
	.set_io = sdhc_stm32_set_io,
	.get_host_props = sdhc_stm32_get_host_props,
	.get_card_present = sdhc_stm32_get_card_present,
	.card_busy = sdhc_stm32_card_busy,
	.reset = sdhc_stm32_reset,
};

static int sdhc_stm32_init(const struct device *dev)
{
	int ret;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	if (config->sdhi_on_gpio.port) {
		if (sdhi_power_on(dev)) {
			LOG_ERR("Failed to power card on.");
			return -ENODEV;
		}
	}

	ret = sdhc_stm32_activate(dev);
	if (ret != 0) {
		LOG_ERR("Clock and GPIO could not be initialized for the SDHC module, err=%d", ret);
		return ret;
	}

	ret = sdhc_stm32_sd_init(dev);
	if (ret != 0) {
		LOG_ERR("SDIO Init Failed\n");
		sdhc_stm32_log_err_type(config->hsd);
		return ret;
	}

	LOG_INF("SDIO Init Passed Successfully.\n");

	sdhc_stm32_init_props(dev);

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
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed to disable SDHC clock during PM suspend process.");
		return ret;
	}

	/* Move pins to sleep state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
	if (ret == -ENOENT) {
		/* Warn but don't block suspend */
		LOG_WRN("SDHC pinctrl sleep state not available");
	}

	return ret;
}

static int sdhc_stm32_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = sdhc_stm32_activate(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = sdhc_stm32_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}
#endif

#define SDHC_STM32_INIT(index)                                                                     \
	static SDIO_HandleTypeDef hsd_##index = {                                                  \
		.Instance = (MMC_TypeDef *)DT_INST_REG_ADDR(index),                                \
	};                                                                                         \
	static struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static const struct sdhc_stm32_config sdhc_stm32_cfg_##index = {                           \
		.hsd = &hsd_##index,                                                               \
		.pclken = pclken_##index,                                                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.hw_flow_control = DT_INST_PROP_OR(index, hw_flow_control, 0),                     \
		.clk_div = DT_INST_PROP(index, clk_div),                                           \
		.bus_width = DT_INST_PROP(index, bus_width),                                       \
		.power_delay_ms = DT_INST_PROP_OR(index, power_delay_ms, 500),                     \
		.support_1_8_v = DT_INST_PROP_OR(index, support_1_8_v, 0),                         \
		.sdhi_on_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), sdhi_on_gpios, {0}),       \
		.cd_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), cd_gpios, {0}),                 \
		.min_freq = DT_INST_PROP(index, min_bus_freq),                                     \
		.max_freq = DT_INST_PROP(index, max_bus_freq),                                     \
	};                                                                                         \
	static struct sdhc_stm32_data sdhc_stm32_data_##index;                                     \
	PM_DEVICE_DT_INST_DEFINE(index, sdhc_stm32_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(index, &sdhc_stm32_init, NULL, &sdhc_stm32_data_##index,             \
			      &sdhc_stm32_cfg_##index, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,     \
			      &sdhc_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_STM32_INIT)
