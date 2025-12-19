/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief SDHC driver for Infineon MCU family.
 *
 * This driver support only SD protocol of the SD interface.
 *
 *
 * Features:
 * * Supports data transfer using CPU, SDMA, ADMA2 and ADMA3 modes
 * * Supports a configurable block size (1 to 65,535 Bytes)
 * * Supports interrupt enabling and masking
 * * Supports SD-HCI Host version 4 mode or less
 * * Compliant with the SD 6.0, SDIO 4.10 and earlier versions
 * * SD interface features:
 * * - Supports the 4-bit interface
 * * - Supports Ultra High Speed (UHS-I) mode
 * * - Supports Default Speed (DS), High Speed (HS), SDR12, SDR25, SDR50, and DDR50 speed modes
 * * - Supports SDIO card interrupts in both 1-bit and 4-bit modes
 * * - Supports Standard capacity (SDSC), High capacity (SDHC) and Extended capacity (SDXC) memory
 * * - Supports CRC and check for command and data packets
 * * - Supports packet timeouts
 *
 */

#define DT_DRV_COMPAT infineon_sdhc_sdio

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/cache.h>

#include "cy_sd_host.h"
#include "cy_sysclk.h"

LOG_MODULE_REGISTER(sdhc_infineon, CONFIG_SDHC_LOG_LEVEL);

#include <zephyr/irq.h>

#define IFX_SDHC_RETRY_TIMES              (1000UL) /* The number loops to make timeout in msec */
#define IFX_CMD_CMPLT_DELAY_US            (5UL)    /* The Command complete delay */
#define IFX_SD_HOST_MAX_TIMEOUT           (0x0EU)  /* The data max timeout for TOUT_CTRL_R. */
#define IFX_SD_HOST_RETRY_TIME            (1000UL) /* The number loops to make timeout in msec. */
#define IFX_SD_HOST_BUFFER_RDY_TIMEOUT_MS (150U)   /* The Buffer read ready timeout. */
#define IFX_SD_HOST_RD_WR_ENABLE_TIMEOUT  (1U)     /* The valid data in the Host buffer timeout. */
#define IFX_SD_HOST_WRITE_TIMEOUT_MS      (250U)   /* The Write timeout for one block. */
#define IFX_SD_HOST_SD_ACMD_OFFSET        (0x40UL)

struct sdhc_infineon_config {
	const struct pinctrl_dev_config *pincfg;
	SDHC_Type *reg_addr;
	uint8_t irq_priority;
	IRQn_Type irq;
};

struct __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT) sdhc_infineon_data {
	uint32_t adma_descriptor_tbl[2];
	struct k_sem thread_lock;
	struct k_sem transfer_sem;
	struct sdhc_host_props props;
	struct ifx_cat1_clock clock;
	uint32_t bus_clock; /* Value in Hz */
	cy_en_sd_host_bus_width_t bus_width;
	enum sdhc_power power_mode;
	cy_en_sd_host_bus_speed_mode_t speed_mode;
	enum sd_voltage signal_voltage;
	uint8_t clock_peri_group;
	bool app_cmd;
};

static const cy_stc_sd_host_init_config_t sdhc_config = {
	.emmc = false,
	.dmaType = CY_SD_HOST_DMA_ADMA2,
	.enableLedControl = false,
};

static void sdhc_infineon_irq_handler(const struct device *dev)
{
	const struct sdhc_infineon_config *const config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;

	/* Clear all interrupts that could have been configured */
	SDHC_Type *base = config->reg_addr;
	uint32_t interruptStatus = Cy_SD_Host_GetNormalInterruptStatus(base);

	/* CY_SD_HOST_XFER_COMPLETE occurred and appropriate bit in interrupt mask is enabled */
	if (interruptStatus & Cy_SD_Host_GetNormalInterruptMask(base) & CY_SD_HOST_XFER_COMPLETE) {
		/* Clearing transfer complete status */
		Cy_SD_Host_ClearNormalInterruptStatus(base, CY_SD_HOST_XFER_COMPLETE);

		k_sem_give(&sdhc_data->transfer_sem);

		/* Disabling transfer complete interrupt mask */
		Cy_SD_Host_SetNormalInterruptMask(base,
						  Cy_SD_Host_GetNormalInterruptMask(base) &
							  (uint32_t)~CY_SD_HOST_XFER_COMPLETE);
	}
}

static void infineon_sdhc_normal_reset(SDHC_Type *base)
{
	uint32_t intNormal; /* The normal events mask. */

	intNormal = Cy_SD_Host_GetNormalInterruptStatus(base);

	/* Check the normal event. */
	if (0UL < intNormal) {
		/* Clear the normal event. */
		Cy_SD_Host_ClearNormalInterruptStatus(base, intNormal);
	}
}

static void infineon_sdhc_error_reset(SDHC_Type *base)
{
	uint32_t intError; /* The error events mask. */

	intError = Cy_SD_Host_GetErrorInterruptStatus(base);

	/* Check the error event. */
	if (0UL < intError) {
		/* Clear the error event. */
		Cy_SD_Host_ClearErrorInterruptStatus(base, intError);

		Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_CMD_LINE);
	}
}

static int sdhc_infineon_reset(const struct device *dev)
{
	const struct sdhc_infineon_config *config = dev->config;
	uint32_t timeout_us = 1000;

	Cy_SD_Host_SoftwareReset(config->reg_addr, CY_SD_HOST_RESET_DATALINE);
	Cy_SD_Host_SoftwareReset(config->reg_addr, CY_SD_HOST_RESET_CMD_LINE);

	while ((config->reg_addr->CORE.SW_RST_R != 0) && (timeout_us-- != 0)) {
		k_busy_wait(1);
	}

	/* Reset was not cleared by SDHC IP Block. Something wrong. Are clocks enabled? */
	CY_ASSERT(timeout_us != 0);

	return 0;
}

static cy_en_sd_host_response_type_t infineon_sdhc_resp_type(uint32_t response_type)
{
	switch (response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		return CY_SD_HOST_RESPONSE_NONE;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5:
		return CY_SD_HOST_RESPONSE_LEN_48B;
	case SD_RSP_TYPE_R2:
		return CY_SD_HOST_RESPONSE_LEN_136;
	default:
		return CY_SD_HOST_RESPONSE_LEN_48;
	}
}

static cy_en_sd_host_cmd_type_t infineon_sdhc_cmd_type(uint32_t opcode)
{
	switch (opcode) {
	case SD_GO_IDLE_STATE:
	case SD_STOP_TRANSMISSION:
		return CY_SD_HOST_CMD_ABORT;
	default:
		return CY_SD_HOST_CMD_NORMAL;
	}
}

static bool infineon_sdhc_dma_enable(cy_stc_sd_host_cmd_config_t *cmdConfig)
{
	switch (cmdConfig->commandIndex) {
	case SD_SWITCH:
	case SD_SEND_STATUS:
	case (SD_APP_SEND_SCR + IFX_SD_HOST_SD_ACMD_OFFSET):
		return false;
	default:
		return true;
	}
}

static cy_en_sd_host_auto_cmd_t infineon_sdhc_autocommand(cy_stc_sd_host_cmd_config_t *cmdConfig,
							  struct sdhc_data *data)
{
	if (cmdConfig->commandIndex == SDIO_RW_EXTENDED) {
		return CY_SD_HOST_AUTO_CMD_NONE;
	}

	return (data->blocks > 1UL) ? CY_SD_HOST_AUTO_CMD_AUTO : CY_SD_HOST_AUTO_CMD_NONE;
}

static cy_en_sd_host_auto_cmd_t infineon_sdhc_intatblockgap(cy_stc_sd_host_cmd_config_t *cmdConfig,
							    struct sdhc_data *data)
{
	if (cmdConfig->commandIndex == SDIO_RW_EXTENDED) {
		return false;
	}

	return (data->blocks > 1UL) ? true : false;
}

static cy_en_sd_host_status_t infineon_sdhc_pollcmdcomplete(const struct device *dev,
							    uint32_t *time_used_ms)
{
	const struct sdhc_infineon_config *config = dev->config;
	cy_en_sd_host_status_t result = CY_SD_HOST_ERROR_TIMEOUT;
	uint32_t retry = IFX_SDHC_RETRY_TIMES * IFX_CMD_CMPLT_DELAY_US;

	while (retry > 0UL) {
		/* Command complete */
		if (CY_SD_HOST_CMD_COMPLETE ==
		    (CY_SD_HOST_CMD_COMPLETE &
		     Cy_SD_Host_GetNormalInterruptStatus(config->reg_addr))) {

			/* Clear interrupt flag */
			Cy_SD_Host_ClearNormalInterruptStatus(config->reg_addr,
							      CY_SD_HOST_CMD_COMPLETE);

			result = CY_SD_HOST_SUCCESS;
			break;
		}

		k_busy_wait(1);
		retry--;
	}

	return result;
}

static cy_rslt_t infineon_sdhc_prepare_for_transfer(const struct device *dev)
{
	const struct sdhc_infineon_config *config = dev->config;

	/* Enabling transfer complete interrupt as it takes part in write / read processes */
	Cy_SD_Host_SetNormalInterruptMask(config->reg_addr,
					  Cy_SD_Host_GetNormalInterruptMask(config->reg_addr) |
						  CY_SD_HOST_XFER_COMPLETE);

	return CY_RSLT_SUCCESS;
}

static cy_en_sd_host_status_t infineon_sdhc_host_poll_transfer_complete(SDHC_Type *base)
{
	cy_en_sd_host_status_t ret = CY_SD_HOST_ERROR_TIMEOUT;
	uint32_t retry = IFX_SD_HOST_RETRY_TIME;

	while (retry > 0UL) {
		/* Transfer Complete */
		if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_XFER_COMPLETE,
				      SDHC_CORE_NORMAL_INT_STAT_R(base))) {
			/* Clear the interrupt flag */
			SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_XFER_COMPLETE;

			ret = CY_SD_HOST_SUCCESS;
			break;
		}

		k_busy_wait(IFX_SD_HOST_WRITE_TIMEOUT_MS);
		retry--;
	}

	return ret;
}

static cy_en_sd_host_status_t infineon_sdhc_poll_buf_read_ready(SDHC_Type *base)
{
	cy_en_sd_host_status_t ret = CY_SD_HOST_ERROR_TIMEOUT;
	uint32_t retry = IFX_SD_HOST_RETRY_TIME;

	while (retry > 0UL) {
		/* Check the Buffer Read ready */
		if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_BUF_RD_READY,
				      SDHC_CORE_NORMAL_INT_STAT_R(base))) {
			/* Clear the interrupt flag */
			SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_BUF_RD_READY;

			ret = CY_SD_HOST_SUCCESS;
			break;
		}

		k_busy_wait(IFX_SD_HOST_BUFFER_RDY_TIMEOUT_MS);
		retry--;
	}

	return ret;
}

static cy_en_sd_host_status_t infineon_sdhc_cmd_rx_data(SDHC_Type *base,
							cy_stc_sd_host_data_config_t *pcmd)
{
	cy_en_sd_host_status_t ret;
	uint32_t blkSize;
	uint32_t blkCnt;
	uint32_t i;
	uint32_t retry;

	blkCnt = pcmd->numberOfBlock;
	blkSize = pcmd->blockSize;

	while (blkCnt > 0UL) {
		/* Wait for the Buffer Read ready. */
		ret = infineon_sdhc_poll_buf_read_ready(base);

		if (ret != CY_SD_HOST_SUCCESS) {
			break;
		}

		for (i = blkSize >> 2UL; i != 0UL; i--) {
			/* Wait if valid data exists in the Host buffer. */
			retry = IFX_SD_HOST_RETRY_TIME;
			while ((false == _FLD2BOOL(SDHC_CORE_PSTATE_REG_BUF_RD_ENABLE,
						   SDHC_CORE_PSTATE_REG(base))) &&
			       (retry > 0UL)) {
				k_busy_wait(IFX_SD_HOST_RD_WR_ENABLE_TIMEOUT);
				retry--;
			}

			if (false == _FLD2BOOL(SDHC_CORE_PSTATE_REG_BUF_RD_ENABLE,
					       SDHC_CORE_PSTATE_REG(base))) {
				break;
			}

			/* Read data from the Host buffer. */
			*pcmd->data = Cy_SD_Host_BufferRead(base);
			pcmd->data++;
		}
		blkCnt--;
	}

	/* Wait for the Transfer Complete. */
	ret = infineon_sdhc_host_poll_transfer_complete(base);

	return ret;
}

static cy_rslt_t infineon_sdhc_config_data_transfer(const struct device *dev,
						    struct sdhc_data *data,
						    cy_stc_sd_host_data_config_t *dataConfig)
{
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;

	dataConfig->blockSize = data->block_size;
	dataConfig->numberOfBlock = data->blocks;
	dataConfig->dataTimeout = IFX_SD_HOST_MAX_TIMEOUT;
	dataConfig->enReliableWrite = false;

	if (dataConfig->enableDma == true) {
		/* ADMA2 mode. */
		uint32_t length = data->block_size * data->blocks;
#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
		sys_cache_data_flush_range(data->data, length);
#endif
		sdhc_data->adma_descriptor_tbl[0] =
			(1UL << CY_SD_HOST_ADMA_ATTR_VALID_POS) | /* Attr Valid */
			(1UL << CY_SD_HOST_ADMA_ATTR_END_POS) |   /* Attr End */
			(0UL << CY_SD_HOST_ADMA_ATTR_INT_POS) |   /* Attr Int */
			(CY_SD_HOST_ADMA_TRAN << CY_SD_HOST_ADMA_ACT_POS) |
			(length << CY_SD_HOST_ADMA_LEN_POS); /* Len */

		/* SDHC needs to be able to access the data_ptr that is in DTCM when using CM55.
		 * Remap this address for access.
		 */
#if defined(CORE_NAME_CM55_0)
		sdhc_data->adma_descriptor_tbl[1] = (uint32_t)cy_DTCMRemapAddr(data->data);
		dataConfig->data = (uint32_t *)cy_DTCMRemapAddr(&sdhc_data->adma_descriptor_tbl[0]);
#else
		sdhc_data->adma_descriptor_tbl[1] = (uint32_t)data->data;
		dataConfig->data = (uint32_t *)&sdhc_data->adma_descriptor_tbl[0];
#endif

#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
		sys_cache_data_flush_range((void *)sdhc_data->adma_descriptor_tbl,
					   sizeof(sdhc_data->adma_descriptor_tbl));
#endif
	} else {
		dataConfig->data = (uint32_t *)data->data;
	}

	return (cy_rslt_t)Cy_SD_Host_InitDataTransfer(config->reg_addr, dataConfig);
}

static cy_rslt_t infineon_sdhc_send_cmd(const struct device *dev,
					cy_stc_sd_host_cmd_config_t *cmdConfig,
					struct sdhc_data *data, bool is_read)
{
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;
	cy_stc_sd_host_data_config_t dataConfig;
	cy_rslt_t result = CY_RSLT_SUCCESS;

	/* First clear out the transfer and command complete statuses */
	Cy_SD_Host_ClearNormalInterruptStatus(config->reg_addr,
					      (CY_SD_HOST_XFER_COMPLETE | CY_SD_HOST_CMD_COMPLETE));

	if (cmdConfig->dataPresent) {
		dataConfig.read = is_read;
		dataConfig.enableDma = infineon_sdhc_dma_enable(cmdConfig);
		dataConfig.autoCommand = infineon_sdhc_autocommand(cmdConfig, data);
		dataConfig.enableIntAtBlockGap = infineon_sdhc_intatblockgap(cmdConfig, data);
		result = infineon_sdhc_config_data_transfer(dev, data, &dataConfig);

		if (result == CY_RSLT_SUCCESS && dataConfig.enableDma == true) {
			result = infineon_sdhc_prepare_for_transfer(dev);
		}
	}

	if (result == CY_RSLT_SUCCESS) {
		result = (cy_rslt_t)Cy_SD_Host_SendCommand(config->reg_addr, cmdConfig);
	}

	if (result == CY_RSLT_SUCCESS) {
		result = (cy_rslt_t)infineon_sdhc_pollcmdcomplete(dev, NULL);
	}

	if (result == CY_RSLT_SUCCESS) {
		if (cmdConfig->dataPresent) {
			if (dataConfig.enableDma == true) {
				result = k_sem_take(&sdhc_data->transfer_sem,
						    K_MSEC(data->timeout_ms));
				if (result != 0UL) {
					LOG_ERR("Can not take sem!");
				}

#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
				if (dataConfig.read == true) {
					sys_cache_data_invd_range(data->data,
								  data->block_size * data->blocks);
				}
#endif
			} else {
				/* DMA is not used - wait until all data is read. */
				result = infineon_sdhc_cmd_rx_data(config->reg_addr, &dataConfig);
			}
		}
	}

	return result;
}

static int sdhc_infineon_request(const struct device *dev, struct sdhc_command *cmd,
				 struct sdhc_data *data)
{
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;
	bool large_response;
	int result = 0;

	LOG_DBG("Opcode: %d", cmd->opcode);

	/* Reset semaphore */
	k_sem_reset(&sdhc_data->transfer_sem);
	result = k_sem_take(&sdhc_data->thread_lock, K_FOREVER);
	if (result < 0) {
		LOG_ERR("Can not take sem!");
		goto end;
	}

	cy_stc_sd_host_cmd_config_t cmdConfig = {
		.commandIndex = cmd->opcode,
		.commandArgument = cmd->arg,
		.enableCrcCheck = true,
		.enableAutoResponseErrorCheck = false,
		.respType = infineon_sdhc_resp_type(cmd->response_type),
		.enableIdxCheck = true,
		.dataPresent = (data != NULL) ? true : false,
		.cmdType = infineon_sdhc_cmd_type(cmd->opcode),
	};

	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		cmdConfig.enableCrcCheck = false;
		cmdConfig.enableIdxCheck = false;
		/* No response CMD so no complete interrupt */
		(void)infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);

		/* Software reset for the CMD line */
		Cy_SD_Host_SoftwareReset(config->reg_addr, CY_SD_HOST_RESET_CMD_LINE);
		break;

	case SD_SEND_IF_COND:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		Cy_SD_Host_GetResponse(config->reg_addr, cmd->response, false);
		if ((cmd->response[0] & 0xFF) != SD_IF_COND_CHECK) {
			/* Reset the error and the CMD line for the case of the SDIO card. */
			infineon_sdhc_error_reset(config->reg_addr);
			infineon_sdhc_normal_reset(config->reg_addr);
		}
		goto end;

	case MMC_SEND_OP_COND:
	case SDIO_SEND_OP_COND:
	case SD_SELECT_CARD:
		cmdConfig.enableCrcCheck = false;
		cmdConfig.enableIdxCheck = false;
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_APP_SEND_OP_COND:
		cmdConfig.commandIndex += IFX_SD_HOST_SD_ACMD_OFFSET;
		cmdConfig.enableCrcCheck = false;
		cmdConfig.enableIdxCheck = false;
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_ALL_SEND_CID:
	case SD_SEND_CSD:
		cmdConfig.enableCrcCheck = true;
		cmdConfig.enableIdxCheck = false;
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_SEND_STATUS:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		k_busy_wait(500);
		break;

	case SD_SEND_RELATIVE_ADDR:
	case SD_SET_BLOCK_SIZE:
	case SD_ERASE_BLOCK_START:
	case SD_ERASE_BLOCK_END:
	case SD_ERASE_BLOCK_OPERATION:
	case SDIO_RW_DIRECT:
	case SD_APP_CMD:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		if (result != 0UL) {
			LOG_INF("Kevin CMD%d result:0x%x", cmd->opcode, result);
		}
		break;

	case SD_VOL_SWITCH:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		k_msleep(200);
		break;

	case SD_SWITCH:
		/* Check app cmd */
		if (sdhc_data->app_cmd && cmd->opcode == SD_APP_SET_BUS_WIDTH) {
			cmdConfig.commandIndex += IFX_SD_HOST_SD_ACMD_OFFSET;
			cmdConfig.enableCrcCheck = false;
			cmdConfig.enableIdxCheck = false;
			result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		} else {
			result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		}
		break;

	case SDIO_RW_EXTENDED:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data,
						!(cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)));
		k_busy_wait(500);
		break;

	case SD_APP_SEND_NUM_WRITTEN_BLK:
		cmdConfig.commandIndex += IFX_SD_HOST_SD_ACMD_OFFSET;
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_APP_SEND_SCR:
		cmdConfig.commandIndex += IFX_SD_HOST_SD_ACMD_OFFSET;
		cmdConfig.respType = CY_SD_HOST_RESPONSE_LEN_48B;
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, true);
		break;

	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		result = infineon_sdhc_send_cmd(dev, &cmdConfig, data, false);
		break;

	default:
		result = -ENOTSUP;
	}

	if (cmdConfig.respType != CY_SD_HOST_RESPONSE_NONE) {
		large_response = (cmdConfig.respType == CY_SD_HOST_RESPONSE_LEN_136) ? true : false;
		Cy_SD_Host_GetResponse(config->reg_addr, cmd->response, large_response);
	}

end:
	if (cmd->opcode == SD_APP_CMD) {
		sdhc_data->app_cmd = true;
	} else {
		sdhc_data->app_cmd = false;
	}
	k_sem_give(&sdhc_data->thread_lock);

	return result;
}

static cy_rslt_t infineon_sdhc_find_best_div(uint32_t hz_src, uint32_t desired_hz,
					     uint32_t *divider)
{
	/* Rounding up for correcting the error in integer division
	 * to ensure the actual frequency is less than or equal to
	 * the requested frequency.
	 * Ensure computed divider is no more than 10-bit.
	 */
	if (hz_src > desired_hz) {
		uint32_t freq = (desired_hz << 1);
		uint32_t calculated_divider = ((hz_src + freq - 1) / freq) & 0x3FF;
		/* Real divider is 2 x calculated_divider */
		*divider = calculated_divider << 1;
	} else {
		*divider = 1;
	}

	return CY_RSLT_SUCCESS;
}

static cy_en_sd_host_status_t infineon_sdhc_change_clock(const struct device *dev,
							 uint32_t *frequency)
{
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;
	cy_rslt_t result = CY_RSLT_SUCCESS;
	uint32_t most_suitable_div = 0;
	uint32_t bus_freq = 0;
	uint32_t source_freq = 0;
	en_clk_dst_t clk_idx;

	if (config->reg_addr == SDHC0) {
		clk_idx = PCLK_SDHC0_CLK_HF;
	} else if (config->reg_addr == SDHC1) {
		clk_idx = PCLK_SDHC1_CLK_HF;
	} else {
		return CY_SD_HOST_ERROR_INVALID_PARAMETER;
	}

	source_freq =
		ifx_cat1_utils_peri_pclk_get_frequency((en_clk_dst_t)clk_idx, &sdhc_data->clock);

	if (result == CY_RSLT_SUCCESS) {
		result = infineon_sdhc_find_best_div(source_freq, *frequency, &most_suitable_div);
	}

	if (result == CY_RSLT_SUCCESS) {
		bus_freq = source_freq / most_suitable_div;
	}

	if (result == CY_RSLT_SUCCESS) {
		Cy_SD_Host_DisableSdClk(config->reg_addr);
		result =
			(cy_rslt_t)Cy_SD_Host_SetSdClkDiv(config->reg_addr, most_suitable_div >> 1);
		Cy_SD_Host_EnableSdClk(config->reg_addr);
	}

	if (result == CY_RSLT_SUCCESS) {
		*frequency = bus_freq;
	}

	return result;
}

static void infineon_sdhc_card_power_cycle(const struct device *dev, enum sdhc_power power_mode)
{
	const struct sdhc_infineon_config *config = dev->config;

	if (power_mode == SDHC_POWER_ON) {
		Cy_SD_Host_EnableCardVoltage(config->reg_addr);
	}
}

static int sdhc_infineon_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *const sdhc_data = dev->data;
	cy_en_sd_host_bus_width_t bus_width;
	cy_en_sd_host_bus_speed_mode_t speed_mode;
	int ret = 0;

	LOG_INF("SDHC I/O: bus width %d, clock %dHz, card power %s, voltage %s, timing %d",
		ios->bus_width, ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V", ios->timing);

	/* Toggle card power supply */
	if (sdhc_data->power_mode != ios->power_mode) {
		infineon_sdhc_card_power_cycle(dev, ios->power_mode);
		sdhc_data->power_mode = ios->power_mode;
	}

	if (ios->bus_width > 0) {
		/* Set bus width */
		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			bus_width = CY_SD_HOST_BUS_WIDTH_1_BIT;
			break;
		case SDHC_BUS_WIDTH4BIT:
			bus_width = CY_SD_HOST_BUS_WIDTH_4_BIT;
			break;
		case SDHC_BUS_WIDTH8BIT:
			bus_width = CY_SD_HOST_BUS_WIDTH_8_BIT;
			return -ENOTSUP;
		default:
			return -ENOTSUP;
		}

		if (sdhc_data->bus_width != bus_width) {
			/* Update the host side setting. */
			ret = Cy_SD_Host_SetHostBusWidth(config->reg_addr, bus_width);

			if (ret == 0) {
				LOG_INF("Bus width set successfully to %d bit", ios->bus_width);
			} else {
				LOG_ERR("Error configuring bus width");
				return -EINVAL;
			}

			sdhc_data->bus_width = bus_width;
		}
	}

	if (ios->timing > 0) {
		/* Set I/O timing */
		switch (ios->timing) {
		case SDHC_TIMING_LEGACY:
			speed_mode = CY_SD_HOST_BUS_SPEED_DEFAULT;
			break;
		case SDHC_TIMING_HS:
			speed_mode = CY_SD_HOST_BUS_SPEED_HIGHSPEED;
			break;
		case SDHC_TIMING_SDR12:
			speed_mode = CY_SD_HOST_BUS_SPEED_SDR12_5;
			break;
		case SDHC_TIMING_SDR25:
			speed_mode = CY_SD_HOST_BUS_SPEED_SDR25;
			break;
		case SDHC_TIMING_SDR50:
			speed_mode = CY_SD_HOST_BUS_SPEED_SDR50;
			break;
		case SDHC_TIMING_DDR50:
			speed_mode = CY_SD_HOST_BUS_SPEED_DDR50;
			break;
		default:
			LOG_ERR("Timing mode not supported for this device");
			return -ENOTSUP;
		}

		if (sdhc_data->speed_mode != speed_mode) {
			ret = Cy_SD_Host_SetHostSpeedMode(config->reg_addr, speed_mode);

			if (ret == 0) {
				LOG_INF("Timing set successfully to %d", ios->timing);
			} else {
				LOG_ERR("Error configuring Timing");
				return -EINVAL;
			}

			sdhc_data->speed_mode = speed_mode;
		}
	}

	if (ios->clock != sdhc_data->bus_clock) {
		if (ios->clock == 0) {
			/* Disable providing the SD Clock. */
			Cy_SD_Host_DisableSdClk(config->reg_addr);
		} else {
			/* Check for frequency boundaries supported by host */
			if (ios->clock > sdhc_data->props.f_max ||
			    ios->clock < sdhc_data->props.f_min) {
				LOG_ERR("Proposed clock outside supported host range");
				return -EINVAL;
			}

			uint32_t actual_freq = ios->clock;

			/* Try setting new clock */
			ret = infineon_sdhc_change_clock(dev, &actual_freq);

			if (ret == 0) {
				LOG_INF("Bus clock successfully set to %d kHz", ios->clock / 1000);
			} else {
				LOG_ERR("Error configuring card clock");
				return -EINVAL;
			}
		}

		sdhc_data->bus_clock = (uint32_t)ios->clock;
	}

	if (sdhc_data->signal_voltage != ios->signal_voltage) {
		switch (ios->signal_voltage) {
		case SD_VOL_3_3_V:
			Cy_SD_Host_ChangeIoVoltage(config->reg_addr, CY_SD_HOST_IO_VOLT_3_3V);
			break;
		case SD_VOL_1_8_V:
			/* Switch the bus to 1.8 V (Set the IO_VOLT_SEL pin to low)*/
			Cy_SD_Host_ChangeIoVoltage(config->reg_addr, CY_SD_HOST_IO_VOLT_1_8V);
			break;
		default:
			return -ENOTSUP;
		}

		sdhc_data->signal_voltage = ios->signal_voltage;
	}

	return ret;
}

static int sdhc_infineon_get_card_present(const struct device *dev)
{
	return true;
}

static int sdhc_infineon_execute_tuning(const struct device *dev)
{
	return 0;
}

static int sdhc_infineon_card_busy(const struct device *dev)
{
	const struct sdhc_infineon_config *config = dev->config;
	int busy_status = 0;
	/* Check DAT Line Active */
	uint32_t pState = Cy_SD_Host_GetPresentState(config->reg_addr);

	if (((pState & CY_SD_HOST_DAT_3_0) == 0) ||
	    ((pState & CY_SD_HOST_DAT_LINE_ACTIVE) == CY_SD_HOST_DAT_LINE_ACTIVE) ||
	    ((pState & CY_SD_HOST_CMD_CMD_INHIBIT_DAT) == CY_SD_HOST_CMD_CMD_INHIBIT_DAT)) {
		busy_status = 1;
	}

	return busy_status;
}

static int sdhc_infineon_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_infineon_data *sdhc_data = dev->data;

	memcpy(props, &sdhc_data->props, sizeof(struct sdhc_host_props));

	return 0;
}

bool Cy_SD_Host_IsCardConnected(SDHC_Type const *base)
{
	(void)base;
	return true;
}

static int sdhc_infineon_init(const struct device *dev)
{
	cy_en_sd_host_status_t result = 0;
	const struct sdhc_infineon_config *config = dev->config;
	struct sdhc_infineon_data *sdhc_data = dev->data;
	cy_stc_sd_host_context_t context;

	/* Configure DT provided device signals when available */
	result = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (result) {
		return result;
	}

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	if (config->reg_addr == SDHC0) {
		Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SDHC0_PERI_NR, CY_MMIO_SDHC0_GROUP_NR,
					     CY_MMIO_SDHC0_SLAVE_NR, CY_MMIO_SDHC0_CLK_HF_NR);
	} else if (config->reg_addr == SDHC1) {
		Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SDHC1_PERI_NR, CY_MMIO_SDHC1_GROUP_NR,
					     CY_MMIO_SDHC1_SLAVE_NR, CY_MMIO_SDHC1_CLK_HF_NR);
	}
#endif

	k_sem_init(&sdhc_data->thread_lock, 1, 1);
	k_sem_init(&sdhc_data->transfer_sem, 1, 1);

	/* Enable the SDHC block */
	Cy_SD_Host_Enable(config->reg_addr);

	/* Configure SD Host to operate */
	result = Cy_SD_Host_Init(config->reg_addr, &sdhc_config, &context);
	if (result != CY_SD_HOST_SUCCESS) {
		return -EFAULT;
	}

	irq_enable(config->irq);

	/* Clear slot data so card is initialized at set_io() */
	sdhc_data->bus_clock = 0UL;
	sdhc_data->bus_width = CY_SD_HOST_BUS_WIDTH_1_BIT;
	sdhc_data->power_mode = SDHC_POWER_OFF;
	sdhc_data->speed_mode = CY_SD_HOST_BUS_SPEED_DEFAULT;
	sdhc_data->signal_voltage = SD_VOL_3_3_V;

	return 0;
}

static DEVICE_API(sdhc, sdhc_infineon_api) = {
	.reset = sdhc_infineon_reset,
	.request = sdhc_infineon_request,
	.set_io = sdhc_infineon_set_io,
	.get_card_present = sdhc_infineon_get_card_present,
	.execute_tuning = sdhc_infineon_execute_tuning,
	.card_busy = sdhc_infineon_card_busy,
	.get_host_props = sdhc_infineon_get_host_props,
};

#define IFX_SDHC_IRQ_INIT(n)                                                                       \
	void sdhc_infineon_isr_##n(void)                                                           \
	{                                                                                          \
		sdhc_infineon_irq_handler(DEVICE_DT_INST_GET(n));                                  \
	}                                                                                          \
	static void sdhc_infineon_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq), DT_INST_IRQ_BY_IDX(n, 1, priority),     \
			    sdhc_infineon_isr_##n, DEVICE_DT_INST_GET(n), 0);                      \
	}

#define IFX_SDHC_IRQ_CONFIG(n) sdhc_infineon_irq_config_##n();

#define IRQ_INFO(n)                                                                                \
	.irq = DT_INST_IRQN_BY_IDX(n, 1), .irq_priority = DT_INST_IRQ_BY_IDX(n, 1, priority)

#define PERI_INFO(n) .clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),

#define IFX_SDHC_PERI_CLOCK_INIT(n)                                                                \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),         \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
	},                                                                                         \
	PERI_INFO(n)

#define IFX_SDHC_INIT(n)                                                                           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	IFX_SDHC_IRQ_INIT(n)                                                                       \
                                                                                                   \
	static int sdhc_infineon_init##n(const struct device *dev)                                 \
	{                                                                                          \
		IFX_SDHC_IRQ_CONFIG(n);                                                            \
		return sdhc_infineon_init(dev);                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct sdhc_infineon_config sdhc_infineon_##n##_config = {                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.reg_addr = (SDHC_Type *)DT_INST_REG_ADDR(n),                                      \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
		IRQ_INFO(n)};                                                                      \
                                                                                                   \
	static struct sdhc_infineon_data sdhc_infineon_##n##_data = {                              \
		.power_mode = SDHC_POWER_ON,                                                       \
		.speed_mode = CY_SD_HOST_BUS_SPEED_DEFAULT,                                        \
		.props = {.is_spi = false,                                                         \
			  .f_max = DT_INST_PROP(n, max_bus_freq),                                  \
			  .f_min = DT_INST_PROP(n, min_bus_freq),                                  \
			  .power_delay = DT_INST_PROP(n, power_delay_ms),                          \
			  .host_caps = {.vol_180_support = !DT_INST_PROP(n, no_1_8_v),             \
					.vol_300_support = false,                                  \
					.vol_330_support = true,                                   \
					.suspend_res_support = false,                              \
					.sdma_support = true,                                      \
					.high_spd_support =                                        \
						(DT_INST_PROP(n, bus_width) == 4) ? true : false,  \
					.adma_2_support = true,                                    \
					.adma3_support = true,                                     \
					.sdio_async_interrupt_support = true,                      \
					.ddr50_support = false,                                    \
					.sdr104_support = false,                                   \
					.sdr50_support = true,                                     \
					.bus_8_bit_support = false,                                \
					.bus_4_bit_support =                                       \
						(DT_INST_PROP(n, bus_width) == 4) ? true : false,  \
					.hs200_support = false,                                    \
					.hs400_support = false}},                                  \
		IFX_SDHC_PERI_CLOCK_INIT(n)};                                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sdhc_infineon_init##n, NULL, &sdhc_infineon_##n##_data,          \
			      &sdhc_infineon_##n##_config, PRE_KERNEL_1,                           \
			      CONFIG_SDHC_INIT_PRIORITY, &sdhc_infineon_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SDHC_INIT)
