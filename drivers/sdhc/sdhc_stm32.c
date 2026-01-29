/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdhc

#include "sdhc_stm32_ll.h"
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(sdhc_stm32, CONFIG_SDHC_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

BUILD_ASSERT((CONFIG_SDHC_BUFFER_ALIGNMENT % sizeof(uint32_t)) == 0U);

#define SDIO_OCR_SDIO_S18R BIT(24) /* SDIO OCR bit indicating support for 1.8V switching */

struct sdhc_stm32_config {
	bool hw_flow_control;              /* flag for enabling hardware flow control */
	bool support_1_8_v;                /* flag indicating support for 1.8V signaling */
	unsigned int max_freq;             /* Max bus frequency in Hz */
	unsigned int min_freq;             /* Min bus frequency in Hz */
	uint8_t bus_width;                 /* Width of the SDMMC bus */
	uint16_t clk_div;                  /* Clock divider value to configure SDMMC clock speed */
	uint32_t power_delay_ms;           /* power delay prop for the host in milliseconds */
	SDMMC_TypeDef *Instance;           /* Base address of the SDMMC peripheral */
	const struct stm32_pclken *pclken; /* Pointer to peripheral clock configuration */
	const struct pinctrl_dev_config *pcfg; /* Pointer to pin control configuration */
	struct gpio_dt_spec sdhi_on_gpio;  /* Power pin to control the regulators used by card.*/
	struct gpio_dt_spec cd_gpio;       /* Card detect GPIO pin */
	irq_config_func_t irq_config_func; /* IRQ config function */
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
 * @brief Log SDMMC-related error conditions.
 *
 * This helper function queries the error status of an SDIO operation
 * and reports specific error types using `LOG_ERR()`.
 * In addition to logging, it also resets the `error_code` field
 * of the `SDIO_HandleTypeDef` to `HAL_SDIO_ERROR_NONE`.
 *
 * @param dev_data Pointer to the STM32 SDHC data structure.
 */
static void sdhc_stm32_log_err_type(struct sdhc_stm32_data *dev_data)
{
	uint32_t error_code = dev_data->error_code;

	if (error_code == SDMMC_ERROR_NONE) {
		return;
	}

	static const struct {
		uint32_t mask;
		const char *msg;
	} sdmmc_errors[] = {
		{SDMMC_ERROR_TX_UNDERRUN, "Transmit FIFO underrun during write"},
		{SDMMC_ERROR_RX_OVERRUN, "Receive FIFO overrun during read"},
		{SDMMC_ERROR_INVALID_PARAMETER, "Invalid parameter passed to SD/SDIO operation"},
		{SDMMC_ERROR_ILLEGAL_CMD, "Command is not legal for the card state"},
		{SDMMC_ERROR_BUSY, "SDHC interface is busy"},
		{SDMMC_ERROR_INVALID_VOLTRANGE, "Unsupported voltage range requested"},
		{SDMMC_ERROR_UNSUPPORTED_FEATURE, "Requested card feature is not supported"},
		{SDMMC_ERROR_DMA, "DMA transfer error occurred"},
		{SDMMC_ERROR_CID_CSD_OVERWRITE, "CID/CSD register overwrite attempted"},

		{SDMMC_ERROR_GENERAL_UNKNOWN_ERR | SDMMC_ERROR_REQUEST_NOT_APPLICABLE,
		 "General SDHC error or invalid operation"},

		{SDMMC_ERROR_TIMEOUT | SDMMC_ERROR_CMD_RSP_TIMEOUT | SDMMC_ERROR_DATA_TIMEOUT,
		 "Timeout occurred (command or data response)"},

		{SDMMC_ERROR_CMD_CRC_FAIL | SDMMC_ERROR_DATA_CRC_FAIL | SDMMC_ERROR_COM_CRC_FAILED,
		 "CRC failure detected (command, data, or communication)"},

		{SDMMC_ERROR_ADDR_MISALIGNED | SDMMC_ERROR_ADDR_OUT_OF_RANGE,
		 "Addressing error: misaligned or out-of-range access"},

		{SDMMC_ERROR_WRITE_PROT_VIOLATION | SDMMC_ERROR_LOCK_UNLOCK_FAILED,
		 "Access violation: write-protect or lock/unlock failure"},

		{SDMMC_ERROR_ERASE_RESET | SDMMC_ERROR_AKE_SEQ_ERR,
		 "Card error: erase reset or authentication sequence failure"},

		{SDMMC_ERROR_BLOCK_LEN_ERR | SDMMC_ERROR_ERASE_SEQ_ERR |
			 SDMMC_ERROR_BAD_ERASE_PARAM | SDMMC_ERROR_WP_ERASE_SKIP,
		 "Block or erase sequence error"},

	};

	for (size_t i = 0; i < ARRAY_SIZE(sdmmc_errors); i++) {
		if (error_code & sdmmc_errors[i].mask) {
			LOG_ERR("SDHC Error: %s", sdmmc_errors[i].msg);
			dev_data->error_code = SDMMC_ERROR_NONE;
			return;
		}
	}

	LOG_ERR("Unknown SDMMC Error: 0x%08x", error_code);
	dev_data->error_code = SDMMC_ERROR_NONE;
}

/**
 * Initializes the SDHC peripheral with the configuration specified.
 *
 * This includes deinitializing any previous configuration, and applying
 * parameters like clock edge, power saving, clock divider, hardware
 * flow control and bus width.
 *
 * @param dev Pointer to the device structure for the SDHC peripheral.
 *
 * @return 0 on success, err code on failure.
 */
static int sdhc_stm32_sd_init(const struct device *dev)
{
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	uint32_t sdmmc_clk_rate;

	data->host_io.bus_width = config->bus_width;

	if (sdhc_stm32_ll_deinit(config->Instance, data) != 0) {
		LOG_ERR("Failed to de-initialize the SDHC device");
		return -EIO;
	}

	data->Init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
	data->Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	data->Init.ClockDiv = config->clk_div;

	/*
	 * Get SDMMC kernel clock rate for clock divider calculations.
	 * Use pclken[1] (kernel clock) if available, otherwise fall back to pclken[0] (bus clock).
	 */
	if (clock_control_get_rate(clk,
				   (clock_control_subsys_t)(uintptr_t)&config
					   ->pclken[(DT_INST_NUM_CLOCKS(0) > 1) ? 1 : 0],
				   &sdmmc_clk_rate) < 0) {
		LOG_ERR("Failed to get SDMMC clock rate");
		return -EIO;
	}
	data->sdmmc_clk = sdmmc_clk_rate;

	if (config->hw_flow_control) {
		data->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	} else {
		data->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	}

	if (data->host_io.bus_width == SDHC_BUS_WIDTH4BIT) {
		data->Init.BusWide = SDMMC_BUS_WIDE_4B;
	} else if (data->host_io.bus_width == SDHC_BUS_WIDTH8BIT) {
		data->Init.BusWide = SDMMC_BUS_WIDE_8B;
	} else {
		data->Init.BusWide = SDMMC_BUS_WIDE_1B;
	}

	if (sdhc_stm32_ll_init(config->Instance, data) != 0) {
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
					    (clock_control_subsys_t)(uintptr_t)&config->pclken[1],
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

	return SDMMC_CmdGoIdleState(config->Instance);
}

static int sdhc_stm32_rw_direct(const struct device *dev, struct sdhc_command *cmd)
{
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_stm32_data *dev_data = dev->data;

	return sdhc_stm32_ll_sdmmc_rw_direct(config->Instance, cmd->arg, cmd->response, dev_data);
}

static int sdhc_stm32_rw_extended(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	int res;
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

	sdhc_stm32_sdio_ext_cmd_t arg = {.Reg_Addr = reg_addr,
					 .IOFunctionNbr = func,
					 .Block_Mode = is_block_mode ? SDMMC_SDIO_MODE_BLOCK
								     : SDMMC_SDIO_MODE_BYTE,
					 .OpCode = increment};
	dev_data->block_size = is_block_mode ? data->block_size : 0;
	dev_data->total_transfer_bytes = data->blocks * data->block_size;

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		dev_data->sdio_dma_buf = k_aligned_alloc(CONFIG_SDHC_BUFFER_ALIGNMENT,
							 data->blocks * data->block_size);
		if (dev_data->sdio_dma_buf == NULL) {
			LOG_ERR("DMA buffer allocation failed");
			return -ENOMEM;
		}
	}

	if (direction == SDIO_IO_WRITE) {
		if (IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
			res = sdhc_stm32_ll_sdio_write_extended(config->Instance, &arg, data->data,
								dev_data->total_transfer_bytes,
								data->timeout_ms, dev_data);
		} else {
			memcpy(dev_data->sdio_dma_buf, data->data, dev_data->total_transfer_bytes);
			sys_cache_data_flush_range(dev_data->sdio_dma_buf,
						   dev_data->total_transfer_bytes);
			res = sdhc_stm32_ll_sdio_write_extended_dma(config->Instance, &arg,
								    dev_data);
		}
	} else {
		if (IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
			res = sdhc_stm32_ll_sdio_read_extended(config->Instance, &arg, data->data,
							       dev_data->total_transfer_bytes,
							       data->timeout_ms, dev_data);
		} else {
			sys_cache_data_flush_range(dev_data->sdio_dma_buf,
						   dev_data->total_transfer_bytes);
			res = sdhc_stm32_ll_sdio_read_extended_dma(config->Instance, &arg,
								   dev_data);
		}
	}

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		/* Only wait on semaphore if HAL function succeeded */
		if (res != 0) {
			k_free(dev_data->sdio_dma_buf);
			return res;
		}

		/* Wait for whole transfer to complete */
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(data->timeout_ms)) != 0) {
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

	return res;
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

	/* Start switching procedue */
	config->Instance->POWER |= SDMMC_POWER_VSWITCHEN;

	res = SDMMC_CmdVoltageSwitch(config->Instance);
	if (res != 0) {
		LOG_ERR("CMD11 failed: %#x", res);
		return -EIO;
	}

	LOG_DBG("Successfully switched to 1.8V signaling");
	return 0;
}

static uint32_t sdhc_stm32_send_relative_addr(const struct sdhc_stm32_config *config,
					      struct sdhc_command *cmd,
					      struct sdhc_stm32_data *dev_data)
{
	uint32_t sdmmc_res = SDMMC_CmdSetRelAdd(config->Instance, (uint16_t *)&cmd->response);

	if (sdmmc_res != 0U) {
		return sdmmc_res;
	}
	/*
	 * Restore RCA by reversing the double 16-bit right shift from
	 * Zephyr subsys and SDMMC_CmdSetRelAdd
	 */
	cmd->response[0] = cmd->response[0] << 16;

	return sdmmc_res;
}

static uint32_t sdhc_stm32_select_card(const struct sdhc_stm32_config *config,
				       struct sdhc_command *cmd)
{
	uint32_t sdmmc_res = SDMMC_CmdSelDesel(config->Instance, cmd->arg);

	if (sdmmc_res != 0U) {
		return sdmmc_res;
	}

	cmd->response[0] = SDMMC_GetResponse(config->Instance, SDMMC_RESP1);

	return sdmmc_res;
}

static int sdhc_stm32_write_blocks(const struct device *dev, struct sdhc_data *data)
{
	int ret;
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_stm32_data *dev_data = dev->data;

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		sys_cache_data_flush_range(data->data, data->blocks * data->block_size);

		ret = sdhc_stm32_ll_write_blocks_dma(config->Instance, data->data, data->block_addr,
						     data->blocks, dev_data);
	} else {
		ret = sdhc_stm32_ll_write_blocks(config->Instance, data->data, data->block_addr,
						 data->blocks, data->timeout_ms, dev_data);
	}

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(data->timeout_ms)) != 0) {
			LOG_ERR("Failed to acquire Semaphore");
			return -ETIMEDOUT;
		}
	}

	return ret;
}

/**
 * @brief Read blocks from SD card
 * This function handles both DMA and polling modes based on configuration
 *
 * @param dev SDHC device instance
 * @param data Data descriptor containing buffer, address, and block count
 * @return int 0 on success, negative errno on failure
 *
 */
static int sdhc_stm32_read_blocks(const struct device *dev, struct sdhc_data *data)
{
	int ret;
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_stm32_data *dev_data = dev->data;

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		sys_cache_data_flush_range(data->data, data->blocks * data->block_size);

		ret = sdhc_stm32_ll_read_blocks_dma(config->Instance, data->data, data->block_addr,
						    data->blocks, dev_data);
	} else {
		ret = sdhc_stm32_ll_read_blocks(config->Instance, data->data, data->block_addr,
						data->blocks, data->timeout_ms, dev_data);
	}

	if (!IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE)) {
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(data->timeout_ms)) != 0) {
			LOG_ERR("Failed to acquire Semaphore");
			return -ETIMEDOUT;
		}
		sys_cache_data_invd_range(data->data, data->blocks * data->block_size);
	}

	return ret;
}

static int sdhc_stm32_send_csd_and_save_card_configs(const struct sdhc_stm32_config *config,
						     struct sdhc_command *cmd,
						     struct sdhc_stm32_data *dev_data)
{
	int res = 0;

	res = SDMMC_CmdSendCSD(config->Instance, cmd->arg);
	if (res == 0U) {
		cmd->response[1] = SDMMC_GetResponse(config->Instance, SDMMC_RESP2);
		cmd->response[2] = SDMMC_GetResponse(config->Instance, SDMMC_RESP3);
		dev_data->card_class = (SDMMC_GetResponse(config->Instance, SDMMC_RESP2) >> 20U);
	}

	return res;
}

static uint32_t sdhc_stm32_send_op_cond(const struct sdhc_stm32_config *config,
					struct sdhc_command *cmd, struct sdhc_stm32_data *dev_data)
{
	uint32_t res;

	res = SDMMC_CmdAppOperCommand(config->Instance, cmd->arg);
	if (res == 0U) {
		cmd->response[0] = SDMMC_GetResponse(config->Instance, SDMMC_RESP1);
	}

	return res;
}

static uint32_t sdhc_stm32_send_cid(const struct sdhc_stm32_config *config,
				    struct sdhc_command *cmd)
{
	uint32_t res;

	res = SDMMC_CmdSendCID(config->Instance);
	if (res == 0U) {
		cmd->response[0] = SDMMC_GetResponse(config->Instance, SDMMC_RESP1);
		cmd->response[1] = SDMMC_GetResponse(config->Instance, SDMMC_RESP2);
		cmd->response[2] = SDMMC_GetResponse(config->Instance, SDMMC_RESP3);
		cmd->response[3] = SDMMC_GetResponse(config->Instance, SDMMC_RESP4);
	}

	return res;
}

static bool sdhc_stm32_is_read_write_opcode(struct sdhc_command *cmd)
{
	return ((cmd->opcode == SD_READ_SINGLE_BLOCK) || (cmd->opcode == SD_READ_MULTIPLE_BLOCK) ||
		(cmd->opcode == SD_WRITE_SINGLE_BLOCK) ||
		(cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) || (cmd->opcode == SDIO_RW_EXTENDED));
}

static int sdhc_stm32_card_busy(const struct device *dev)
{
	struct sdhc_stm32_data *dev_data = dev->data;

	/* Card is busy if mutex is locked */
	if (k_mutex_lock(&dev_data->bus_mutex, K_NO_WAIT) == 0) {
		/* Mutex was available, unlock and return not busy */
		k_mutex_unlock(&dev_data->bus_mutex);
		return 0;
	}

	/* Mutex was locked, card is busy */
	return 1;
}

/**
 * @brief Send command to SD/MMC card
 *
 * @param dev SDHC device instance
 * @param cmd Command structure containing opcode, argument, and response buffer
 * @param data Optional data transfer descriptor (for read/write commands)
 *
 * @return 0 on success, non-zero code on failure.
 */
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

	if (sdhc_stm32_card_busy(dev)) {
		LOG_ERR("Card is busy");
		k_mutex_unlock(&dev_data->bus_mutex);
		return -ETIMEDOUT;
	}

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		sdmmc_res = sdhc_stm32_go_idle_state(dev);
		break;

	case SD_SEND_IF_COND:
		sdmmc_res = SDMMC_CmdOperCond(config->Instance);
		if (sdmmc_res == 0U) {
			cmd->response[0] = SDMMC_GetResponse(config->Instance, SDMMC_RESP1);
		}
		break;

	case SD_SEND_CSD:
		res = sdhc_stm32_send_csd_and_save_card_configs(config, cmd, dev_data);
		break;

	case SD_ERASE_BLOCK_START:
		res = sdhc_stm32_ll_erase_block_start(dev_data, config->Instance, cmd->arg);
		break;

	case SD_ERASE_BLOCK_END:
		res = sdhc_stm32_ll_erase_block_end(dev_data, config->Instance, cmd->arg);
		break;

	case SD_ERASE_BLOCK_OPERATION:
		res = sdhc_stm32_ll_erase(dev_data, config->Instance, cmd->arg);
		break;

	case SD_SWITCH:
		__ASSERT_NO_MSG(data != NULL);
		sdmmc_res = sdhc_stm32_ll_switch_speed(config->Instance, cmd->arg, data->data,
						       data->block_size, dev_data);
		break;

	case SD_APP_CMD:
		sdmmc_res = SDMMC_CmdAppCommand(config->Instance, cmd->arg);
		if (sdmmc_res == 0U) {
			cmd->response[0] = SDMMC_GetResponse(config->Instance, SDMMC_RESP1);
		}
		break;

	case SD_APP_SEND_OP_COND:
		sdmmc_res = sdhc_stm32_send_op_cond(config, cmd, dev_data);
		break;

	case SD_ALL_SEND_CID:
		sdmmc_res = sdhc_stm32_send_cid(config, cmd);
		break;

	case SD_SELECT_CARD:
		sdmmc_res = sdhc_stm32_select_card(config, cmd);
		break;

	case SD_SEND_RELATIVE_ADDR:
		sdmmc_res = sdhc_stm32_send_relative_addr(config, cmd, dev_data);
		break;

	case SDIO_SEND_OP_COND:
		sdmmc_res = SDMMC_CmdSendOperationcondition(config->Instance, cmd->arg,
							    (uint32_t *)&cmd->response);
		break;

	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_write_blocks(dev, data);
		break;

	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_read_blocks(dev, data);
		break;

	case SDIO_RW_DIRECT:
		res = sdhc_stm32_rw_direct(dev, cmd);
		break;

	case SDIO_RW_EXTENDED:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_rw_extended(dev, cmd, data);
		break;

	case SD_APP_SEND_SCR:
		res = sdhc_stm32_ll_find_scr(config->Instance, dev_data, data->data,
					     data->block_size);
		break;

	case SD_SET_BLOCK_SIZE:
		sdmmc_res = SDMMC_CmdBlockLength(config->Instance, (uint32_t)cmd->arg);
		break;

	case SD_VOL_SWITCH:
		res = sdhc_stm32_switch_to_1_8v(dev);
		break;

	case SD_SEND_STATUS:
		sdmmc_res = sdhc_stm32_ll_send_status(config->Instance, dev_data, cmd->arg,
						      &cmd->response[0]);
		break;

	default:
		LOG_DBG("Unsupported Command, opcode:%d", cmd->opcode);
		res = -ENOTSUP;
	}

	if ((sdmmc_res != 0U) || (res != 0)) {
		LOG_DBG("Command Failed, opcode:%d", cmd->opcode);
		sdhc_stm32_log_err_type(dev_data);

		if (sdmmc_res != 0U) {
			res = -EIO;
		}
	}

	/*
	 * Defer PM release to ISR only for successful DMA-based read/write commands.
	 * Release PM here for all other cases (polling mode, non-read/write opcodes, errors).
	 */
	if (IS_ENABLED(CONFIG_SDHC_STM32_POLLING_MODE) || !sdhc_stm32_is_read_write_opcode(cmd) ||
	    res != 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
	}

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
			LOG_ERR("Invalid clock frequency, domain (%u, %u)", props->f_min,
				props->f_max);
			res = -EINVAL;
			goto end;
		}
		if (sdhc_stm32_ll_config_freq(config->Instance, (uint32_t)ios->clock, data) != 0) {
			LOG_ERR("Failed to set clock to %d", ios->clock);
			res = -EIO;
			goto end;
		}
		host_io->clock = ios->clock;
		LOG_DBG("Clock set to %d", ios->clock);
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		(void)SDMMC_PowerState_OFF(config->Instance);
		k_msleep(data->props.power_delay);
	} else {
		(void)SDMMC_PowerState_ON(config->Instance);
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

		stm32_reg_modify_bits(&config->Instance->CLKCR, SDMMC_CLKCR_WIDBUS,
				      bus_width_reg_value);
		host_io->bus_width = ios->bus_width;
	}

end:
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
	const struct sdhc_stm32_config *config = dev->config;

	/* If a CD pin is configured, use it for card detection */
	if (config->cd_gpio.port != NULL) {
		res = gpio_pin_get_dt(&config->cd_gpio);
		return res;
	}

	/* No CD pin configured, assume card is in slot */
	return 1;
}

static int sdhc_stm32_reset(const struct device *dev)
{
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Resetting Host controller */
	(void)SDMMC_PowerState_OFF(config->Instance);
	k_msleep(data->props.power_delay);
	(void)SDMMC_PowerState_ON(config->Instance);
	k_msleep(data->props.power_delay);

	/* Clear error flags */
	__SDMMC_CLEAR_FLAG(config->Instance, SDMMC_STATIC_FLAGS);
	data->error_code = SDMMC_ERROR_NONE;

	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return 0;
}

static void sdhc_stm32_clear_icr_flags(SDMMC_TypeDef *Instance)
{
	uint32_t icr_clear_flag = 0;
	uint32_t sta = Instance->STA;

	if ((sta & SDMMC_STA_DCRCFAIL) != 0U) {
		icr_clear_flag |= SDMMC_ICR_DCRCFAILC;
	}

	if ((sta & SDMMC_STA_DTIMEOUT) != 0U) {
		icr_clear_flag |= SDMMC_ICR_DTIMEOUTC;
	}

	if ((sta & SDMMC_STA_TXUNDERR) != 0U) {
		icr_clear_flag |= SDMMC_ICR_TXUNDERRC;
	}

	if ((sta & SDMMC_STA_RXOVERR) != 0U) {
		icr_clear_flag |= SDMMC_ICR_RXOVERRC;
	}

	if (icr_clear_flag != 0U) {
		LOG_ERR("SDMMC interrupt err flag raised: 0x%08X", icr_clear_flag);
		Instance->ICR = icr_clear_flag;
	}
}

void sdhc_stm32_event_isr(const struct device *dev)
{
	const struct sdhc_stm32_config *config = dev->config;
	struct sdhc_stm32_data *data = dev->data;

	if (__SDMMC_GET_FLAG(config->Instance, SDMMC_FLAG_DATAEND | SDMMC_FLAG_DCRCFAIL |
						       SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_RXOVERR |
						       SDMMC_FLAG_TXUNDERR)) {
		k_sem_give(&data->device_sync_sem);
	}

	sdhc_stm32_clear_icr_flags(config->Instance);

	if (data->error_code != 0) {
		LOG_ERR("Error Interrupt");
		sdhc_stm32_log_err_type(data);
	}

	sdhc_stm32_ll_irq_handler(config->Instance, data);

	/* Release PM locks when transfer finishes (successfully or with error) */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);
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
		LOG_ERR("SDHC init failed");
		sdhc_stm32_log_err_type(data);
		return ret;
	}

	LOG_INF("SDHC Init Passed Successfully");

	sdhc_stm32_init_props(dev);

	config->irq_config_func();
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	k_mutex_init(&data->bus_mutex);

	return ret;
}

static DEVICE_API(sdhc, sdhc_stm32_api) = {
	.request = sdhc_stm32_request,
	.set_io = sdhc_stm32_set_io,
	.get_host_props = sdhc_stm32_get_host_props,
	.get_card_present = sdhc_stm32_get_card_present,
	.card_busy = sdhc_stm32_card_busy,
	.reset = sdhc_stm32_reset,
};

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

#define STM32_SDHC_IRQ_HANDLER(index)                                                              \
	static void sdhc_stm32_irq_config_func_##index(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, event, irq),                                \
			    DT_INST_IRQ_BY_NAME(index, event, priority), sdhc_stm32_event_isr,     \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQ_BY_NAME(index, event, irq));                                \
	}

#define SDHC_STM32_INIT(index)                                                                     \
                                                                                                   \
	STM32_SDHC_IRQ_HANDLER(index)                                                              \
                                                                                                   \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct sdhc_stm32_config sdhc_stm32_cfg_##index = {                           \
		.Instance = (SDMMC_TypeDef *)DT_INST_REG_ADDR(index),                              \
		.irq_config_func = sdhc_stm32_irq_config_func_##index,                             \
		.pclken = pclken_##index,                                                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.hw_flow_control = DT_INST_PROP(index, hw_flow_control),                           \
		.clk_div = DT_INST_PROP(index, clk_div),                                           \
		.bus_width = DT_INST_PROP(index, bus_width),                                       \
		.power_delay_ms = DT_INST_PROP(index, power_delay_ms),                             \
		.support_1_8_v = DT_INST_PROP(index, support_1_8_v),                               \
		.sdhi_on_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), sdhi_on_gpios, {0}),       \
		.cd_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), cd_gpios, {0}),                 \
		.min_freq = DT_INST_PROP(index, min_bus_freq),                                     \
		.max_freq = DT_INST_PROP(index, max_bus_freq),                                     \
	};                                                                                         \
                                                                                                   \
	static struct sdhc_stm32_data sdhc_stm32_data_##index;                                     \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(index, sdhc_stm32_pm_action);                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &sdhc_stm32_init, PM_DEVICE_DT_INST_GET(index),               \
			      &sdhc_stm32_data_##index, &sdhc_stm32_cfg_##index, POST_KERNEL,      \
			      CONFIG_SDHC_INIT_PRIORITY, &sdhc_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_STM32_INIT)
