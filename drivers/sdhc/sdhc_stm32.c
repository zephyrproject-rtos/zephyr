/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdmmc

#include <stm32_bitops.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>

LOG_MODULE_REGISTER(sdhc_stm32, CONFIG_SDHC_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

BUILD_ASSERT((CONFIG_SDHC_BUFFER_ALIGNMENT % sizeof(uint32_t)) == 0U);

#define SDIO_OCR_SDIO_S18R BIT(24) /* SDIO OCR bit indicating support for 1.8V switching */

/* Set IDMA buffer address: adapts to series with IDMABASER vs IDMABASE0 */
#if defined(SDMMC_IDMABASER_IDMABASER)
#define SDHC_STM32_IDMA_SET_BUFFER(instance, addr) ((instance)->IDMABASER = (uint32_t)(addr))
#else
#define SDHC_STM32_IDMA_SET_BUFFER(instance, addr) ((instance)->IDMABASE0 = (uint32_t)(addr))
#endif

/* SDMMC data error flags */
#define SDMMC_DATA_ERROR_FLAGS                                                                     \
	(SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR)

/* SDMMC wait flags for RX operations */
#define SDMMC_WAIT_RX_FLAGS                                                                        \
	(SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)

/* SDMMC wait flags for TX operations */
#define SDMMC_WAIT_TX_FLAGS                                                                        \
	(SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)

/* SDMMC wait flags for RX with DBCKEND */
#define SDMMC_WAIT_RX_DBCKEND_FLAGS                                                                \
	(SDMMC_WAIT_RX_FLAGS | SDMMC_FLAG_DBCKEND)

struct sdhc_stm32_config {
	DEVICE_MMIO_ROM;
	bool hw_flow_control;              /* flag for enabling hardware flow control */
	bool support_1_8_v;                /* flag indicating support for 1.8V signaling */
	unsigned int max_freq;             /* Max bus frequency in Hz */
	unsigned int min_freq;             /* Min bus frequency in Hz */
	uint8_t bus_width;                 /* Width of the SDMMC bus */
	uint16_t clk_div;                  /* Clock divider value to configure SDMMC clock speed */
	uint32_t power_delay_ms;           /* power delay prop for the host in milliseconds */
	struct stm32_pclken *pclken;       /* Pointer to peripheral clock configuration */
	const struct pinctrl_dev_config *pcfg; /* Pointer to pin control configuration */
	struct gpio_dt_spec sdhi_on_gpio;  /* Power pin to control the regulators used by card.*/
	struct gpio_dt_spec cd_gpio;       /* Card detect GPIO pin */
	irq_config_func_t irq_config_func; /* IRQ config function */
};

struct sdhc_stm32_data {
	DEVICE_MMIO_RAM;
	struct k_mutex bus_mutex;      /* Sync between commands */
	struct sdhc_io host_io;        /* Input/Output host configuration */
	struct sdhc_host_props props;  /* current host properties */
	struct k_sem device_sync_sem;  /* Sync between device communication messages */
	void *sdio_dma_buf;            /* DMA buffer for SDIO/SDMMC data transfer */
	uint32_t total_transfer_bytes; /* number of bytes transferred */
	uint32_t sdmmc_clk;            /* Specifies the clock*/
	uint32_t block_size;           /* Block size for SDMMC data transfer */
	uint32_t error_code;           /* SD Card Error codes */
	bool is_sdio_transfer;         /* Flag to indicate if current transfer is SDIO */

	uint32_t rx_xfer_size; /* SD Rx Transfer size */
	uint32_t tx_xfer_size; /* SD Tx Transfer size */
	bool is_multi_block;   /* Flag for multi-block transfers */
};

static SDMMC_TypeDef *sdhc_stm32_get_instance(const struct device *dev)
{
	return (SDMMC_TypeDef *)DEVICE_MMIO_GET(dev);
}

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

#if CONFIG_SDHC_LOG_LEVEL >= LOG_LEVEL_ERR
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

	{SDMMC_ERROR_GENERAL_UNKNOWN_ERR, "General SDHC error"},
	{SDMMC_ERROR_REQUEST_NOT_APPLICABLE, "Request not applicable"},

	{SDMMC_ERROR_TIMEOUT, "Timeout occurred"},
	{SDMMC_ERROR_CMD_RSP_TIMEOUT, "Command response timeout"},
	{SDMMC_ERROR_DATA_TIMEOUT, "Data timeout"},

	{SDMMC_ERROR_CMD_CRC_FAIL, "Command CRC failure"},
	{SDMMC_ERROR_DATA_CRC_FAIL, "Data CRC failure"},
	{SDMMC_ERROR_COM_CRC_FAILED, "Communication CRC failure"},

	{SDMMC_ERROR_ADDR_MISALIGNED, "Address misaligned"},
	{SDMMC_ERROR_ADDR_OUT_OF_RANGE, "Address out of range"},

	{SDMMC_ERROR_WRITE_PROT_VIOLATION, "Write-protect violation"},
	{SDMMC_ERROR_LOCK_UNLOCK_FAILED, "Lock/unlock failure"},

	{SDMMC_ERROR_ERASE_RESET, "Erase reset"},
	{SDMMC_ERROR_AKE_SEQ_ERR, "Authentication sequence error"},

	{SDMMC_ERROR_BLOCK_LEN_ERR, "Block length error"},
	{SDMMC_ERROR_ERASE_SEQ_ERR, "Erase sequence error"},
	{SDMMC_ERROR_BAD_ERASE_PARAM, "Bad erase parameter"},
	{SDMMC_ERROR_WP_ERASE_SKIP, "Write-protected erase skipped"},
};
#endif /* CONFIG_SDHC_LOG_LEVEL >= LOG_LEVEL_ERR */

/**
 * Logs detailed SDIO error types using Zephyr's logging subsystem.
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
	uint32_t errors = dev_data->error_code;

	__ASSERT(errors != SDMMC_ERROR_NONE, "%s called with no error", __func__);

#if CONFIG_SDHC_LOG_LEVEL  >= LOG_LEVEL_ERR
	if (errors != 0U) {
		for (size_t i = 0; i < ARRAY_SIZE(sdmmc_errors); i++) {
			if ((errors & sdmmc_errors[i].mask) != 0U) {
				errors &= ~sdmmc_errors[i].mask;
				LOG_ERR("%s", sdmmc_errors[i].msg);
			}
		}

		if (errors != 0U) {
			LOG_ERR("Unknown error bit(s): 0x%08X", errors);
		}
	}
#endif /* CONFIG_SDHC_LOG_LEVEL  >= LOG_LEVEL_ERR */

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
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);
	uint32_t sdmmc_clk_rate;

	data->host_io.bus_width = config->bus_width;

	SDMMC_PowerState_OFF(instance);

	/* Get SDMMC kernel clock rate: use pclken[1] (kernel clock) if available,
	 * otherwise fall back to pclken[0] (bus clock).
	 */
	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_get_rate(clk,
					   (clock_control_subsys_t)&config->pclken[1],
					   &sdmmc_clk_rate) < 0) {
			LOG_ERR("Failed to get SDMMC clock rate");
			return -EIO;
		}
	} else {
		if (clock_control_get_rate(clk,
					   (clock_control_subsys_t)&config->pclken[0],
					   &sdmmc_clk_rate) < 0) {
			LOG_ERR("Failed to get SDMMC clock rate");
			return -EIO;
		}
	}
	data->sdmmc_clk = sdmmc_clk_rate;

	SDMMC_InitTypeDef init;

	init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
	init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	init.ClockDiv = config->clk_div;

	if (config->hw_flow_control) {
		init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	} else {
		init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	}

	if (data->host_io.bus_width == SDHC_BUS_WIDTH4BIT) {
		init.BusWide = SDMMC_BUS_WIDE_4B;
	} else if (data->host_io.bus_width == SDHC_BUS_WIDTH8BIT) {
		init.BusWide = SDMMC_BUS_WIDE_8B;
	} else {
		init.BusWide = SDMMC_BUS_WIDE_1B;
	}

	if (SDMMC_Init(instance, init) != HAL_OK) {
		return -EIO;
	}

	SDMMC_PowerState_ON(instance);

	if (config->clk_div != 0) {
		sdmmc_clk_rate = data->sdmmc_clk / (2U * config->clk_div);
	}

	data->error_code = 0;
	data->is_multi_block = false;

	return 0;
}

static int sdhc_stm32_activate(const struct device *dev)
{
	int ret;
	const struct sdhc_stm32_config *config = (struct sdhc_stm32_config *)dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk,
					    (clock_control_subsys_t)&config->pclken[1],
					    NULL) != 0) {
			LOG_ERR("Failed to enable SDHC domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]) != 0) {
		return -EIO;
	}

	return 0;
}

static void sdhc_stm32_disable_data_interrupts(SDMMC_TypeDef *instance)
{
	__SDMMC_DISABLE_IT(instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
					     SDMMC_IT_TXUNDERR | SDMMC_IT_RXOVERR |
					     SDMMC_IT_IDMABTC | SDMMC_IT_TXFIFOHE |
					     SDMMC_IT_RXFIFOHF);
}

static void sdhc_stm32_abort_dma_transfer(SDMMC_TypeDef *instance, struct sdhc_stm32_data *dev_data)
{
	sdhc_stm32_disable_data_interrupts(instance);
	__SDMMC_DISABLE_IT(instance, SDMMC_IT_IDMABTC);
	__SDMMC_CMDTRANS_DISABLE(instance);

	instance->DLEN = 0U;
	instance->IDMACTRL = SDMMC_DISABLE_IDMA;

	if (dev_data->is_sdio_transfer) {
		if ((instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
			instance->DCTRL = SDMMC_DCTRL_SDIOEN;
		} else {
			instance->DCTRL = 0U;
		}

		dev_data->is_sdio_transfer = false;
	} else {
		instance->DCTRL |= SDMMC_DCTRL_FIFORST;
		instance->CMD |= SDMMC_CMD_CMDSTOP;
		dev_data->error_code |= SDMMC_CmdStopTransfer(instance);
		instance->CMD &= ~(SDMMC_CMD_CMDSTOP);
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DABORT);
		instance->DCTRL = 0U;
		dev_data->is_multi_block = false;
	}

	__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_IDMABTC);
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);
}

static uint32_t sdhc_stm32_convert_block_size(struct sdhc_stm32_data *dev_data)
{
	uint32_t block_size = dev_data->block_size;
	struct sdhc_host_props *props = &dev_data->props;

	uint32_t max_block_len = SDMMC_DATABLOCK_SIZE_512B << props->host_caps.max_blk_len;
	uint32_t max_blk_size = max_block_len * BITS_PER_BYTE;

	__ASSERT(IS_POWER_OF_TWO(block_size) && block_size >= 1U && block_size <= max_blk_size,
		"Invalid block size: %u", block_size);
	/* SDMMC_DATABLOCK_SIZE_<n>B is log2(n) shifted to DBLOCKSIZE bit position */
	return LOG2(block_size) << SDMMC_DCTRL_DBLOCKSIZE_Pos;
}

static int sdhc_stm32_handle_data_error(SDMMC_TypeDef *instance, uint32_t errorcode,
					struct sdhc_stm32_data *data)
{
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
	data->error_code |= errorcode;
	if (errorcode == SDMMC_ERROR_DATA_TIMEOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

static uint32_t sdhc_stm32_map_resp_type(uint32_t resp_type)
{
	switch (resp_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_R2:
		return SDMMC_RESPONSE_LONG;
	case SD_RSP_TYPE_NONE:
		return SDMMC_RESPONSE_NO;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R5b:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
	default:
		return SDMMC_RESPONSE_SHORT;
	}
}

/**
 * @brief Send a command to the SDMMC peripheral and wait for response.
 *
 * @param dev_data  Pointer to the STM32 SDHC driver data structure
 * @param instance  SDMMC peripheral base address
 * @param cmd       Zephyr SDHC command structure (opcode, arg, response_type, timeout_ms)
 * @return 0 on success, negative errno on failure
 */
static int sdhc_stm32_send_cmd(struct sdhc_stm32_data *dev_data, SDMMC_TypeDef *instance,
			struct sdhc_command *cmd)
{
	uint32_t response_reg_type = sdhc_stm32_map_resp_type(cmd->response_type);
	uint32_t native_rsp_type = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;
	uint32_t sta = 0U;
	bool ret;

	SDMMC_CmdInitTypeDef cmdinit = {
		.Argument = cmd->arg,
		.CmdIndex = cmd->opcode,
		.Response = response_reg_type,
		.WaitForInterrupt = SDMMC_WAIT_NO,
		.CPSM = SDMMC_CPSM_ENABLE,
	};

	(void)SDMMC_SendCommand(instance, &cmdinit);

	if (response_reg_type == SDMMC_RESPONSE_NO) {
		/* No-response command (CMD0): wait for CMDSENT */
		ret = WAIT_FOR(instance->STA & SDMMC_FLAG_CMDSENT, cmd->timeout_ms * USEC_PER_MSEC,
			       k_yield());

		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_CMD_FLAGS);
		return ret ? 0 : -ETIMEDOUT;
	}

	/*
	 * Wait for response flags and CMDACT cleared.
	 * For R1b, include BUSYD0END to detect end of DAT0 busy signaling.
	 */
	uint32_t wait_flags = SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT | SDMMC_FLAG_CCRCFAIL;

	if (native_rsp_type == SD_RSP_TYPE_R1b) {
		wait_flags |= SDMMC_FLAG_BUSYD0END;
	}

	ret = WAIT_FOR(((sta = instance->STA) & wait_flags) && !(sta & SDMMC_FLAG_CMDACT),
		       cmd->timeout_ms * USEC_PER_MSEC, k_yield());

	if (!ret || (sta & SDMMC_FLAG_CTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_CMD_FLAGS);
		dev_data->error_code = SDMMC_ERROR_CMD_RSP_TIMEOUT;
		return -ETIMEDOUT;
	}

	if (sta & SDMMC_FLAG_CCRCFAIL) {
		/*
		 * R3 and R4 responses do not include a valid CRC
		 * field per SD specification. So, ignore CRC failure
		 * for these response types.
		 */
		if (native_rsp_type != SD_RSP_TYPE_R3 && native_rsp_type != SD_RSP_TYPE_R4) {
			__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_CCRCFAIL);
			return -EILSEQ;
		}
	}

	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_CMD_FLAGS);
	return 0;
}

static uint32_t handle_r1_errors(uint32_t resp)
{
	uint32_t err = 0;

	if ((resp & SDMMC_OCR_ERRORBITS) == 0U) {
		return 0;
	}

	struct {
		uint32_t flag;
		uint32_t error;
	} r1_errors[] = {
		{SDMMC_OCR_ADDR_OUT_OF_RANGE, SDMMC_ERROR_ADDR_OUT_OF_RANGE},
		{SDMMC_OCR_ADDR_MISALIGNED, SDMMC_ERROR_ADDR_MISALIGNED},
		{SDMMC_OCR_BLOCK_LEN_ERR, SDMMC_ERROR_BLOCK_LEN_ERR},
		{SDMMC_OCR_ERASE_SEQ_ERR, SDMMC_ERROR_ERASE_SEQ_ERR},
		{SDMMC_OCR_BAD_ERASE_PARAM, SDMMC_ERROR_BAD_ERASE_PARAM},
		{SDMMC_OCR_WRITE_PROT_VIOLATION, SDMMC_ERROR_WRITE_PROT_VIOLATION},
		{SDMMC_OCR_LOCK_UNLOCK_FAILED, SDMMC_ERROR_LOCK_UNLOCK_FAILED},
		{SDMMC_OCR_COM_CRC_FAILED, SDMMC_ERROR_COM_CRC_FAILED},
		{SDMMC_OCR_ILLEGAL_CMD, SDMMC_ERROR_ILLEGAL_CMD},
		{SDMMC_OCR_CARD_ECC_FAILED, SDMMC_ERROR_CARD_ECC_FAILED},
		{SDMMC_OCR_CC_ERROR, SDMMC_ERROR_CC_ERR},
		{SDMMC_OCR_STREAM_READ_UNDERRUN, SDMMC_ERROR_STREAM_READ_UNDERRUN},
		{SDMMC_OCR_STREAM_WRITE_OVERRUN, SDMMC_ERROR_STREAM_WRITE_OVERRUN},
		{SDMMC_OCR_CID_CSD_OVERWRITE, SDMMC_ERROR_CID_CSD_OVERWRITE},
		{SDMMC_OCR_WP_ERASE_SKIP, SDMMC_ERROR_WP_ERASE_SKIP},
		{SDMMC_OCR_CARD_ECC_DISABLED, SDMMC_ERROR_CARD_ECC_DISABLED},
		{SDMMC_OCR_ERASE_RESET, SDMMC_ERROR_ERASE_RESET},
		{SDMMC_OCR_AKE_SEQ_ERROR, SDMMC_ERROR_AKE_SEQ_ERR},
	};

	for (size_t i = 0; i < ARRAY_SIZE(r1_errors); i++) {
		if (resp & r1_errors[i].flag) {
			err |= r1_errors[i].error;
		}
	}

	if (err == 0) {
		err = SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
	return err;
}

static uint32_t handle_r5_errors(uint32_t resp)
{
	uint32_t err = 0;

	if ((resp & SDMMC_SDIO_R5_ERRORBITS) == 0U) {
		return 0;
	}

	if (resp & SDMMC_SDIO_R5_OUT_OF_RANGE) {
		err |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
	}
	if (resp & SDMMC_SDIO_R5_INVALID_FUNCTION_NUMBER) {
		err |= SDMMC_ERROR_INVALID_PARAMETER;
	}
	if (resp & SDMMC_SDIO_R5_ILLEGAL_CMD) {
		err |= SDMMC_ERROR_ILLEGAL_CMD;
	}
	if (resp & SDMMC_SDIO_R5_COM_CRC_FAILED) {
		err |= SDMMC_ERROR_COM_CRC_FAILED;
	}

	if (err == 0) {
		err = SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
	return err;
}

static uint32_t handle_r6_errors(uint32_t resp)
{
	uint32_t err = 0;

	if ((resp & SDMMC_SDIO_R5_ERRORBITS) == 0U) {
		return 0;
	}
	if (resp & SDMMC_R6_ILLEGAL_CMD) {
		err |= SDMMC_ERROR_ILLEGAL_CMD;
	}
	if (resp & SDMMC_R6_COM_CRC_FAILED) {
		err |= SDMMC_ERROR_COM_CRC_FAILED;
	}
	if (resp & SDMMC_R6_GENERAL_UNKNOWN_ERROR) {
		err |= SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}

	return err;
}

/**
 * @brief Validates SD/SDIO command response error flags
 *
 * This function inspects the first 32-bit response word returned by the card
 * and checks for error conditions according to the response type.
 *
 * @param dev_data Pointer to the STM32 SDHC driver private data structure.
 * @param cmd      Pointer to the SDHC command structure
 *
 * @retval 0        No error flags detected in the response.
 * @retval -EIO     An error flag was detected.
 *
 */
static int validate_response_err_flags(struct sdhc_stm32_data *dev_data, struct sdhc_command *cmd)
{
	uint32_t err = 0;
	uint32_t resp = cmd->response[0];

	switch (cmd->response_type) {

	case SD_RSP_TYPE_R1:
		err = handle_r1_errors(resp);
		break;

	case SD_RSP_TYPE_R5:
		err = handle_r5_errors(resp);
		break;

	case SD_RSP_TYPE_R6:
		err = handle_r6_errors(resp);
		break;

	default:
		return 0;
	}

	if (err != 0) {
		dev_data->error_code = err;
		return -EIO;
	}
	return 0;
}

/**
 * @brief Set response registers into cmd->response[] based on response type.
 *
 * @param dev_data  Pointer to the STM32 SDHC driver data structure
 * @param instance  SDMMC peripheral base address
 * @param cmd       Zephyr SDHC command structure
 */
static int sdhc_stm32_set_response(struct sdhc_stm32_data *dev_data, SDMMC_TypeDef *instance,
			    struct sdhc_command *cmd)
{
	uint32_t resp = sdhc_stm32_map_resp_type(cmd->response_type);

	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_CMD_FLAGS);
	switch (resp) {
	case SDMMC_RESPONSE_LONG:
		cmd->response[0] = SDMMC_GetResponse(instance, SDMMC_RESP1);
		cmd->response[1] = SDMMC_GetResponse(instance, SDMMC_RESP2);
		cmd->response[2] = SDMMC_GetResponse(instance, SDMMC_RESP3);
		cmd->response[3] = SDMMC_GetResponse(instance, SDMMC_RESP4);
		break;
	case SDMMC_RESPONSE_SHORT:
		cmd->response[0] = SDMMC_GetResponse(instance, SDMMC_RESP1);
		return validate_response_err_flags(dev_data, cmd);
	case SDMMC_RESPONSE_NO:
	default:
		break;
	}

	return 0;
}

static void sdhc_stm32_read_fifo_block(SDMMC_TypeDef *instance, uint8_t **p_buf)
{
	__ASSERT_NO_MSG(p_buf != NULL);
	__ASSERT_NO_MSG(*p_buf != NULL);

	uint32_t data;
	uint8_t *buf = *p_buf;

	for (uint32_t count = 0U; count < (SDMMC_HALFFIFOBYTES / sizeof(uint32_t)); count++) {
		data = SDMMC_ReadFIFO(instance);
		UNALIGNED_PUT(data, (uint32_t *)buf);
		buf += 4;
	}
	*p_buf = buf;
}

static void sdhc_stm32_write_fifo_block(SDMMC_TypeDef *instance, const uint8_t **p_buf)
{
	__ASSERT_NO_MSG(p_buf != NULL);
	__ASSERT_NO_MSG(*p_buf != NULL);

	uint32_t data;
	const uint8_t *buf = *p_buf;

	for (uint32_t count = 0U; count < (SDMMC_HALFFIFOBYTES / sizeof(uint32_t)); count++) {
		data = UNALIGNED_GET((const uint32_t *)buf);
		SDMMC_WriteFIFO(instance, &data);
		buf += 4;
	}
	*p_buf = buf;
}

static int sdhc_stm32_poll_read_transfer(SDMMC_TypeDef *instance, uint8_t **tempbuff,
					 uint32_t *dataremaining, uint32_t timeout,
					 struct sdhc_stm32_data *data)
{
	int ret;

	ret = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_RX_FLAGS), timeout * USEC_PER_MSEC, ({
				if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXFIFOHF) &&
				    (*dataremaining >= SDMMC_HALFFIFOBYTES)) {
					/* Read data from SDMMC Rx FIFO */
					sdhc_stm32_read_fifo_block(instance, tempbuff);
					*dataremaining -= SDMMC_HALFFIFOBYTES;
				}
				k_yield();
		       }));

	if (ret == 0) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}
	return 0;
}

static int sdhc_stm32_poll_write_transfer(SDMMC_TypeDef *instance, const uint8_t **tempbuff,
					  uint32_t *dataremaining, uint32_t timeout,
					  struct sdhc_stm32_data *data)
{
	int ret;

	ret = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_TX_FLAGS), timeout * USEC_PER_MSEC, ({
				if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_TXFIFOHE) &&
				    (*dataremaining >= SDMMC_HALFFIFOBYTES)) {
					/* Write data to SDMMC Tx FIFO */
					sdhc_stm32_write_fifo_block(instance, tempbuff);
					*dataremaining -= SDMMC_HALFFIFOBYTES;
				}
				k_yield();
		       }));

	if (ret == 0) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}
	return 0;
}

static int sdhc_stm32_send_stop_cmd(SDMMC_TypeDef *instance, uint32_t number_of_blocks,
				    struct sdhc_stm32_data *data)
{
	uint32_t errorstate;

	if (!__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DATAEND) || (number_of_blocks <= 1U)) {
		return 0;
	}

	/* Send stop transmission command */
	errorstate = SDMMC_CmdStopTransfer(instance);
	if (errorstate != SDMMC_ERROR_NONE) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		data->error_code |= errorstate;
		return -EIO;
	}

	return 0;
}

static int sdhc_stm32_rw_blocks_poll(struct sdhc_stm32_data *dev_data, struct sdhc_command *cmd,
				     SDMMC_TypeDef *instance, void *p_data,
				     uint32_t number_of_blocks, uint32_t timeout, bool is_write)
{
	int res;
	SDMMC_DataInitTypeDef config = {0};
	uint32_t errorstate;
	uint32_t dataremaining;

	if (p_data == NULL) {
		dev_data->error_code |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Initialize data control register */
	instance->DCTRL = 0U;

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = number_of_blocks * BLOCKSIZE;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
	config.TransferDir = is_write ? SDMMC_TRANSFER_DIR_TO_CARD : SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(instance, &config);
	__SDMMC_CMDTRANS_ENABLE(instance);

	/* Send read/write block command in polling mode */
	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res == 0) {
		res = sdhc_stm32_set_response(dev_data, instance, cmd);
	}
	if (res != 0) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		return -EIO;
	}

	/* Poll on SDMMC flags */
	dataremaining = config.DataLength;
	if (is_write) {
		const uint8_t *tempbuff = p_data;

		res = sdhc_stm32_poll_write_transfer(instance, &tempbuff, &dataremaining, timeout,
						     dev_data);
	} else {
		uint8_t *tempbuff = p_data;

		res = sdhc_stm32_poll_read_transfer(instance, &tempbuff, &dataremaining, timeout,
						    dev_data);
	}
	if (res != 0) {
		return res;
	}

	__SDMMC_CMDTRANS_DISABLE(instance);

	/* Send stop transmission command in case of multiblock transfer */
	res = sdhc_stm32_send_stop_cmd(instance, number_of_blocks, dev_data);
	if (res != 0) {
		return res;
	}

	/* Check for data transfer errors */
	errorstate = __SDMMC_GET_FLAG(instance, SDMMC_DATA_ERROR_FLAGS);
	if (errorstate != 0U) {
		return sdhc_stm32_handle_data_error(instance, errorstate, dev_data);
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

static int sdhc_stm32_rw_blocks_dma(struct sdhc_command *cmd, SDMMC_TypeDef *instance,
				    const uint8_t *p_data, uint32_t number_of_blocks,
				    struct sdhc_stm32_data *dev_data, bool is_write)
{
	int res;
	SDMMC_DataInitTypeDef config = {0};
	uint32_t data_error_it;

	if (p_data == NULL) {
		dev_data->error_code |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Initialize data control register */
	instance->DCTRL = 0U;

	if (is_write) {
		dev_data->tx_xfer_size = BLOCKSIZE * number_of_blocks;
		config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
		data_error_it = SDMMC_IT_TXUNDERR;
	} else {
		dev_data->rx_xfer_size = BLOCKSIZE * number_of_blocks;
		config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
		data_error_it = SDMMC_IT_RXOVERR;
	}

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = BLOCKSIZE * number_of_blocks;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(instance, &config);

	__SDMMC_CMDTRANS_ENABLE(instance);
	SDHC_STM32_IDMA_SET_BUFFER(instance, p_data);
	instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

	dev_data->is_multi_block = (number_of_blocks > 1U);

	/* Send read/write block command in DMA mode */
	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res == 0) {
		res = sdhc_stm32_set_response(dev_data, instance, cmd);
	}
	if (res != 0) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		dev_data->is_multi_block = false;
		return -EIO;
	}

	/* Enable transfer interrupts */
	__SDMMC_ENABLE_IT(instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | data_error_it |
				     SDMMC_IT_DATAEND));

	return 0;
}

static int sdhc_stm32_rw_blocks(struct sdhc_command *cmd, const struct device *dev,
				struct sdhc_data *data, bool is_write)
{
	int ret;
	struct sdhc_stm32_data *dev_data = dev->data;
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);

	if (IS_ENABLED(CONFIG_SDHC_STM32_DMA_MODE)) {
		sys_cache_data_flush_range(data->data, data->blocks * data->block_size);

		ret = sdhc_stm32_rw_blocks_dma(cmd, instance, data->data, data->blocks, dev_data,
					       is_write);
	} else {
		ret = sdhc_stm32_rw_blocks_poll(dev_data, cmd, instance, data->data, data->blocks,
						data->timeout_ms, is_write);
	}

	if (IS_ENABLED(CONFIG_SDHC_STM32_DMA_MODE) && ret == 0) {
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(data->timeout_ms)) != 0) {
			dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
			sdhc_stm32_abort_dma_transfer(instance, dev_data);
			LOG_ERR("Timed out waiting for transfer completion");
			return -ETIMEDOUT;
		}

		if (!is_write) {
			sys_cache_data_invd_range(data->data, data->blocks * data->block_size);
		}
	}

	return ret;
}

static void sdhc_stm32_config_sdio_dpsm(SDMMC_TypeDef *instance, bool is_block_mode,
					uint32_t size_byte, SDMMC_DataInitTypeDef *config,
					bool is_write, struct sdhc_stm32_data *dev_data)
{
	/* Initialize data control register */
	if ((instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		instance->DCTRL = 0U;
	}

	/* Configure SDIO Data Path State Machine (DPSM) */
	config->DataTimeOut = SDMMC_DATATIMEOUT;

	if (is_block_mode) {
		uint32_t blocks = (size_byte & ~(dev_data->block_size & 1U)) >>
				  (find_lsb_set(dev_data->block_size) - 1);

		config->DataLength = blocks * dev_data->block_size;
		config->DataBlockSize = sdhc_stm32_convert_block_size(dev_data);
	} else {
		config->DataLength = (size_byte > 0U) ? size_byte : SDMMC_DEFAULT_BLOCK_SIZE;
		config->DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config->TransferDir = is_write ? SDMMC_TRANSFER_DIR_TO_CARD : SDMMC_TRANSFER_DIR_TO_SDMMC;
	config->TransferMode = is_block_mode ? SDMMC_TRANSFER_MODE_BLOCK : SDMMC_TRANSFER_MODE_SDIO;
	config->DPSM = SDMMC_DPSM_DISABLE;
}

static void sdhc_stm32_read_remaining_bytes(SDMMC_TypeDef *instance, uint8_t **tempbuff,
					    uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byte_count;
	uint8_t *buf = *tempbuff;
	uint32_t remaining = *dataremaining;

	while ((remaining > 0U) && !(__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXFIFOE))) {
		data = SDMMC_ReadFIFO(instance);
		if (remaining >= 4U) {
			UNALIGNED_PUT(data, (uint32_t *)buf);
			buf += 4;
			remaining -= 4;
		} else {
			for (byte_count = 0U; byte_count < remaining; byte_count++) {
				*buf = (uint8_t)((data >> (byte_count * 8U)) & 0xFFU);
				buf++;
			}
			remaining = 0;
		}
	}

	*tempbuff = buf;
	*dataremaining = remaining;
}

static void sdhc_stm32_write_remaining_bytes(SDMMC_TypeDef *instance, uint8_t **u8buff,
					     uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byte_count;
	uint8_t *buf = *u8buff;
	uint32_t remaining = *dataremaining;

	while (remaining > 0U) {
		if (remaining >= 4U) {
			data = UNALIGNED_GET((uint32_t *)buf);
			buf += 4;
			remaining -= 4;
		} else {
			data = 0U;
			for (byte_count = 0U; byte_count < remaining; byte_count++) {
				data |= ((uint32_t)(*buf) << (byte_count * BITS_PER_BYTE));
				buf++;
			}
			remaining = 0;
		}
		instance->FIFO = data;
	}

	*u8buff = buf;
	*dataremaining = remaining;
}

static void sdhc_stm32_write_fifo_words(SDMMC_TypeDef *instance, uint32_t **u32tempbuff)
{
	uint32_t *buf = *u32tempbuff;

	for (uint32_t reg_count = 0U;
	     reg_count < (SDMMC_HALFFIFOBYTES / sizeof(uint32_t));
	     reg_count++) {
		instance->FIFO = *buf++;
	}
	*u32tempbuff = buf;
}

static int sdhc_stm32_find_scr(struct sdhc_command *cmd, SDMMC_TypeDef *instance,
			       struct sdhc_stm32_data *dev_data, uint32_t scr[2],
			       uint32_t block_size, uint32_t buf_size)
{
	SDMMC_DataInitTypeDef config = {0};
	uint32_t index = 0U;
	uint32_t tempscr[2U] = {0UL, 0UL};
	int ret;

	if (buf_size < 8U) {
		dev_data->error_code = SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = block_size;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_8B;
	config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_ENABLE;
	(void)SDMMC_ConfigData(instance, &config);

	ret = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (ret == 0) {
		ret = sdhc_stm32_set_response(dev_data, instance, cmd);
	}

	if (ret != 0) {
		return ret;
	}

	ret = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_RX_DBCKEND_FLAGS),
		       SDMMC_SWDATATIMEOUT * USEC_PER_MSEC, ({
				if ((!__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXFIFOE)) &&
				    (index == 0U)) {
					tempscr[0] = SDMMC_ReadFIFO(instance);
					tempscr[1] = SDMMC_ReadFIFO(instance);
					index++;
				}
				k_yield();
		       }));

	if (ret == 0) {
		dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}

	if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DTIMEOUT);
		dev_data->error_code |= SDMMC_ERROR_DATA_TIMEOUT;
		return -ETIMEDOUT;
	} else if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DCRCFAIL);
		dev_data->error_code |= SDMMC_ERROR_DATA_CRC_FAIL;
		return -EILSEQ;
	} else if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXOVERR)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_RXOVERR);
		dev_data->error_code |= SDMMC_ERROR_RX_OVERRUN;
		return -EIO;
	}
	/* No error flag set */
	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	scr[0] = sys_cpu_to_be32(tempscr[1]);
	scr[1] = sys_cpu_to_be32(tempscr[0]);

	return 0;
}

void sdhc_stm32_irq_handler(SDMMC_TypeDef *instance, struct sdhc_stm32_data *dev_data)
{
	uint32_t errorcode = __SDMMC_GET_FLAG(instance, SDMMC_DATA_ERROR_FLAGS);
	bool data_end = __SDMMC_GET_FLAG(instance, SDMMC_FLAG_DATAEND) != 0U;

	if ((errorcode == 0U) && !data_end) {
		/* Clear IDMA buffer transfer complete interrupt flag */
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_IDMABTC);
		return;
	}

	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	sdhc_stm32_disable_data_interrupts(instance);
	__SDMMC_DISABLE_IT(instance, SDMMC_IT_IDMABTC);

	__SDMMC_CMDTRANS_DISABLE(instance);

	instance->DLEN = 0;
	instance->IDMACTRL = SDMMC_DISABLE_IDMA;

	if (dev_data->is_sdio_transfer) {
		if ((instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
			instance->DCTRL = SDMMC_DCTRL_SDIOEN;
		} else {
			instance->DCTRL = 0U;
		}

		dev_data->is_sdio_transfer = false;
	} else {
		if (errorcode != 0U) {
			instance->DCTRL |= SDMMC_DCTRL_FIFORST;
			instance->CMD |= SDMMC_CMD_CMDSTOP;
			dev_data->error_code |= SDMMC_CmdStopTransfer(instance);
			instance->CMD &= ~(SDMMC_CMD_CMDSTOP);
			__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DABORT);
		} else {
			instance->DCTRL = 0U;

			if (dev_data->is_multi_block) {
				uint32_t stop_err = SDMMC_CmdStopTransfer(instance);

				if (stop_err != SDMMC_ERROR_NONE) {
					dev_data->error_code |= stop_err;
				}
			}
		}

		dev_data->is_multi_block = false;
	}

	if (errorcode != 0U) {
		dev_data->error_code |= errorcode;
		LOG_ERR("DMA transfer error: 0x%x", dev_data->error_code);
	} else {
		LOG_DBG("DMA transfer completed");
	}

	/* Clear IDMA buffer transfer complete interrupt flag */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_IDMABTC);
}

static int sdhc_stm32_sdio_rw_direct(struct sdhc_stm32_data *dev_data, SDMMC_TypeDef *instance,
			      struct sdhc_command *cmd)
{
	int res;

	dev_data->error_code = SDMMC_ERROR_NONE;

	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res == 0) {
		res = sdhc_stm32_set_response(dev_data, instance, cmd);
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	return res;
}

static int sdhc_stm32_sdio_rw_extended_poll(struct sdhc_command *cmd, SDMMC_TypeDef *instance,
					    bool is_block_mode, uint8_t *p_data,
					    uint32_t timeout_ms, struct sdhc_stm32_data *dev_data,
					    bool is_write)
{
	SDMMC_DataInitTypeDef config = {0};
	uint32_t errorstate;
	uint32_t dataremaining;
	uint8_t *tempbuff = p_data;
	int res;

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Configure DPSM */
	sdhc_stm32_config_sdio_dpsm(instance, is_block_mode, dev_data->total_transfer_bytes,
				    &config, is_write, dev_data);
	(void)SDMMC_ConfigData(instance, &config);
	__SDMMC_CMDTRANS_ENABLE(instance);

	/* Send CMD53 */
	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res == 0) {
		res = sdhc_stm32_set_response(dev_data, instance, cmd);
	}
	if (res != 0) {
		stm32_reg_modify_bits(&instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);
		return -EIO;
	}

	/* Poll on SDMMC flags and transfer data from/to FIFO */
	dataremaining = config.DataLength;

	if (is_write) {
		uint32_t *u32tempbuff = (uint32_t *)tempbuff;

		res = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_TX_FLAGS),
			       timeout_ms * USEC_PER_MSEC, ({
				if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_TXFIFOHE) &&
				    (dataremaining >= SDMMC_HALFFIFOBYTES)) {
					sdhc_stm32_write_fifo_words(instance, &u32tempbuff);
					dataremaining -= SDMMC_HALFFIFOBYTES;
				} else if ((dataremaining < SDMMC_HALFFIFOBYTES) &&
					   (__SDMMC_GET_FLAG(instance,
						     SDMMC_FLAG_TXFIFOHE | SDMMC_FLAG_TXFIFOE))) {
					uint8_t *u8buff = (uint8_t *)u32tempbuff;

					sdhc_stm32_write_remaining_bytes(instance, &u8buff,
									 &dataremaining);
					u32tempbuff = (uint32_t *)u8buff;
				}
				k_yield();
		       }));
	} else {
		res = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_RX_FLAGS),
			       timeout_ms * USEC_PER_MSEC, ({
				if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXFIFOHF) &&
				    (dataremaining >= SDMMC_HALFFIFOBYTES)) {
					sdhc_stm32_read_fifo_block(instance, &tempbuff);
					dataremaining -= SDMMC_HALFFIFOBYTES;
				} else if (dataremaining < SDMMC_HALFFIFOBYTES) {
					sdhc_stm32_read_remaining_bytes(instance, &tempbuff,
									&dataremaining);
				}
				k_yield();
		       }));
	}

	if (res == 0) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}

	__SDMMC_CMDTRANS_DISABLE(instance);

	/* Check for data transfer errors */
	errorstate = __SDMMC_GET_FLAG(instance, SDMMC_DATA_ERROR_FLAGS);
	if (errorstate != 0U) {
		return sdhc_stm32_handle_data_error(instance, errorstate, dev_data);
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

static int sdhc_stm32_sdio_rw_extended_dma(struct sdhc_command *cmd, SDMMC_TypeDef *instance,
					   bool is_block_mode, struct sdhc_stm32_data *dev_data,
					   bool is_write)
{
	SDMMC_DataInitTypeDef config = {0};
	int res;

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	dev_data->is_sdio_transfer = true;

	/* Configure DMA (use single buffer mode) */
	instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
	SDHC_STM32_IDMA_SET_BUFFER(instance, dev_data->sdio_dma_buf);

	/* Configure SDIO Data Path State Machine (DPSM) */
	sdhc_stm32_config_sdio_dpsm(instance, is_block_mode, dev_data->total_transfer_bytes,
				    &config, is_write, dev_data);

	(void)SDMMC_ConfigData(instance, &config);

	__SDMMC_CMDTRANS_ENABLE(instance);

	/* Send CMD53 */
	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res == 0) {
		res = sdhc_stm32_set_response(dev_data, instance, cmd);
	}
	if (res != 0) {
		stm32_reg_modify_bits(&instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
		__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);
		return -EIO;
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(instance,
			  (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
			   (is_write ? SDMMC_IT_TXUNDERR : SDMMC_IT_RXOVERR) | SDMMC_IT_DATAEND));

	return 0;
}

static int sdhc_stm32_switch_speed(SDMMC_TypeDef *instance, uint32_t switch_arg, uint8_t status[64],
				   uint32_t block_size, uint32_t buf_size,
				   struct sdhc_stm32_data *dev_data)
{
	uint32_t errorstate;
	SDMMC_DataInitTypeDef sdmmc_datainitstructure;
	uint32_t sd_hs[16] = {0};
	uint32_t loop = 0;
	int ret;

	if ((block_size == 0U) || (block_size % sizeof(uint32_t) != 0U) || (buf_size < 64U)) {
		dev_data->error_code = SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	/* Initialize the Data control register */
	instance->DCTRL = 0;

	/* Configure the SD DPSM (Data Path State Machine) */
	sdmmc_datainitstructure.DataTimeOut = SDMMC_DATATIMEOUT;
	sdmmc_datainitstructure.DataLength = block_size;
	sdmmc_datainitstructure.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
	sdmmc_datainitstructure.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	sdmmc_datainitstructure.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	sdmmc_datainitstructure.DPSM = SDMMC_DPSM_ENABLE;

	(void)SDMMC_ConfigData(instance, &sdmmc_datainitstructure);

	errorstate = SDMMC_CmdSwitch(instance, switch_arg);
	if (errorstate != SDMMC_ERROR_NONE) {
		dev_data->error_code |= errorstate;
		return -EIO;
	}

	ret = WAIT_FOR(__SDMMC_GET_FLAG(instance, SDMMC_WAIT_RX_DBCKEND_FLAGS),
		       SDMMC_SWDATATIMEOUT * USEC_PER_MSEC, ({
			if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXFIFOHF)) {
				if ((loop + 1) * SDMMC_HALFFIFOBYTES <= sizeof(sd_hs)) {
					uint8_t *buf =
						((uint8_t *)sd_hs) + (loop * SDMMC_HALFFIFOBYTES);

					sdhc_stm32_read_fifo_block(instance, &buf);
					loop++;
				}
			}
			k_yield();
		       }));

	if (ret == 0) {
		dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}

	if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DTIMEOUT);
		dev_data->error_code |= SDMMC_ERROR_DATA_TIMEOUT;
		return -ETIMEDOUT;
	} else if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_DCRCFAIL);
		dev_data->error_code |= SDMMC_ERROR_DATA_CRC_FAIL;
		return -EILSEQ;
	} else if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_RXOVERR)) {
		__SDMMC_CLEAR_FLAG(instance, SDMMC_FLAG_RXOVERR);
		dev_data->error_code |= SDMMC_ERROR_RX_OVERRUN;
		return -EIO;
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_DATA_FLAGS);

	/* Copy switch status to output buffer */
	if (status != NULL) {
		memcpy(status, sd_hs, sizeof(sd_hs));
	}

	return 0;
}

static int sdhc_stm32_rw_extended(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	int res;
	struct sdhc_stm32_data *dev_data = dev->data;
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);
	bool direction = cmd->arg >> SDIO_CMD_ARG_RW_SHIFT;
	bool is_block_mode = cmd->arg & BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT);

	if (data->data == NULL) {
		LOG_ERR("Invalid NULL data buffer passed to CMD53");
		return -EINVAL;
	}

	dev_data->block_size = is_block_mode ? data->block_size : 0;
	dev_data->total_transfer_bytes = data->blocks * data->block_size;

	if (IS_ENABLED(CONFIG_SDHC_STM32_DMA_MODE)) {
		dev_data->sdio_dma_buf = k_aligned_alloc(CONFIG_SDHC_BUFFER_ALIGNMENT,
							 data->blocks * data->block_size);
		if (dev_data->sdio_dma_buf == NULL) {
			LOG_ERR("DMA buffer allocation failed");
			return -ENOMEM;
		}

		if (direction == SDIO_IO_WRITE) {
			memcpy(dev_data->sdio_dma_buf, data->data, dev_data->total_transfer_bytes);
		}

		sys_cache_data_flush_range(dev_data->sdio_dma_buf, dev_data->total_transfer_bytes);
		res = sdhc_stm32_sdio_rw_extended_dma(cmd, instance, is_block_mode, dev_data,
						      direction);

		/* Only wait on semaphore if HAL function succeeded */
		if (res != 0) {
			k_free(dev_data->sdio_dma_buf);
			return res;
		}

		/* Wait for whole transfer to complete */
		if (k_sem_take(&dev_data->device_sync_sem, K_MSEC(data->timeout_ms)) != 0) {
			dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
			sdhc_stm32_abort_dma_transfer(instance, dev_data);
			k_free(dev_data->sdio_dma_buf);
			return -ETIMEDOUT;
		}

		if (direction == SDIO_IO_READ) {
			sys_cache_data_invd_range(dev_data->sdio_dma_buf,
						  dev_data->total_transfer_bytes);
			memcpy(data->data, dev_data->sdio_dma_buf, data->block_size * data->blocks);
		}

		k_free(dev_data->sdio_dma_buf);
	} else {
		res = sdhc_stm32_sdio_rw_extended_poll(cmd, instance, is_block_mode, data->data,
						       data->timeout_ms, dev_data, direction);
	}

	return res;
}

/**
 * @brief Switch the SDMMC interface to 1.8V signaling level.
 *
 * This function performs the voltage switching sequence required for
 * UHS (Ultra High Speed) mode operation.
 *
 * @param dev  Pointer to the device structure for the SDHC instance.
 * @param cmd  Pointer to the SD command used in the voltage switch sequence.
 *
 * @return 0 on success, negative errno on failure
 */
static int sdhc_stm32_switch_to_1_8v(const struct device *dev, struct sdhc_command *cmd)
{
	uint32_t res;
	struct sdhc_stm32_data *dev_data = dev->data;
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);

	instance->POWER |= SDMMC_POWER_VSWITCHEN;

	res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
	if (res != 0) {
		return res;
	}

	res = sdhc_stm32_set_response(dev_data, instance, cmd);
	if (res != 0) {
		return res;
	}

	LOG_DBG("Successfully switched to 1.8V signaling");
	return 0;
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

static int sdhc_stm32_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	int res = 0;
	struct sdhc_stm32_data *dev_data = dev->data;
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);

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
	case SD_ALL_SEND_CID:
	case SD_SEND_RELATIVE_ADDR:
	case SD_SELECT_CARD:
	case SD_APP_CMD:
	case SD_APP_SEND_OP_COND:
	case SD_SET_BLOCK_SIZE:
	case SD_SEND_STATUS:
	case SD_SEND_CSD:
	case SD_SEND_IF_COND:
	case SDIO_SEND_OP_COND:
	case SD_ERASE_BLOCK_START:
	case SD_ERASE_BLOCK_END:
	case SD_ERASE_BLOCK_OPERATION:
		res = sdhc_stm32_send_cmd(dev_data, instance, cmd);
		if (res == 0) {
			res = sdhc_stm32_set_response(dev_data, instance, cmd);
		}
		break;

	case SD_SWITCH:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_switch_speed(instance, cmd->arg, data->data,
					      data->block_size, data->blocks * data->block_size,
					      dev_data);
		break;

	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_rw_blocks(cmd, dev, data, true);
		break;

	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_rw_blocks(cmd, dev, data, false);
		break;

	case SDIO_RW_DIRECT:
		res = sdhc_stm32_sdio_rw_direct(dev_data, instance, cmd);
		break;

	case SDIO_RW_EXTENDED:
		__ASSERT_NO_MSG(data != NULL);
		res = sdhc_stm32_rw_extended(dev, cmd, data);
		break;

	case SD_VOL_SWITCH:
		res = sdhc_stm32_switch_to_1_8v(dev, cmd);
		break;

	case SD_APP_SEND_SCR:
		res = sdhc_stm32_find_scr(cmd, instance, dev_data, (uint32_t *)data->data,
					  data->block_size, data->blocks * data->block_size);
		break;

	default:
		LOG_DBG("Unsupported Command, opcode:%d", cmd->opcode);
		res = -ENOTSUP;
	}

	if (res != 0) {
		LOG_DBG("Command Failed, opcode:%d", cmd->opcode);
		/*
		 * Do not report timeout errors for CMD8 (SEND_IF_COND).
		 * A response timeout here is expected if the card is legacy (SDSC)
		 * and does not support CMD8, so this should not be treated as an error.
		 */
		if (!((cmd->opcode == SD_SEND_IF_COND) &&
		      (dev_data->error_code == SDMMC_ERROR_CMD_RSP_TIMEOUT))) {
			sdhc_stm32_log_err_type(dev_data);
		}
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
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);
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

		if (data->sdmmc_clk == 0U) {
			LOG_ERR("SDMMC clock frequency not configured");
			res = -EIO;
			goto end;
		}

		/* Calculate clock divider
		 * Formula: clock_div = PeripheralClock / (2 * DesiredClock)
		 */
		uint32_t clock_div = data->sdmmc_clk / (2U * ios->clock);

		stm32_reg_modify_bits(&instance->CLKCR, SDMMC_CLKCR_CLKDIV, clock_div);

		host_io->clock = ios->clock;
		LOG_DBG("Clock set to %d", ios->clock);
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		(void)SDMMC_PowerState_OFF(instance);
		k_msleep(data->props.power_delay);
	} else {
		(void)SDMMC_PowerState_ON(instance);
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

		stm32_reg_modify_bits(&instance->CLKCR, SDMMC_CLKCR_WIDBUS,
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
	props->host_caps.max_blk_len = 2;
	props->host_caps.vol_330_support = true;
	props->host_caps.vol_180_support = sdhc_config->support_1_8_v;
	props->host_caps.bus_8_bit_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH8BIT);
	props->bus_4_bit_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH4BIT);
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
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Resetting Host controller */
	(void)SDMMC_PowerState_OFF(instance);
	k_msleep(data->props.power_delay);
	(void)SDMMC_PowerState_ON(instance);
	k_msleep(data->props.power_delay);

	/* Clear error flags */
	__SDMMC_CLEAR_FLAG(instance, SDMMC_STATIC_FLAGS);
	data->error_code = SDMMC_ERROR_NONE;

	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return 0;
}

static void sdhc_stm32_clear_icr_flags(SDMMC_TypeDef *instance)
{
	uint32_t icr_clear_flag = 0;
	uint32_t sta = instance->STA;

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
		instance->ICR = icr_clear_flag;
	}
}

void sdhc_stm32_event_isr(const struct device *dev)
{
	struct sdhc_stm32_data *data = dev->data;
	SDMMC_TypeDef *instance = sdhc_stm32_get_instance(dev);

	if (__SDMMC_GET_FLAG(instance, SDMMC_FLAG_DATAEND | SDMMC_FLAG_DCRCFAIL |
						       SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_RXOVERR |
						       SDMMC_FLAG_TXUNDERR)) {
		k_sem_give(&data->device_sync_sem);
	}

	sdhc_stm32_clear_icr_flags(instance);

	sdhc_stm32_irq_handler(instance, data);

	if (data->error_code != SDMMC_ERROR_NONE) {
		LOG_ERR("Error Interrupt");
		sdhc_stm32_log_err_type(data);
	}
}

static int sdhc_stm32_init(const struct device *dev)
{
	int ret;
	struct sdhc_stm32_data *data = dev->data;
	const struct sdhc_stm32_config *config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

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
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
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
	static struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);                 \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct sdhc_stm32_config sdhc_stm32_cfg_##index = {                           \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(index)),                                          \
		.irq_config_func = sdhc_stm32_irq_config_func_##index,                             \
		.pclken = pclken_##index,                                                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.hw_flow_control = DT_INST_PROP(index, hw_flow_control),                           \
		.clk_div = DT_INST_PROP(index, clk_div),                                           \
		.bus_width = DT_INST_PROP(index, bus_width),                                       \
		.power_delay_ms = DT_INST_PROP(index, power_delay_ms),                             \
		.support_1_8_v = DT_INST_PROP(index, support_1_8_v),                               \
		.sdhi_on_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), pwr_gpios, {0}),           \
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
