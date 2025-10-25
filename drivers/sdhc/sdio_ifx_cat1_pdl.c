/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @brief PDL based SDIO driver for Infineon CAT1 MCU family.
 *
 * This driver support only SDIO protocol of the SD interface for general
 * I/O functions.
 *
 * Refer to the SD Specifications Part 1 SDIO Specifications Version 4.10 for more
 * information on the SDIO protocol and specifications.
 *
 * Features
 * - Supports 4-bit interface
 * - Supports Ultra High Speed (UHS-I) mode
 * - Supports Default Speed (DS), High Speed (HS), SDR12, SDR25 and SDR50 speed modes
 * - Supports SDIO card interrupts in both 1-bit SD and 4-bit SD modes
 * - Supports Standard capacity (SDSC), High capacity (SDHC) and
 *   Extended capacity (SDXC) memory
 *
 * Note (limitations):
 * - current version of ifx_cat1_sdio supports only following set of commands:
 *   > GO_IDLE_STATE      (CMD0)
 *   > SEND_RELATIVE_ADDR (CMD3)
 *   > IO_SEND_OP_COND    (CMD5)
 *   > SELECT_CARD        (CMD7)
 *   > VOLTAGE_SWITCH     (CMD11)
 *   > GO_INACTIVE_STATE  (CMD15)
 *   > IO_RW_DIRECT       (CMD52)
 *   > IO_RW_EXTENDED     (CMD53)
 */

#define DT_DRV_COMPAT infineon_cat1_pdl_sdhc_sdio

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

#include "cy_sd_host.h"
#include "cy_sysclk.h"

LOG_MODULE_REGISTER(ifx_cat1_sdio, CONFIG_SDHC_LOG_LEVEL);

#include <zephyr/irq.h>

/**
 * Data transfer status on SDHC/SDIO
 */
typedef enum {
	/** No data transfer in progress */
	_MTB_HAL_SDXX_NOT_RUNNING = 0x0,
	/** Waiting for a command to complete */
	_MTB_HAL_SDXX_WAIT_CMD_COMPLETE = 0x1,
	/** Waiting for a transfer to complete */
	_MTB_HAL_SDXX_WAIT_XFER_COMPLETE = 0x2,
	/** Waiting for completion of both a command and a transfer */
	_MTB_HAL_SDXX_WAIT_BOTH = _MTB_HAL_SDXX_WAIT_CMD_COMPLETE | _MTB_HAL_SDXX_WAIT_XFER_COMPLETE
} _mtb_hal_sdxx_data_transfer_status_t;

struct ifx_cat1_sdio_config {
	const struct pinctrl_dev_config *pincfg;
	SDHC_Type *reg_addr;
	uint8_t irq_priority;
};

struct ifx_cat1_sdio_data {
	enum sdhc_clock_speed clock_speed;
	enum sdhc_bus_width bus_width;
	_mtb_hal_sdxx_data_transfer_status_t data_transfer_status;

	void *sdio_cb_user_data;
	sdhc_interrupt_cb_t sdio_cb;
};

static const cy_stc_sd_host_init_config_t host_config = {false, CY_SD_HOST_DMA_ADMA2, false};

/** Commands that can be issued */
typedef enum {
	CYHAL_SDIO_CMD_GO_IDLE_STATE = 0,      //!< Go to idle state
	CYHAL_SDIO_CMD_SEND_RELATIVE_ADDR = 3, //!< Send a relative address
	CYHAL_SDIO_CMD_IO_SEND_OP_COND = 5,    //!< Send an OP IO
	CYHAL_SDIO_CMD_SELECT_CARD = 7,        //!< Send a card select
	CYHAL_SDIO_CMD_VOLTAGE_SWITCH = 11,    //!< Voltage switch
	CYHAL_SDIO_CMD_GO_INACTIVE_STATE = 15, //!< Go to inactive state
	CYHAL_SDIO_CMD_IO_RW_DIRECT = 52,      //!< Perform a direct read/write
	CYHAL_SDIO_CMD_IO_RW_EXTENDED = 53,    //!< Perform an extended read/write
} ifx_sdio_host_command_t;

/** Types of transfer that can be performed */
typedef enum {
	CYHAL_SDIO_XFER_TYPE_READ, //!< Read from the card
	CYHAL_SDIO_XFER_TYPE_WRITE //!< Write to the card
} ifx_sdio_host_transfer_type_t;

#define IFX_SDIO_TRANSFER_TRIES (50U)

static void sdio_reset(SDHC_Type *base)
{
	/* Usually it is enough ~ 15 us for reset to complete on 100 MHz HF clock for any supported
	 * compiler / optimization. Making max time much greater to cover different frequencies and
	 * unusual cases. Timeout and SW_RST_R check will be removed after DRIVERS-5769 is resolved.
	 */
	uint32_t timeout_us = 1000;

	Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_DATALINE);
	Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_CMD_LINE);

	/* This wait loop will be removed after DRIVERS-5769 resolved. */
	while ((base->CORE.SW_RST_R != 0) && (timeout_us-- != 0)) {
		Cy_SysLib_DelayUs(1);
	}

	/* Reset was not cleared by SDHC IP Block. Something wrong. Are clocks enabled? */
	CY_ASSERT(timeout_us != 0);
}

static int ifx_cat1_sdio_reset(const struct device *dev)
{
	const struct ifx_cat1_sdio_config *config = dev->config;
	struct ifx_cat1_sdio_data *dev_data = dev->data;

	uint32_t timeout_us = 1000;

	dev_data->data_transfer_status = _MTB_HAL_SDXX_NOT_RUNNING;
	Cy_SD_Host_SoftwareReset(config->reg_addr, CY_SD_HOST_RESET_DATALINE);
	Cy_SD_Host_SoftwareReset(config->reg_addr, CY_SD_HOST_RESET_CMD_LINE);

	/* This wait loop will be removed after DRIVERS-5769 resolved. */
	while ((config->reg_addr->CORE.SW_RST_R != 0) && (timeout_us-- != 0)) {
		Cy_SysLib_DelayUs(1);
	}

	/* Reset was not cleared by SDHC IP Block. Something wrong. Are clocks enabled? */
	CY_ASSERT(timeout_us != 0);

	return 0;
}

static int ifx_cat1_sdio_set_io(const struct device *dev, struct sdhc_io *ios)
{
	/* NOTE: Set bus width, set card power, set host signal voltage,
	 * set I/O timing does not support in current version of driver
	 */

	return 0;
}

static int ifx_cat1_sdio_card_busy(const struct device *dev)
{
	struct ifx_cat1_sdio_data *dev_data = dev->data;

	return (_MTB_HAL_SDXX_NOT_RUNNING != dev_data->data_transfer_status);
}

static cy_en_sd_host_status_t sdxx_prepare_for_transfer(SDHC_Type *base)
{
	uint32_t activated_cmd_complete = 0;

	/* Enabling transfer complete interrupt as it takes part in in write / read processes */
	Cy_SD_Host_SetNormalInterruptMask(base, Cy_SD_Host_GetNormalInterruptMask(base) |
							activated_cmd_complete |
							CY_SD_HOST_XFER_COMPLETE);

	return CY_SD_HOST_SUCCESS;
}

static cy_en_sd_host_status_t sdxx_pollcmdcomplete(SDHC_Type *base)
{
	cy_en_sd_host_status_t result = CY_SD_HOST_ERROR_TIMEOUT;
	uint32_t retry = 1000;

	while (retry > 0UL) {
		/* Command complete */
		if (CY_SD_HOST_CMD_COMPLETE ==
		    (CY_SD_HOST_CMD_COMPLETE & Cy_SD_Host_GetNormalInterruptStatus(base))) {
			// dev_data->data_transfer_status &= ~_MTB_HAL_SDXX_WAIT_CMD_COMPLETE;

			/* Clear interrupt flag */
			Cy_SD_Host_ClearNormalInterruptStatus(base, CY_SD_HOST_CMD_COMPLETE);

			result = CY_SD_HOST_SUCCESS;
			break;
		}

		Cy_SysLib_DelayUs(5);
		retry--;
	}

	return result;
}

static cy_en_sd_host_status_t sdxx_polltransfercomplete(SDHC_Type *base, const uint16_t delay)
{
	cy_en_sd_host_status_t result = CY_SD_HOST_ERROR_TIMEOUT;
	uint32_t retry = 1000;
	uint32_t status = 0UL;

	while ((CY_SD_HOST_ERROR_TIMEOUT == result) && (retry-- > 0U)) {
		/* We check for either the interrupt register or the byte set in the
		 * _cyhal_sdxx_irq_handler to avoid a deadlock in the case where if an API that is
		 * polling is called from an ISR and its priority is higher than the priority of the
		 * _cyhal_sdxx_irq_handler thus not allowing the signalling byte to be set.
		 */
		status = Cy_SD_Host_GetNormalInterruptStatus(base);
		if (CY_SD_HOST_XFER_COMPLETE == (CY_SD_HOST_XFER_COMPLETE & status)) {
			/* Transfer complete */
			result = CY_SD_HOST_SUCCESS;
			break;
		}
		Cy_SysLib_DelayUs(delay);
	}

	return result;
}

static cy_en_sd_host_status_t sdio_host_transfer_async(SDHC_Type *base,
						       ifx_sdio_host_transfer_type_t direction,
						       uint32_t argument, const uint32_t *data,
						       uint16_t length)
{
	cy_en_sd_host_status_t result;
	uint32_t adma_descriptor_tbl[2];
	uint32_t retry = IFX_SDIO_TRANSFER_TRIES;
	cy_stc_sd_host_cmd_config_t sdhc_cmd;
	cy_stc_sd_host_data_config_t sdhc_data;

	/* Initialize data constants*/
	sdhc_data.autoCommand = CY_SD_HOST_AUTO_CMD_NONE;
	sdhc_data.dataTimeout = 0x0dUL;
	sdhc_data.enableIntAtBlockGap = false;
	sdhc_data.enReliableWrite = false;
	sdhc_data.enableDma = true;

	// Before we perform any operations with the DMA, flush the D-cache, if enabled.
	// Casting away const is safe to do, because it will be a no-op
	// if data has no dirty entries in the cache, and a const variable
	// that is stored in flash will never have dirty entries in the cache
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	if (direction == CYHAL_SDIO_XFER_TYPE_WRITE) {
		SCB_CleanDCache_by_Addr((void *)data, length);
	}
#endif

	do {
		/* Add SDIO Error Handling
		 * SDIO write timeout is expected when doing first write to register
		 * after KSO bit disable (as it goes to AOS core).
		 * This timeout, however, triggers an error state in the hardware.
		 * So, check for the error and then recover from it
		 * as needed via reset issuance. This is the only time known that
		 * a write timeout occurs.
		 */

		/* First clear out the command complete and transfer complete statuses */
		Cy_SD_Host_ClearNormalInterruptStatus(
			base, (CY_SD_HOST_XFER_COMPLETE | CY_SD_HOST_CMD_COMPLETE));

		/* Check if an error occurred on any previous transactions or reset after the first
		 * unsuccessful transfer try */
		if ((Cy_SD_Host_GetNormalInterruptStatus(base) & CY_SD_HOST_ERR_INTERRUPT) ||
		    (retry < IFX_SDIO_TRANSFER_TRIES)) {
			/* Reset the block if there was an error. Note a full reset usually
			 * requires more time, but this short version is working quite well and
			 * successfully clears out the error state.
			 */
			Cy_SD_Host_ClearErrorInterruptStatus(base, 0x61FFUL);
			sdio_reset(base);
		}

		/* Prepare the data transfer register */
		sdhc_cmd.commandIndex = (uint32_t)CYHAL_SDIO_CMD_IO_RW_EXTENDED;
		sdhc_cmd.commandArgument = argument;
		sdhc_cmd.enableCrcCheck = true;
		sdhc_cmd.enableAutoResponseErrorCheck = false;
		sdhc_cmd.respType = CY_SD_HOST_RESPONSE_LEN_48;
		sdhc_cmd.enableIdxCheck = true;
		sdhc_cmd.dataPresent = true;
		sdhc_cmd.cmdType = CY_SD_HOST_CMD_NORMAL;

		sdhc_data.read = (direction == CYHAL_SDIO_XFER_TYPE_WRITE) ? false : true;

		/* Block mode */ // TODO: SREP
		if (length >= 64) {
			sdhc_data.blockSize = 64;
			sdhc_data.numberOfBlock = (length + 64 - 1) / 64;
		}
		/* Byte mode */
		else {
			sdhc_data.blockSize = length;
			sdhc_data.numberOfBlock = 1UL;
		}

		length = sdhc_data.blockSize * sdhc_data.numberOfBlock;

		adma_descriptor_tbl[0] = (1UL << CY_SD_HOST_ADMA_ATTR_VALID_POS) | /* Attr Valid */
					 (1UL << CY_SD_HOST_ADMA_ATTR_END_POS) |   /* Attr End */
					 (0UL << CY_SD_HOST_ADMA_ATTR_INT_POS) |   /* Attr Int */
					 (CY_SD_HOST_ADMA_TRAN << CY_SD_HOST_ADMA_ACT_POS) |
					 ((uint32_t)length << CY_SD_HOST_ADMA_LEN_POS); /* Len */

		adma_descriptor_tbl[1] = (uint32_t)data;

		/* The address of the ADMA descriptor table. */
		sdhc_data.data = (uint32_t *)&(adma_descriptor_tbl[0]);

		result = sdxx_prepare_for_transfer(base);


#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
		SCB_CleanDCache_by_Addr((void *)adma_descriptor_tbl, sizeof(adma_descriptor_tbl));
#endif

		if (CY_SD_HOST_SUCCESS == result) {
			result = (cy_rslt_t)Cy_SD_Host_InitDataTransfer(base, &sdhc_data);
		}

		if (CY_SD_HOST_SUCCESS == result) {
			/* Indicate that async transfer in progress */
			// dev_data->data_transfer_status = _MTB_HAL_SDXX_WAIT_BOTH;
			result = (cy_rslt_t)Cy_SD_Host_SendCommand(base, &sdhc_cmd);
		}

		if (CY_SD_HOST_SUCCESS == result) {
			result = (cy_rslt_t)sdxx_pollcmdcomplete(base);
		}

	} while ((CY_SD_HOST_SUCCESS != result) && (--retry > 0UL));

	if (CY_SD_HOST_SUCCESS != result) {
		/* Transfer failed */
		// dev_data->data_transfer_status = _MTB_HAL_SDXX_NOT_RUNNING;
	}

// Invalidate dcache if enabled to update dcache's contents after DMA transfer
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	if (direction == CYHAL_SDIO_XFER_TYPE_READ) {
		SCB_InvalidateDCache_by_Addr((void *)data, length);
	}
#endif

	return result;
}

static cy_en_sd_host_status_t sdio_host_bulk_transfer(SDHC_Type *base,
						      ifx_sdio_host_transfer_type_t direction,
						      uint32_t argument, const uint32_t *data,
						      uint16_t length, uint32_t *response)
{
	cy_en_sd_host_status_t result = CY_RSLT_SUCCESS;
	uint32_t retry = IFX_SDIO_TRANSFER_TRIES;

	// _cyhal_sdxx_t *dev_data = &(obj->dev_data);

	// TODO: SREP
	// _cyhal_sdxx_setup_smphr(dev_data);

	do {
		result = sdio_host_transfer_async(base, direction, argument, data, length);

		if (CY_SD_HOST_SUCCESS == result) {
			// TODO: SREP : Add logic for wait herre once interrupts start working
			//  result = sdxx_waitfor_transfer_complete(dev_data);
			result = sdxx_polltransfercomplete(base, 500);
		}

		if (CY_SD_HOST_SUCCESS != result) {
			/*  SDIO Error Handling
			 *   SDIO write timeout is expected when doing first write to register
			 *   after KSO bit disable (as it goes to AOS core).
			 *   This is the only time known that a write timeout occurs.
			 *   Issue the reset to recover from error.
			 */
			sdio_reset(base);
		}

	} while ((CY_SD_HOST_SUCCESS != result) && (--retry > 0UL));

	// if (CY_SD_HOST_SUCCESS != result) {
	// 	dev_data->data_transfer_status = _MTB_HAL_SDXX_NOT_RUNNING;
	// }

	if ((response != NULL) && (CY_SD_HOST_SUCCESS == result)) {
		*response = 0UL;
		result = Cy_SD_Host_GetResponse(base, response, false);
	}

	return result;
}

static cy_en_sd_host_status_t sdio_host_send_cmd(SDHC_Type *base, ifx_sdio_host_command_t command,
						 uint32_t argument, uint32_t *response)
{
	cy_en_sd_host_status_t result;
	cy_stc_sd_host_cmd_config_t cmd;
	uint32_t retry = IFX_SDIO_TRANSFER_TRIES;

	/* Clear out the response */
	if (response != NULL) {
		*response = 0UL;
	}

	do {
		cmd.commandIndex = (uint32_t)command;
		cmd.commandArgument = argument;
		cmd.enableCrcCheck = true;
		cmd.enableAutoResponseErrorCheck = false;
		cmd.respType = CY_SD_HOST_RESPONSE_LEN_48;
		cmd.enableIdxCheck = true;
		cmd.dataPresent = false;
		cmd.cmdType = CY_SD_HOST_CMD_NORMAL;

		result = (cy_rslt_t)Cy_SD_Host_SendCommand(base, &cmd);
		if (CY_SD_HOST_SUCCESS == result) {
			/* Wait for the Command Complete event. */
			result = sdxx_pollcmdcomplete(base);
		}

	} while ((CY_RSLT_SUCCESS != result) && (retry-- > 0UL));

	if (CY_RSLT_SUCCESS == result) {
		result = (cy_rslt_t)Cy_SD_Host_GetResponse(base, response, false);
	}

	return result;
}

static int ifx_cat1_sdio_request(const struct device *dev, struct sdhc_command *cmd,
				 struct sdhc_data *data)
{
	const struct ifx_cat1_sdio_config *config = dev->config;
	cy_en_sd_host_status_t result;

	LOG_DBG("Opcode: %d", cmd->opcode);

	switch (cmd->opcode) {
	case CYHAL_SDIO_CMD_GO_IDLE_STATE:
	case CYHAL_SDIO_CMD_SEND_RELATIVE_ADDR:
	case CYHAL_SDIO_CMD_IO_SEND_OP_COND:
	case CYHAL_SDIO_CMD_SELECT_CARD:
	case CYHAL_SDIO_CMD_VOLTAGE_SWITCH:
	case CYHAL_SDIO_CMD_GO_INACTIVE_STATE:
	case CYHAL_SDIO_CMD_IO_RW_DIRECT:
		result = sdio_host_send_cmd(config->reg_addr, cmd->opcode, cmd->arg, cmd->response);
		if (result != CY_SD_HOST_SUCCESS) {
			LOG_ERR("sdio_host_send_cmd failed result = %d", result);
			return -EIO;
		}
		break;

	case CYHAL_SDIO_CMD_IO_RW_EXTENDED:
		ifx_sdio_host_transfer_type_t direction;

		direction = (cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) ? CYHAL_SDIO_XFER_TYPE_WRITE
								    : CYHAL_SDIO_XFER_TYPE_READ;

		result = sdio_host_bulk_transfer(config->reg_addr, direction, cmd->arg, data->data,
						 data->blocks * data->block_size, cmd->response);

		if (result != CY_SD_HOST_SUCCESS) {
			LOG_ERR("sdio_host_bulk_transfer failed result = %d", result);
			return -EIO;
		}
		break;

	default:
		result = -ENOTSUP;
	}

	return 0;
}

static int ifx_cat1_sdio_get_card_present(const struct device *dev)
{
	return 1;
}

static int ifx_cat1_sdio_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	memset(props, 0, sizeof(*props));
	props->f_max = SD_CLOCK_50MHZ;
	props->f_min = SDMMC_CLOCK_400KHZ;
	props->host_caps.bus_4_bit_support = true;
	props->host_caps.high_spd_support = true;
	props->host_caps.sdr50_support = true;
	props->host_caps.sdio_async_interrupt_support = true;
	props->host_caps.vol_330_support = true;

	return 0;
}

static cy_en_sd_host_status_t ifx_cat1_sdio_change_clock(SDHC_Type *base, uint32_t frequency)
{
#define SDHC_CLK_INPUT 100000000UL /* 100 MHz for other targets*/

	cy_en_sd_host_status_t result = CY_SD_HOST_ERROR_INVALID_PARAMETER;
	uint32_t clkDiv;
	uint32_t clockInput = SDHC_CLK_INPUT;

	if (base == NULL) {
		return CY_SD_HOST_ERROR_INVALID_PARAMETER;
	}

	clkDiv = (clockInput / frequency) >> 1UL;

	Cy_SD_Host_DisableSdClk(base);
	result = Cy_SD_Host_SetSdClkDiv(base, (uint16_t)clkDiv);
	Cy_SD_Host_EnableSdClk(base);

	return result;
}

static int ifx_cat1_sdio_init(const struct device *dev)
{
	cy_en_sd_host_status_t result = 0;
	const struct ifx_cat1_sdio_config *config = dev->config;
	struct ifx_cat1_sdio_data *dev_data = dev->data;

	/* Configure DT provided device signals when available */
	result = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (result) {
		return result;
	}

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SDHC0_PERI_NR, CY_MMIO_SDHC0_GROUP_NR,
				     CY_MMIO_SDHC0_SLAVE_NR, CY_MMIO_SDHC0_CLK_HF_NR);

	dev_data->data_transfer_status = _MTB_HAL_SDXX_NOT_RUNNING;

	/* Enable the SDHC block */
	Cy_SD_Host_Enable(config->reg_addr);

	/* Configure SD Host to operate */
	cy_stc_sd_host_context_t context = {0};
	context.cardType = CY_SD_HOST_NOT_EMMC;
	result = Cy_SD_Host_Init(config->reg_addr, &host_config, &context);
	if (result != CY_SD_HOST_SUCCESS) {
		return -EFAULT;
	}

	/* Don't enable any error interrupts for now */
	Cy_SD_Host_SetErrorInterruptMask(config->reg_addr, 0UL);

	/* Clear all interrupts */
	Cy_SD_Host_ClearErrorInterruptStatus(config->reg_addr, 0x61FFUL);
	Cy_SD_Host_ClearNormalInterruptStatus(config->reg_addr, 0x61FFUL);

	result = Cy_SD_Host_SetHostBusWidth(config->reg_addr, CY_SD_HOST_BUS_WIDTH_4_BIT);
	if (result != CY_SD_HOST_SUCCESS) {
		return -EFAULT;
	}

	ifx_cat1_sdio_change_clock(config->reg_addr, SD_CLOCK_50MHZ);

	return 0;
}

static DEVICE_API(sdhc, ifx_cat1_sdio_api) = {
	.reset = ifx_cat1_sdio_reset,
	.request = ifx_cat1_sdio_request,
	.set_io = ifx_cat1_sdio_set_io,
	.get_card_present = ifx_cat1_sdio_get_card_present,
	.card_busy = ifx_cat1_sdio_card_busy,
	.get_host_props = ifx_cat1_sdio_get_host_props,
};

#define IFX_CAT1_SDHC_INIT(n)                                                                      \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct ifx_cat1_sdio_config ifx_cat1_sdio_##n##_config = {                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.reg_addr = (SDHC_Type *)DT_INST_REG_ADDR(n),                                      \
		.irq_priority = DT_INST_IRQ(n, priority)};                                         \
                                                                                                   \
	static struct ifx_cat1_sdio_data ifx_cat1_sdio_##n##_data;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_sdio_init, NULL, &ifx_cat1_sdio_##n##_data,             \
			      &ifx_cat1_sdio_##n##_config, PRE_KERNEL_1,                           \
			      CONFIG_SDHC_INIT_PRIORITY, &ifx_cat1_sdio_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CAT1_SDHC_INIT)
