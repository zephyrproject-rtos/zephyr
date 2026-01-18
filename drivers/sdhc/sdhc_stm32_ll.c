/*
 * Copyright (c) 2025 EXALT Technologies.
 * Copyright (c) 2017 STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This file is derived from STM32Cube HAL (stm32XXxx_hal_sd.c and stm32XXxx_hal_sdio.c)
 * with refactoring to align with Zephyr coding style and architecture.
 */

#include "sdhc_stm32_ll.h"
#include <zephyr/logging/log.h>
#include <stm32_bitops.h>

LOG_MODULE_REGISTER(sdhc_stm32_ll, CONFIG_SDHC_LOG_LEVEL);

/* Private validation macros for SDIO parameters */
#define IS_SDIO_RAW_FLAG(ReadAfterWrite) (((ReadAfterWrite) == 0U) || ((ReadAfterWrite) == 1U))
#define IS_SDIO_FUNCTION(FN)             (((FN) >= 0U) && ((FN) <= 7U))

/**
 * @brief Check and handle data transfer error flags
 *
 * @param hsd Pointer to SDMMC handle
 * @return SDMMC_ERROR_NONE if no error, otherwise appropriate error code
 */
static uint32_t sdhc_stm32_ll_check_data_errors(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t errorcode = SDMMC_ERROR_NONE;

	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
		errorcode |= SDMMC_ERROR_DATA_TIMEOUT;
	}
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
		errorcode |= SDMMC_ERROR_DATA_CRC_FAIL;
	}
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
		errorcode |= SDMMC_ERROR_RX_OVERRUN;
	}
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR)) {
		errorcode |= SDMMC_ERROR_TX_UNDERRUN;
	}

	return errorcode;
}

/**
 * @brief Read multiple words from FIFO (FIFO half-full condition)
 *
 * @param hsd Pointer to SDMMC handle
 * @param pBuf Pointer to destination buffer pointer (updated after read)
 */
static void sdhc_stm32_ll_read_fifo_block(sdhc_stm32_ll_handle_t *hsd, uint8_t **pBuf)
{
	uint32_t data;

	for (uint32_t count = 0U; count < 8U; count++) {
		data = SDMMC_ReadFIFO(hsd->Instance);
		*(*pBuf)++ = (uint8_t)(data & 0xFFU);
		*(*pBuf)++ = (uint8_t)((data >> 8U) & 0xFFU);
		*(*pBuf)++ = (uint8_t)((data >> 16U) & 0xFFU);
		*(*pBuf)++ = (uint8_t)((data >> 24U) & 0xFFU);
	}
}

/**
 * @brief Write multiple words to FIFO
 *
 * @param hsd Pointer to SDMMC handle
 * @param pBuf Pointer to source buffer pointer (updated after write)
 */
static void sdhc_stm32_ll_write_fifo_block(sdhc_stm32_ll_handle_t *hsd, const uint8_t **pBuf)
{
	uint32_t data;

	for (uint32_t count = 0U; count < 8U; count++) {
		data = (uint32_t)(*(*pBuf)++);
		data |= ((uint32_t)(*(*pBuf)++) << 8U);
		data |= ((uint32_t)(*(*pBuf)++) << 16U);
		data |= ((uint32_t)(*(*pBuf)++) << 24U);
		SDMMC_WriteFIFO(hsd->Instance, &data);
	}
}

/**
 * @brief Handle data transfer errors and cleanup
 *
 * @param hsd Pointer to SDMMC handle
 * @param errorcode Error code to set
 * @return int 0 on success, negative errno on failure
 */
static int sdhc_stm32_ll_handle_data_error(sdhc_stm32_ll_handle_t *hsd,
					   uint32_t errorcode)
{
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
	hsd->ErrorCode |= errorcode;
	hsd->State = SDMMC_STATE_READY;
	if (errorcode == SDMMC_ERROR_DATA_TIMEOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

/**
 * @brief Disable all data transfer interrupts
 *
 * @param hsd Pointer to SDMMC handle
 */
static inline void sdhc_stm32_ll_disable_data_interrupts(sdhc_stm32_ll_handle_t *hsd)
{
	__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
						  SDMMC_IT_TXUNDERR | SDMMC_IT_RXOVERR |
						  SDMMC_IT_TXFIFOHE | SDMMC_IT_RXFIFOHF);
	__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_IDMABTC);
}

/* Forward declaration for sdhc_stm32_ll_convert_block_size */
static uint32_t sdhc_stm32_ll_convert_block_size(uint32_t block_size);

/**
 * @brief Validate SDIO extended command parameters
 *
 * @param hsd Pointer to SDMMC handle
 * @param Argument Extended command argument structure
 * @param pData Pointer to data buffer
 * @return int 0 on success, negative errno on failure
 */
static int
sdhc_stm32_ll_validate_sdio_ext_params(sdhc_stm32_ll_handle_t *hsd,
				       sdhc_stm32_sdio_ext_cmd_t *Argument, uint8_t *pData)
{
	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	return 0;
}

/**
 * @brief Configure SDIO data path state machine for extended commands
 *
 * @param hsd Pointer to SDMMC handle
 * @param Argument Extended command argument structure
 * @param Size_byte Number of bytes to transfer
 * @param nbr_of_block Number of blocks (output)
 * @param config Data configuration structure (output)
 * @param is_write true for write operation, false for read
 */
static void sdhc_stm32_ll_config_sdio_dpsm(sdhc_stm32_ll_handle_t *hsd,
					   sdhc_stm32_sdio_ext_cmd_t *Argument, uint32_t Size_byte,
					   uint32_t *nbr_of_block, SDMMC_DataInitTypeDef *config,
					   bool is_write)
{
	/* Compute number of blocks */
	*nbr_of_block = (Size_byte & ~(hsd->block_size & 1U)) >> __CLZ(__RBIT(hsd->block_size));

	/* Initialize data control register */
	if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		hsd->Instance->DCTRL = 0U;
	}

	/* Configure SDIO Data Path State Machine (DPSM) */
	config->DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config->DataLength = (uint32_t)((*nbr_of_block) * hsd->block_size);
		config->DataBlockSize = sdhc_stm32_ll_convert_block_size(hsd->block_size);
	} else {
		config->DataLength = (Size_byte > 0U) ? Size_byte : 512U;
		config->DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config->TransferDir = is_write ? SDMMC_TRANSFER_DIR_TO_CARD : SDMMC_TRANSFER_DIR_TO_SDMMC;
	config->TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				       ? SDMMC_TRANSFER_MODE_BLOCK
				       : SDMMC_TRANSFER_MODE_SDIO;
	config->DPSM = SDMMC_DPSM_DISABLE;
}

/**
 * @brief Build CMD53 argument for SDIO extended read/write
 *
 * @param Argument Extended command argument structure
 * @param nbr_of_block Number of blocks
 * @param Size_byte Size in bytes
 * @param is_write true for write operation, false for read
 * @return uint32_t CMD53 argument value
 */
static uint32_t sdhc_stm32_ll_build_cmd53_arg(sdhc_stm32_sdio_ext_cmd_t *Argument,
					      uint32_t nbr_of_block, uint32_t Size_byte,
					      bool is_write)
{
	uint32_t cmd;
	uint32_t count_field;

	cmd = is_write ? (1U << 31U) : (0U << 31U);
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;

	if (nbr_of_block == 0U) {
		count_field = Size_byte;
	} else {
		count_field = nbr_of_block;
	}
	cmd |= count_field & 0x1FFU;

	return cmd;
}

/**
 * @brief Handle CMD53 command error
 *
 * @param hsd Pointer to SDMMC handle
 * @param errorstate Error state from command
 * @return true if fatal error (should return), false to continue
 */
static bool sdhc_stm32_ll_handle_cmd53_error(sdhc_stm32_ll_handle_t *hsd, uint32_t errorstate)
{
	if (errorstate == 0) {
		return false;
	}

	hsd->ErrorCode |= errorstate;

	if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
			   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
		stm32_reg_modify_bits(&hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST,
				      SDMMC_DCTRL_FIFORST);
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
		hsd->State = SDMMC_STATE_READY;
		return true;
	}

	return false;
}

/**
 * @brief Read remaining bytes from FIFO (less than 32 bytes)
 *
 * @param hsd Pointer to SDMMC handle
 * @param tempbuff Pointer to buffer pointer (updated after read)
 * @param dataremaining Pointer to remaining data count (updated after read)
 */
static void sdhc_stm32_ll_read_remaining_bytes(sdhc_stm32_ll_handle_t *hsd, uint8_t **tempbuff,
					       uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byteCount;
	uint32_t bytes_to_read;

	while ((*dataremaining > 0U) && !(__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOE))) {
		data = SDMMC_ReadFIFO(hsd->Instance);
		bytes_to_read = (*dataremaining < 4U) ? *dataremaining : 4U;
		for (byteCount = 0U; byteCount < bytes_to_read; byteCount++) {
			**tempbuff = (uint8_t)((data >> (byteCount * 8U)) & 0xFFU);
			(*tempbuff)++;
		}
		*dataremaining -= bytes_to_read;
	}
}

/**
 * @brief Write remaining bytes to FIFO (less than 32 bytes)
 *
 * @param hsd Pointer to SDMMC handle
 * @param u8buff Pointer to buffer
 * @param dataremaining Pointer to remaining data count (updated after write)
 */
static void sdhc_stm32_ll_write_remaining_bytes(sdhc_stm32_ll_handle_t *hsd, uint8_t **u8buff,
						uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byteCount;
	uint32_t bytes_to_write;

	while (*dataremaining > 0U) {
		bytes_to_write = (*dataremaining < 4U) ? *dataremaining : 4U;

		data = 0U;
		for (byteCount = 0U; byteCount < bytes_to_write; byteCount++) {
			data |= ((uint32_t)(**u8buff) << (byteCount << 3U));
			(*u8buff)++;
		}
		hsd->Instance->FIFO = data;
		*dataremaining -= bytes_to_write;
	}
}

/**
 * @brief Check for timeout during SDIO extended transfer
 *
 * @param hsd Pointer to SDMMC handle
 * @param tickstart Start tick value
 * @param Timeout_Ms Timeout in milliseconds
 * @return true if timeout occurred, false otherwise
 */
static bool sdhc_stm32_ll_check_timeout(sdhc_stm32_ll_handle_t *hsd, uint32_t tickstart,
					uint32_t Timeout_Ms)
{
	if ((k_uptime_get_32() - tickstart) >= Timeout_Ms) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_TIMEOUT;
		hsd->State = SDMMC_STATE_READY;
		return true;
	}
	return false;
}

int sdhc_stm32_ll_erase(sdhc_stm32_ll_handle_t *hsd, uint32_t BlockStartAdd,
					uint32_t BlockEndAdd)
{
	uint32_t errorstate;
	uint32_t start_add = BlockStartAdd;
	uint32_t end_add = BlockEndAdd;

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if (end_add < start_add) {
			hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
			return -EINVAL;
		}

		if (end_add > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return -EINVAL;
		}

		hsd->State = SDMMC_STATE_BUSY;

		/* Check if the card command class supports erase command */
		if (((hsd->SdCard.Class) & SDMMC_CCCC_ERASE) == 0U) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
			hsd->State = SDMMC_STATE_READY;
			return -ENOTSUP;
		}

		if ((SDMMC_GetResponse(hsd->Instance, SDMMC_RESP1) & SDMMC_CARD_LOCKED) ==
		    SDMMC_CARD_LOCKED) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_LOCK_UNLOCK_FAILED;
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}

		/* Get start and end block for high capacity cards */
		if (hsd->SdCard.CardType != CARD_SDHC_SDXC) {
			start_add *= BLOCKSIZE;
			end_add *= BLOCKSIZE;
		}

		/* According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and
		 * erase_group_end(CMD33)
		 */
		if (hsd->SdCard.CardType != CARD_SECURED) {
			/* Send CMD32 SD_ERASE_GRP_START with argument as addr  */
			errorstate = SDMMC_CmdSDEraseStartAdd(hsd->Instance, start_add);
			if (errorstate != 0) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= errorstate;
				hsd->State = SDMMC_STATE_READY;
				return -EIO;
			}

			/* Send CMD33 SD_ERASE_GRP_END with argument as addr  */
			errorstate = SDMMC_CmdSDEraseEndAdd(hsd->Instance, end_add);
			if (errorstate != 0) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= errorstate;
				hsd->State = SDMMC_STATE_READY;
				return -EIO;
			}
		}

		/* Send CMD38 ERASE */
		errorstate = SDMMC_CmdErase(hsd->Instance, 0UL);
		if (errorstate != 0) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}

		hsd->State = SDMMC_STATE_READY;

		return 0;
	} else {
		return -EBUSY;
	}
}

uint32_t sdhc_stm32_ll_send_status(sdhc_stm32_ll_handle_t *hsd, uint32_t card_rca,
				   uint32_t *pCardStatus)
{
	uint32_t errorstate;

	if (pCardStatus == NULL) {
		return SDMMC_ERROR_INVALID_PARAMETER;
	}

	/* Send Status command */
	errorstate = SDMMC_CmdSendStatus(hsd->Instance, card_rca);
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	/* Get SD card status */
	*pCardStatus = SDMMC_GetResponse(hsd->Instance, SDMMC_RESP1);

	return SDMMC_ERROR_NONE;
}

/**
 * @brief  Sends SD SWITCH command (CMD6) and retrieves switch status.
 *         This function handles the data transfer for CMD6 and returns
 *         the 64-byte switch status.
 * @param  hsd: SD handle
 * @param  switch_arg: CMD6 argument containing mode and function settings
 * @param  status: Pointer to buffer for 64-byte switch status response
 * @retval SD Card error state
 */
uint32_t sdhc_stm32_ll_switch_speed(sdhc_stm32_ll_handle_t *hsd, uint32_t switch_arg,
				    uint8_t *status, uint32_t block_size)
{
	uint32_t errorstate = SDMMC_ERROR_NONE;
	SDMMC_DataInitTypeDef sdmmc_datainitstructure;
	uint32_t SD_hs[16] = {0};
	uint32_t count;
	uint32_t loop = 0;
	uint32_t Timeout = HAL_GetTick();

	/* Initialize the Data control register */
	hsd->Instance->DCTRL = 0;

	/* Configure the SD DPSM (Data Path State Machine) */
	sdmmc_datainitstructure.DataTimeOut = SDMMC_DATATIMEOUT;
	sdmmc_datainitstructure.DataLength = block_size;
	sdmmc_datainitstructure.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
	sdmmc_datainitstructure.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	sdmmc_datainitstructure.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	sdmmc_datainitstructure.DPSM = SDMMC_DPSM_ENABLE;

	(void)SDMMC_ConfigData(hsd->Instance, &sdmmc_datainitstructure);

	errorstate = SDMMC_CmdSwitch(hsd->Instance, switch_arg);
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND |
							SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF)) {
			for (count = 0U; count < 8U; count++) {
				SD_hs[(8U * loop) + count] = SDMMC_ReadFIFO(hsd->Instance);
			}
			loop++;
		}
		if ((HAL_GetTick() - Timeout) >= SDMMC_SWDATATIMEOUT) {
			hsd->ErrorCode = SDMMC_ERROR_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT);

		return SDMMC_ERROR_DATA_TIMEOUT;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL);

		return SDMMC_ERROR_DATA_CRC_FAIL;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR);

		return SDMMC_ERROR_RX_OVERRUN;
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	/* Copy switch status to output buffer */
	if (status != NULL) {
		memcpy(status, SD_hs, 64U);
	}

	return errorstate;
}

uint32_t sdhc_stm32_ll_find_scr(sdhc_stm32_ll_handle_t *hsd, uint32_t *scr, uint32_t block_size)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t index = 0U;
	uint32_t tempscr[2U] = {0UL, 0UL};

	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = block_size;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_8B;
	config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_ENABLE;
	(void)SDMMC_ConfigData(hsd->Instance, &config);

	/* Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
	errorstate = SDMMC_CmdSendSCR(hsd->Instance);
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND |
							SDMMC_FLAG_DATAEND)) {
		if ((!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOE)) && (index == 0U)) {
			tempscr[0] = SDMMC_ReadFIFO(hsd->Instance);
			tempscr[1] = SDMMC_ReadFIFO(hsd->Instance);
			index++;
		}

		if ((HAL_GetTick() - tickstart) >= SDMMC_SWDATATIMEOUT) {
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT);

		return SDMMC_ERROR_DATA_TIMEOUT;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL);

		return SDMMC_ERROR_DATA_CRC_FAIL;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR);

		return SDMMC_ERROR_RX_OVERRUN;
	}
	/* No error flag set */
	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	*scr = (((tempscr[1] & SDMMC_0TO7BITS) << 24U) | ((tempscr[1] & SDMMC_8TO15BITS) << 8U) |
		((tempscr[1] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[1] & SDMMC_24TO31BITS) >> 24U));
	scr++;
	*scr = (((tempscr[0] & SDMMC_0TO7BITS) << 24U) | ((tempscr[0] & SDMMC_8TO15BITS) << 8U) |
		((tempscr[0] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[0] & SDMMC_24TO31BITS) >> 24U));

	return SDMMC_ERROR_NONE;
}

SDMMC_CardStateTypeDef sdhc_stm32_ll_get_card_state(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t cardstate;
	uint32_t errorstate;
	uint32_t resp1 = 0;

	errorstate = sdhc_stm32_ll_send_status(hsd, (uint32_t)(hsd->SdCard.RelCardAdd), &resp1);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
	}

	cardstate = ((resp1 >> 9U) & 0x0FU);

	return (SDMMC_CardStateTypeDef)cardstate;
}

/**
 * @brief Poll for data transfer in read mode
 *
 * @param hsd Pointer to SDMMC handle
 * @param tempbuff Pointer to buffer pointer (updated as data is read)
 * @param dataremaining Pointer to remaining data count (updated as data is read)
 * @param tickstart Start tick for timeout calculation
 * @param Timeout Timeout value in milliseconds
 * @return int 0 on success, negative errno on failure
 */
static int sdhc_stm32_ll_poll_read_transfer(sdhc_stm32_ll_handle_t *hsd,
							    uint8_t **tempbuff,
							    uint32_t *dataremaining,
							    uint32_t tickstart, uint32_t Timeout)
{
	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) &&
		    (*dataremaining >= SDMMC_FIFO_SIZE)) {
			/* Read data from SDMMC Rx FIFO */
			sdhc_stm32_ll_read_fifo_block(hsd, tempbuff);
			*dataremaining -= SDMMC_FIFO_SIZE;
		}

		if (((HAL_GetTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return -ETIMEDOUT;
		}
	}
	return 0;
}

/**
 * @brief Poll for data transfer in write mode
 *
 * @param hsd Pointer to SDMMC handle
 * @param tempbuff Pointer to buffer pointer (updated as data is written)
 * @param dataremaining Pointer to remaining data count (updated as data is written)
 * @param tickstart Start tick for timeout calculation
 * @param Timeout Timeout value in milliseconds
 * @return int 0 on success, negative errno on failure
 */
static int sdhc_stm32_ll_poll_write_transfer(sdhc_stm32_ll_handle_t *hsd,
							     const uint8_t **tempbuff,
							     uint32_t *dataremaining,
							     uint32_t tickstart, uint32_t Timeout)
{
	uint32_t errorstate = SDMMC_ERROR_NONE;

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) &&
		    (*dataremaining >= SDMMC_FIFO_SIZE)) {
			/* Write data to SDMMC Tx FIFO */
			sdhc_stm32_ll_write_fifo_block(hsd, tempbuff);
			*dataremaining -= SDMMC_FIFO_SIZE;
		}

		if (((HAL_GetTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return -ETIMEDOUT;
		}
	}
	return 0;
}

/**
 * @brief Send stop transmission command for multi-block transfers
 *
 * @param hsd Pointer to SDMMC handle
 * @param NumberOfBlocks Number of blocks transferred
 * @return int 0 on success, negative errno on failure
 */
static int sdhc_stm32_ll_send_stop_cmd(sdhc_stm32_ll_handle_t *hsd,
				       uint32_t NumberOfBlocks)
{
	uint32_t errorstate;

	if (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND) || (NumberOfBlocks <= 1U)) {
		return 0;
	}

	if (hsd->SdCard.CardType == CARD_SECURED) {
		return 0;
	}

	/* Send stop transmission command */
	errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
	if (errorstate != SDMMC_ERROR_NONE) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= errorstate;
		hsd->State = SDMMC_STATE_READY;
		hsd->Context = SDMMC_CONTEXT_NONE;
		return -EIO;
	}

	return 0;
}

/**
 * @brief  Reads block(s) from a specified address in a card. The Data transfer
 *         is managed by polling mode.
 * @param  hsd: Pointer to SD handle
 * @param  pData: pointer to the buffer that will contain the received data
 * @param  BlockAdd: Block Address from where data is to be read
 * @param  NumberOfBlocks: Number of SD blocks to read
 * @param  Timeout: Specify timeout value
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_read_blocks(sdhc_stm32_ll_handle_t *hsd, uint8_t *pData,
			      uint32_t BlockAdd, uint32_t NumberOfBlocks,
			      uint32_t Timeout)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t dataremaining;
	uint32_t add = BlockAdd;
	uint8_t *tempbuff = pData;

	if (pData == NULL) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		hsd->ErrorCode |= SDMMC_ERROR_BUSY;
		return -EBUSY;
	}

	hsd->ErrorCode = SDMMC_ERROR_NONE;

	if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
		hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
		return -EINVAL;
	}

	hsd->State = SDMMC_STATE_BUSY;

	/* Initialize data control register */
	hsd->Instance->DCTRL = 0U;

	if (hsd->SdCard.CardType != CARD_SDHC_SDXC) {
		add *= BLOCKSIZE;
	}

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = NumberOfBlocks * BLOCKSIZE;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
	config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(hsd->Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Read block(s) in polling mode */
	if (NumberOfBlocks > 1U) {
		hsd->Context = SDMMC_CONTEXT_READ_MULTIPLE_BLOCK;

		/* Read Multi Block command */
		errorstate = SDMMC_CmdReadMultiBlock(hsd->Instance, add);
	} else {
		hsd->Context = SDMMC_CONTEXT_READ_SINGLE_BLOCK;
		/* Read Single Block command */
		errorstate = SDMMC_CmdReadSingleBlock(hsd->Instance, add);
	}

	if (errorstate != 0) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= errorstate;
		hsd->State = SDMMC_STATE_READY;
		hsd->Context = SDMMC_CONTEXT_NONE;
		return -EIO;
	}

	/* Poll on SDMMC flags */
	dataremaining = config.DataLength;
	int res = sdhc_stm32_ll_poll_read_transfer(hsd, &tempbuff, &dataremaining, tickstart,
						      Timeout);
	if (res != 0) {
		return res;
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Send stop transmission command in case of multiblock read */
	res = sdhc_stm32_ll_send_stop_cmd(hsd, NumberOfBlocks);
	if (res != 0) {
		return res;
	}

	/* Check for data transfer errors */
	errorstate = sdhc_stm32_ll_check_data_errors(hsd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->Context = SDMMC_CONTEXT_NONE;
		return sdhc_stm32_ll_handle_data_error(hsd, errorstate);
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief  Allows to write block(s) to a specified address in a card. The Data
 *         transfer is managed by polling mode.
 * @param  hsd: Pointer to SD handle
 * @param  pData: pointer to the buffer that will contain the data to transmit
 * @param  BlockAdd: Block Address where data will be written
 * @param  NumberOfBlocks: Number of SD blocks to write
 * @param  Timeout: Specify timeout value
 * @retval SDMMC status
 */
int sdhc_stm32_ll_write_blocks(sdhc_stm32_ll_handle_t *hsd, const uint8_t *pData,
			       uint32_t BlockAdd, uint32_t NumberOfBlocks,
			       uint32_t Timeout)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t dataremaining;
	uint32_t add = BlockAdd;
	const uint8_t *tempbuff = pData;

	if (pData == NULL) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		hsd->ErrorCode |= SDMMC_ERROR_BUSY;
		return -EBUSY;
	}

	hsd->ErrorCode = SDMMC_ERROR_NONE;

	if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
		hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
		return -EINVAL;
	}

	hsd->State = SDMMC_STATE_BUSY;

	/* Initialize data control register */
	hsd->Instance->DCTRL = 0U;

	if (hsd->SdCard.CardType != CARD_SDHC_SDXC) {
		add *= BLOCKSIZE;
	}

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = NumberOfBlocks * BLOCKSIZE;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
	config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
	config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(hsd->Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Write Blocks in Polling mode */
	if (NumberOfBlocks > 1U) {
		hsd->Context = SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK;
		errorstate = SDMMC_CmdWriteMultiBlock(hsd->Instance, add);
	} else {
		hsd->Context = SDMMC_CONTEXT_WRITE_SINGLE_BLOCK;
		errorstate = SDMMC_CmdWriteSingleBlock(hsd->Instance, add);
	}

	if (errorstate != 0) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= errorstate;
		hsd->State = SDMMC_STATE_READY;
		hsd->Context = SDMMC_CONTEXT_NONE;
		return -EIO;
	}

	/* Write block(s) in polling mode */
	dataremaining = config.DataLength;
	int res = sdhc_stm32_ll_poll_write_transfer(hsd, &tempbuff, &dataremaining, tickstart,
						       Timeout);
	if (res != 0) {
		return res;
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Send stop transmission command in case of multiblock write */
	res = sdhc_stm32_ll_send_stop_cmd(hsd, NumberOfBlocks);
	if (res != 0) {
		return res;
	}

	/* Check for data transfer errors */
	errorstate = sdhc_stm32_ll_check_data_errors(hsd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->Context = SDMMC_CONTEXT_NONE;
		return sdhc_stm32_ll_handle_data_error(hsd, errorstate);
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief  Writes block(s) to a specified address in a card. The Data transfer
 *         is managed by DMA mode.
 * @note   This API should be followed by a check on the card state through
 *         sdhc_stm32_ll_get_card_state().
 * @note   You could also check the DMA transfer process through the SD Tx
 *         interrupt event.
 * @param  hsd: Pointer to SD handle
 * @param  pData: Pointer to the buffer that will contain the data to transmit
 * @param  BlockAdd: Block Address where data will be written
 * @param  NumberOfBlocks: Number of blocks to write
 * @retval SDMMC status
 */
int sdhc_stm32_ll_write_blocks_dma(sdhc_stm32_ll_handle_t *hsd,
						   const uint8_t *pData, uint32_t BlockAdd,
						   uint32_t NumberOfBlocks)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t add = BlockAdd;

	if (pData == NULL) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return -EINVAL;
		}

		hsd->State = SDMMC_STATE_BUSY;

		/* Initialize data control register */
		hsd->Instance->DCTRL = 0U;

		hsd->pTxBuffPtr = pData;
		hsd->TxXferSize = BLOCKSIZE * NumberOfBlocks;

		if (hsd->SdCard.CardType != CARD_SDHC_SDXC) {
			add *= BLOCKSIZE;
		}

		/* Configure the SD DPSM (Data Path State Machine) */
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
		config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM = SDMMC_DPSM_DISABLE;
		(void)SDMMC_ConfigData(hsd->Instance, &config);

		__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		hsd->Instance->IDMABASE0 = (uint32_t)pData;
		hsd->Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

		/* Write Blocks in Polling mode */
		if (NumberOfBlocks > 1U) {
			hsd->Context = (SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK | SDMMC_CONTEXT_DMA);

			/* Write Multi Block command */
			errorstate = SDMMC_CmdWriteMultiBlock(hsd->Instance, add);
		} else {
			hsd->Context = (SDMMC_CONTEXT_WRITE_SINGLE_BLOCK | SDMMC_CONTEXT_DMA);

			/* Write Single Block command */
			errorstate = SDMMC_CmdWriteSingleBlock(hsd->Instance, add);
		}
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return -EIO;
		}

		/* Enable transfer interrupts */
		__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
						  SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND));

		return 0;
	} else {
		return -EBUSY;
	}
}

/**
 * @brief  Reads block(s) from a specified address in a card. The Data transfer
 *         is managed by DMA mode.
 * @note   This API should be followed by a check on the card state through
 *         sdhc_stm32_ll_get_card_state().
 * @note   You could also check the DMA transfer process through the SD Rx
 *         interrupt event.
 * @param  hsd: Pointer SD handle
 * @param  pData: Pointer to the buffer that will contain the received data
 * @param  BlockAdd: Block Address from where data is to be read
 * @param  NumberOfBlocks: Number of blocks to read.
 * @retval SDMMC status
 */
int sdhc_stm32_ll_read_blocks_dma(sdhc_stm32_ll_handle_t *hsd, uint8_t *pData,
						  uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t add = BlockAdd;

	if (pData == NULL) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return -EINVAL;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return -EINVAL;
		}

		hsd->State = SDMMC_STATE_BUSY;

		/* Initialize data control register */
		hsd->Instance->DCTRL = 0U;

		hsd->pRxBuffPtr = pData;
		hsd->RxXferSize = BLOCKSIZE * NumberOfBlocks;

		if (hsd->SdCard.CardType != CARD_SDHC_SDXC) {
			add *= BLOCKSIZE;
		}

		/* Configure the SD DPSM (Data Path State Machine) */
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
		config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM = SDMMC_DPSM_DISABLE;
		(void)SDMMC_ConfigData(hsd->Instance, &config);

		__SDMMC_CMDTRANS_ENABLE(hsd->Instance);
		hsd->Instance->IDMABASE0 = (uint32_t)pData;
		hsd->Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

		/* Read Blocks in DMA mode */
		if (NumberOfBlocks > 1U) {
			hsd->Context = (SDMMC_CONTEXT_READ_MULTIPLE_BLOCK | SDMMC_CONTEXT_DMA);

			/* Read Multi Block command */
			errorstate = SDMMC_CmdReadMultiBlock(hsd->Instance, add);
		} else {
			hsd->Context = (SDMMC_CONTEXT_READ_SINGLE_BLOCK | SDMMC_CONTEXT_DMA);

			/* Read Single Block command */
			errorstate = SDMMC_CmdReadSingleBlock(hsd->Instance, add);
		}
		if (errorstate != 0) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return -EIO;
		}

		/* Enable transfer interrupts */
		__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
						  SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND));

		return 0;
	} else {
		return -EBUSY;
	}
}

/**
 * @brief  Wrap up reading in non-blocking mode.
 * @param  hsd: pointer to a sdhc_stm32_ll_handle_t structure that contains
 *              the configuration information.
 * @retval None
 */
void sdhc_stm32_ll_read_it(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t count;
	uint32_t data;
	uint8_t *tmp;

	tmp = hsd->pRxBuffPtr;

	if (hsd->RxXferSize >= SDMMC_FIFO_SIZE) {
		/* Read data from SDMMC Rx FIFO */
		for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
			data = SDMMC_ReadFIFO(hsd->Instance);
			UNALIGNED_PUT(data, (uint32_t *)tmp);
			tmp += 4;
		}

		hsd->pRxBuffPtr = tmp;
		hsd->RxXferSize -= SDMMC_FIFO_SIZE;
	}
}

/**
 * @brief  Wrap up writing in non-blocking mode.
 * @param  hsd: pointer to a sdhc_stm32_ll_handle_t structure that contains
 *              the configuration information.
 * @retval None
 */
void sdhc_stm32_ll_write_it(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t count;
	uint32_t data;
	const uint8_t *tmp;

	tmp = hsd->pTxBuffPtr;

	if (hsd->TxXferSize >= SDMMC_FIFO_SIZE) {
		/* Write data to SDMMC Tx FIFO */
		for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
			data = UNALIGNED_GET((uint32_t *)tmp);
			tmp += 4;
			(void)SDMMC_WriteFIFO(hsd->Instance, &data);
		}

		hsd->pTxBuffPtr = tmp;
		hsd->TxXferSize -= SDMMC_FIFO_SIZE;
	}
}

/**
 * @brief Check if multi-block transfer context
 * @param context Current transfer context
 * @return true if multi-block read or write
 */
static inline bool sdhc_stm32_ll_is_multi_block(uint32_t context)
{
	return ((context & SDMMC_CONTEXT_READ_MULTIPLE_BLOCK) != 0U) ||
	       ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U);
}

/**
 * @brief Handle data end interrupt for IT mode
 * @param hsd Pointer to SD handle
 * @param context Current transfer context
 */
static void sdhc_stm32_ll_handle_dataend_it(sdhc_stm32_ll_handle_t *hsd, uint32_t context)
{
	uint32_t errorstate;

	if (sdhc_stm32_ll_is_multi_block(context)) {
		errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
		if (errorstate != SDMMC_ERROR_NONE) {
			hsd->ErrorCode |= errorstate;
		}
	}

	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;
	hsd->Context = SDMMC_CONTEXT_NONE;
}

/**
 * @brief Handle data end interrupt for DMA mode
 * @param hsd Pointer to SD handle
 * @param context Current transfer context
 */
static void sdhc_stm32_ll_handle_dataend_dma(sdhc_stm32_ll_handle_t *hsd, uint32_t context)
{
	uint32_t errorstate;

	hsd->Instance->DLEN = 0;
	hsd->Instance->DCTRL = 0;
	hsd->Instance->IDMACTRL = SDMMC_DISABLE_IDMA;

	if (sdhc_stm32_ll_is_multi_block(context)) {
		errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
		if (errorstate != SDMMC_ERROR_NONE) {
			hsd->ErrorCode |= errorstate;
		}
	}

	hsd->State = SDMMC_STATE_READY;
	hsd->Context = SDMMC_CONTEXT_NONE;
}

/**
 * @brief Handle data error interrupt
 * @param hsd Pointer to SD handle
 * @param context Current transfer context
 */
static void sdhc_stm32_ll_handle_data_error_irq(sdhc_stm32_ll_handle_t *hsd, uint32_t context)
{
	hsd->ErrorCode |= sdhc_stm32_ll_check_data_errors(hsd);
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
	sdhc_stm32_ll_disable_data_interrupts(hsd);

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);
	hsd->Instance->DCTRL |= SDMMC_DCTRL_FIFORST;
	hsd->Instance->CMD |= SDMMC_CMD_CMDSTOP;
	hsd->ErrorCode |= SDMMC_CmdStopTransfer(hsd->Instance);
	hsd->Instance->CMD &= ~(SDMMC_CMD_CMDSTOP);
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DABORT);

	if ((context & SDMMC_CONTEXT_IT) != 0U) {
		hsd->State = SDMMC_STATE_READY;
		hsd->Context = SDMMC_CONTEXT_NONE;
	} else if ((context & SDMMC_CONTEXT_DMA) != 0U) {
		if (hsd->ErrorCode != SDMMC_ERROR_NONE) {
			__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_IDMABTC);
			hsd->Instance->IDMACTRL = SDMMC_DISABLE_IDMA;
			hsd->State = SDMMC_STATE_READY;
		}
	} else {
		/* Nothing to do */
	}
}

/**
 * @brief  This function handles SD card interrupt request.
 * @param  hsd: Pointer to SD handle
 * @retval None
 */
void sdhc_stm32_ll_irq_handler(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t context = hsd->Context;

	/* Check for RX FIFO half full interrupt */
	if ((__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) != RESET) &&
	    ((context & SDMMC_CONTEXT_IT) != 0U)) {
		sdhc_stm32_ll_read_it(hsd);
		return;
	}

	/* Check for TX FIFO half empty interrupt */
	if ((__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) != RESET) &&
	    ((context & SDMMC_CONTEXT_IT) != 0U)) {
		sdhc_stm32_ll_write_it(hsd);
		return;
	}

	/* Check for data end interrupt */
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND) != RESET) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND);
		sdhc_stm32_ll_disable_data_interrupts(hsd);
		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		if ((context & SDMMC_CONTEXT_IT) != 0U) {
			sdhc_stm32_ll_handle_dataend_it(hsd, context);
		} else if ((context & SDMMC_CONTEXT_DMA) != 0U) {
			sdhc_stm32_ll_handle_dataend_dma(hsd, context);
		} else {
			/* Nothing to do */
		}
		return;
	}

	/* Check for data error interrupts */
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
						    SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR) !=
	    RESET) {
		sdhc_stm32_ll_handle_data_error_irq(hsd, context);
		return;
	}

	/* Check for IDMA buffer transfer complete interrupt */
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_IDMABTC) != RESET) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_IDMABTC);
		return;
	}
}

/**
 * @brief Helper function to convert block size to SDMMC_DataBlockSize
 * @param block_size Block size in bytes (must be power of 2, 1-16384)
 * @return SDMMC DataBlockSize value
 */
static uint32_t sdhc_stm32_ll_convert_block_size(uint32_t block_size)
{
	/* SDMMC_DATABLOCK_SIZE_<n>B is log2(n) shifted to DBLOCKSIZE bit position */
	__ASSERT(IS_POWER_OF_TWO(block_size) && block_size >= 1U && block_size <= 16384U,
		 "Invalid block size: %u", block_size);

	return LOG2(block_size) << SDMMC_DCTRL_DBLOCKSIZE_Pos;
}

/**
 * @brief Configure SDIO/SDMMC clock frequency
 *
 * This function configures the SDMMC clock divider to achieve the desired
 * clock frequency. It directly manipulates the CLKCR register.
 * @param hsd Pointer to SDIO LL handle
 * @param ClockSpeed Desired clock speed in Hz
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_config_freq(sdhc_stm32_ll_handle_t *hsd, uint32_t ClockSpeed)
{
	uint32_t ClockDiv;

	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));

	/* Check the handle parameter */
	if (hsd == NULL) {
		return -EINVAL;
	}

	/* Check if peripheral is in ready state */
	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	/* Verify clock frequency is set */
	if (hsd->sdmmc_clk == 0U) {
		LOG_ERR("SDMMC clock frequency not configured");
		return -EIO;
	}

	/* Calculate clock divider
	 * Formula: ClockDiv = PeripheralClock / (2 * DesiredClock)
	 * This is the STM32 SDMMC clock divider calculation
	 */
	ClockDiv = hsd->sdmmc_clk / (2U * ClockSpeed);

	/* Modify the CLKCR register to set the clock divider
	 * This is direct LL register manipulation
	 */
	stm32_reg_modify_bits(&hsd->Instance->CLKCR, SDMMC_CLKCR_CLKDIV, ClockDiv);

	LOG_DBG("Configured SDMMC clock: freq=%u Hz, div=%u", ClockSpeed, ClockDiv);

	return 0;
}

/**
 * @brief Get SDIO state
 * @param hsd Pointer to SDIO LL handle
 * @return Current state
 */
uint32_t sdhc_stm32_ll_get_state(const sdhc_stm32_ll_handle_t *hsd)
{
	if (hsd == NULL) {
		return SDMMC_STATE_ERROR;
	}
	return hsd->State;
}

/**
 * @brief Get SDIO error code
 * @param hsd Pointer to SDMMC handle
 * @return Error code
 */
uint32_t sdhc_stm32_ll_get_error(const sdhc_stm32_ll_handle_t *hsd)
{
	if (hsd == NULL) {
		return SDMMC_ERROR_INVALID_PARAMETER;
	}
	return hsd->ErrorCode;
}

/**
 * @brief Initialize SDIO peripheral
 *
 * This function initializes the SDMMC peripheral hardware registers.
 * It does NOT perform card initialization or enumeration - that is handled
 * by the Zephyr SD subsystem.
 * @param hsd Pointer to SDIO LL handle
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_init(sdhc_stm32_ll_handle_t *hsd)
{
	SDMMC_InitTypeDef Init;
	uint32_t sdmmc_clk;
	uint32_t init_freq = 400000U; /* 400 kHz initialization frequency */

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));
	assert_param(IS_SDMMC_CLOCK_EDGE(hsd->Init.ClockEdge));
	assert_param(IS_SDMMC_CLOCK_POWER_SAVE(hsd->Init.ClockPowerSave));
	assert_param(IS_SDMMC_BUS_WIDE(hsd->Init.BusWide));
	assert_param(IS_SDMMC_HARDWARE_FLOW_CONTROL(hsd->Init.HardwareFlowControl));
	assert_param(IS_SDMMC_CLKDIV(hsd->Init.ClockDiv));

	/* Check the handle parameter */
	if (hsd == NULL) {
		return -EINVAL;
	}

	/* Verify clock frequency is set */
	if (hsd->sdmmc_clk == 0U) {
		LOG_ERR("SDMMC clock frequency not configured");
		return -EINVAL;
	}

	/* If state is already initialized, we can just reconfigure */
	if (hsd->State == SDMMC_STATE_RESET) {
		/* Initialize with default values for first-time init */
		Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
		Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
		Init.BusWide = SDMMC_BUS_WIDE_1B;
		Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;

		/* Calculate initial clock divider for 400 kHz */
		Init.ClockDiv = hsd->sdmmc_clk / (2U * init_freq);

		/* Initialize SDMMC peripheral with default configuration */
		if (SDMMC_Init(hsd->Instance, Init) != 0) {
			return -EIO;
		}

		/* Set Power State to ON */
		SDMMC_PowerState_ON(hsd->Instance);

		/* Wait 74 cycles: required power up time before starting SDIO operations
		 * At 400 kHz, this is ~185 us. Wait 1 ms to be safe.
		 */
		sdmmc_clk = hsd->sdmmc_clk / (2U * Init.ClockDiv);
		k_msleep(1U + (74U * 1000U / (sdmmc_clk)));
	}

	hsd->State = SDMMC_STATE_PROGRAMMING;

	/* Configure the SDMMC with user parameters from handle */
	Init.ClockEdge = hsd->Init.ClockEdge;
	Init.ClockPowerSave = hsd->Init.ClockPowerSave;
	Init.BusWide = hsd->Init.BusWide;
	Init.HardwareFlowControl = hsd->Init.HardwareFlowControl;
	Init.ClockDiv = hsd->Init.ClockDiv;

#if defined(USE_SD_DIRPOL)
	/* Set Transceiver polarity */
	hsd->Instance->POWER |= SDMMC_POWER_DIRPOL;
#endif

	/* Apply user configuration to SDMMC peripheral */
	if (SDMMC_Init(hsd->Instance, Init) != 0) {
		return -EIO;
	}

	/* Clear error code and set state to ready */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->Context = SDMMC_CONTEXT_NONE;
	hsd->State = SDMMC_STATE_READY;

	LOG_DBG("SDMMC peripheral initialized successfully");

	return 0;
}

/**
 * @brief Deinitialize SDIO peripheral
 *
 * This function deinitializes the SDMMC peripheral hardware.
 * It powers off the peripheral and resets the state.
 * @param hsd Pointer to SDMMC handle
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_deinit(sdhc_stm32_ll_handle_t *hsd)
{
	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));

	/* Check the handle parameter */
	if (hsd == NULL) {
		return -EINVAL;
	}

	/* Set Power State to OFF */
	SDMMC_PowerState_OFF(hsd->Instance);

	/* Clear error code and reset state */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_RESET;

	LOG_DBG("SDMMC peripheral deinitialized");

	return 0;
}

/**
 * @brief Read direct (CMD52)
 *
 * This function performs a direct read operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param hsd Pointer to SDMMC handle
 * @param Argument Direct command argument structure
 * @param pData Pointer to receive data
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_direct(sdhc_stm32_ll_handle_t *hsd,
						   sdhc_stm32_sdio_direct_cmd_t *Argument,
						   uint8_t *pData)
{
	uint32_t cmd;
	uint32_t errorstate;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_RAW_FLAG(Argument->ReadAfterWrite));
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Construct CMD52 argument for read operation
	 * Bit 31: R/W flag (0 = read, 1 = write)
	 * Bits 30-28: Function number
	 * Bit 27: RAW flag (read after write)
	 * Bits 25-9: Register address
	 * Bits 7-0: Data (write) or stuff bits (read)
	 */
	cmd = (0U << 31U); /* Read operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->ReadAfterWrite) << 27U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= 0U; /* Stuff bits for read */

	/* Send CMD52 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteDirect(hsd->Instance, cmd, pData);

	if (errorstate != 0) {
		hsd->ErrorCode |= errorstate;
		/* Check if it's a critical error */
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			/* Clear all static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief Write direct (CMD52)
 *
 * This function performs a direct write operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param hsd Pointer to SDMMC handle
 * @param Argument Direct command argument structure
 * @param Data Data to write
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_direct(sdhc_stm32_ll_handle_t *hsd,
						    sdhc_stm32_sdio_direct_cmd_t *Argument,
						    uint8_t Data)
{
	uint32_t cmd;
	uint32_t errorstate;
	uint8_t write_data = Data;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(IS_SDIO_RAW_FLAG(Argument->ReadAfterWrite));
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL)) {
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Construct CMD52 argument for write operation
	 * Bit 31: R/W flag (0 = read, 1 = write)
	 * Bits 30-28: Function number
	 * Bit 27: RAW flag (read after write)
	 * Bits 25-9: Register address
	 * Bits 7-0: Data to write
	 */
	cmd = (1U << 31U); /* Write operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->ReadAfterWrite) << 27U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= Data;

	/* Send CMD52 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteDirect(hsd->Instance, cmd, &write_data);

	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		/* Check if it's a critical error */
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			/* Clear all static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief Read extended (CMD53 polling mode)
 *
 * This function performs an extended read operation using CMD53 in polling mode.
 * It configures the data path, sends CMD53, and polls the FIFO for data.
 * @param hsd Pointer to SDMMC handle
 * @param Argument Extended command argument structure
 * @param pData Pointer to receive buffer
 * @param Size_byte Number of bytes to read
 * @param Timeout_Ms Timeout in milliseconds
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_extended(sdhc_stm32_ll_handle_t *hsd,
						     sdhc_stm32_sdio_ext_cmd_t *Argument,
						     uint8_t *pData, uint32_t Size_byte,
						     uint32_t Timeout_Ms)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = k_uptime_get_32();
	uint32_t dataremaining;
	uint8_t *tempbuff = pData;
	uint32_t nbr_of_block;
	int status;

	/* Validate parameters */
	status = sdhc_stm32_ll_validate_sdio_ext_params(hsd, Argument, pData);
	if (status != 0) {
		return status;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Configure DPSM */
	sdhc_stm32_ll_config_sdio_dpsm(hsd, Argument, Size_byte, &nbr_of_block, &config, false);
	(void)SDMMC_ConfigData(hsd->Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Build and send CMD53 */
	cmd = sdhc_stm32_ll_build_cmd53_arg(Argument, nbr_of_block, Size_byte, false);
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (sdhc_stm32_ll_handle_cmd53_error(hsd, errorstate)) {
		return -EIO;
	}

	/* Poll on SDMMC flags and read data from FIFO */
	dataremaining = config.DataLength;

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) &&
		    (dataremaining >= 32U)) {
			sdhc_stm32_ll_read_fifo_block(hsd, &tempbuff);
			dataremaining -= 32U;
		} else if (dataremaining < 32U) {
			sdhc_stm32_ll_read_remaining_bytes(hsd, &tempbuff, &dataremaining);
		} else {
			/* FIFO not ready yet, waiting for data */
		}

		if (sdhc_stm32_ll_check_timeout(hsd, tickstart, Timeout_Ms)) {
			return -ETIMEDOUT;
		}
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Check for data transfer errors */
	errorstate = sdhc_stm32_ll_check_data_errors(hsd);
	if (errorstate != SDMMC_ERROR_NONE) {
		return sdhc_stm32_ll_handle_data_error(hsd, errorstate);
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief Write 32 bytes to FIFO using word-aligned buffer
 *
 * @param hsd Pointer to SDMMC handle
 * @param u32tempbuff Pointer to word-aligned buffer pointer
 */
static void sdhc_stm32_ll_write_fifo_words(sdhc_stm32_ll_handle_t *hsd, uint32_t **u32tempbuff)
{
	for (uint32_t regCount = 0U; regCount < 8U; regCount++) {
		hsd->Instance->FIFO = **u32tempbuff;
		(*u32tempbuff)++;
	}
}

/**
 * @brief Write extended (CMD53 polling mode)
 *
 * This function performs an extended write operation using CMD53 in polling mode.
 * It configures the data path, sends CMD53, and polls the FIFO to write data.
 * @param hsd Pointer to SDIO LL handle
 * @param Argument Extended command argument structure
 * @param pData Pointer to transmit buffer
 * @param Size_byte Number of bytes to write
 * @param Timeout_Ms Timeout in milliseconds
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_extended(sdhc_stm32_ll_handle_t *hsd,
						      sdhc_stm32_sdio_ext_cmd_t *Argument,
						      uint8_t *pData, uint32_t Size_byte,
						      uint32_t Timeout_Ms)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = k_uptime_get_32();
	uint32_t dataremaining;
	uint32_t *u32tempbuff = (uint32_t *)pData;
	uint32_t nbr_of_block;
	int status;

	/* Validate parameters */
	status = sdhc_stm32_ll_validate_sdio_ext_params(hsd, Argument, pData);
	if (status != 0) {
		return status;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Configure DPSM */
	sdhc_stm32_ll_config_sdio_dpsm(hsd, Argument, Size_byte, &nbr_of_block, &config, true);
	(void)SDMMC_ConfigData(hsd->Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Build and send CMD53 */
	cmd = sdhc_stm32_ll_build_cmd53_arg(Argument, nbr_of_block, Size_byte, true);
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (sdhc_stm32_ll_handle_cmd53_error(hsd, errorstate)) {
		return -EIO;
	}

	/* Poll on SDMMC flags and write data to FIFO */
	dataremaining = config.DataLength;

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) &&
		    (dataremaining >= 32U)) {
			sdhc_stm32_ll_write_fifo_words(hsd, &u32tempbuff);
			dataremaining -= 32U;
		} else if ((dataremaining < 32U) &&
			   (__SDMMC_GET_FLAG(hsd->Instance,
					     SDMMC_FLAG_TXFIFOHE | SDMMC_FLAG_TXFIFOE))) {
			uint8_t *u8buff = (uint8_t *)u32tempbuff;

			sdhc_stm32_ll_write_remaining_bytes(hsd, &u8buff, &dataremaining);
		} else {
			/* FIFO not ready yet, waiting to transmit */
		}

		if (sdhc_stm32_ll_check_timeout(hsd, tickstart, Timeout_Ms)) {
			return -ETIMEDOUT;
		}
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Check for data transfer errors */
	errorstate = sdhc_stm32_ll_check_data_errors(hsd);
	if (errorstate != SDMMC_ERROR_NONE) {
		return sdhc_stm32_ll_handle_data_error(hsd, errorstate);
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return 0;
}

/**
 * @brief Read extended DMA (CMD53 DMA mode)
 *
 * This function performs an extended read operation using CMD53 in DMA mode.
 * It configures the data path and DMA, sends CMD53, and enables interrupts.
 * The actual data transfer completion is handled by the interrupt handler.
 * @param hsd Pointer to SDIO LL handle
 * @param Argument Extended command argument structure
 * @param pData Pointer to receive buffer (must be DMA-capable)
 * @param Size_byte Number of bytes to read
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_extended_dma(sdhc_stm32_ll_handle_t *hsd,
							 sdhc_stm32_sdio_ext_cmd_t *Argument,
							 uint8_t *pData, uint32_t Size_byte)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t cmd;
	uint32_t nbr_of_block;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Initialize data control register */
	if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		hsd->Instance->DCTRL = 0U;
	}

	/* Compute number of blocks to receive */
	nbr_of_block = (Size_byte & ~(hsd->block_size & 1U)) >> __CLZ(__RBIT(hsd->block_size));

	/* Configure DMA (use single buffer mode) */
	hsd->Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
	hsd->Instance->IDMABASE0 = (uint32_t)pData;

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * hsd->block_size);
		config.DataBlockSize = sdhc_stm32_ll_convert_block_size(hsd->block_size);
	} else {
		config.DataLength = (Size_byte > 0U) ? Size_byte : 512U;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				      ? SDMMC_TRANSFER_MODE_BLOCK
				      : SDMMC_TRANSFER_MODE_SDIO;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(hsd->Instance, &config);

	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Construct CMD53 argument for read operation */
	cmd = (0U << 31U); /* Read operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= ((nbr_of_block == 0U) ? Size_byte : nbr_of_block) & 0x1FFU;

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			stm32_reg_modify_bits(&hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST,
					      SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR |
					  SDMMC_IT_DATAEND));

	return 0;
}

/**
 * @brief Write extended DMA (CMD53 DMA mode)
 *
 * This function performs an extended write operation using CMD53 in DMA mode.
 * It configures the data path and DMA, sends CMD53, and enables interrupts.
 * The actual data transfer completion is handled by the interrupt handler.
 * @param hsd Pointer to SDIO LL handle
 * @param Argument Extended command argument structure
 * @param pData Pointer to transmit buffer (must be DMA-capable)
 * @param Size_byte Number of bytes to write
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_extended_dma(sdhc_stm32_ll_handle_t *hsd,
							  sdhc_stm32_sdio_ext_cmd_t *Argument,
							  uint8_t *pData, uint32_t Size_byte)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t count;
	uint32_t errorstate;
	uint32_t nbr_of_block;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return -EBUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Initialize data control register */
	if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		hsd->Instance->DCTRL = 0U;
	}

	/* Compute number of blocks to send */
	nbr_of_block = (Size_byte & ~(hsd->block_size & 1U)) >> __CLZ(__RBIT(hsd->block_size));

	/* Configure DMA (use single buffer mode) */
	hsd->Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
	hsd->Instance->IDMABASE0 = (uint32_t)pData;

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * hsd->block_size);
		config.DataBlockSize = sdhc_stm32_ll_convert_block_size(hsd->block_size);
	} else {
		config.DataLength = (Size_byte > 512U) ? 512U : Size_byte;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
	config.TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				      ? SDMMC_TRANSFER_MODE_BLOCK
				      : SDMMC_TRANSFER_MODE_SDIO;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(hsd->Instance, &config);

	__SDMMC_CMDTRANS_ENABLE(hsd->Instance);

	/* Construct CMD53 argument for write operation */
	cmd = (1U << 31U); /* Write operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;

	if (nbr_of_block == 0U) {
		count = (Size_byte > 512U) ? 512U : Size_byte;
	} else {
		count = nbr_of_block;
	}

	cmd |= (count & 0x1FFU);

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			stm32_reg_modify_bits(&hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST,
					      SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return -EIO;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
					  SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND));

	return 0;
}

/**
 * @brief Reset SDIO card
 *
 * This function resets the SDIO card by writing to the RES bit in the CCCR register
 * using CMD52. This is the proper way to reset an I/O card or the I/O portion of
 * a combo card.
 * @param hsd Pointer to SDMMC handle
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_card_reset(sdhc_stm32_ll_handle_t *hsd)
{
	sdhc_stm32_sdio_direct_cmd_t cmd_arg;
	int status;
	uint8_t data = 0x08U; /* RES bit (bit 3) in CCCR register 6 */

	/* Check the parameters */
	assert_param(hsd != NULL);

	/* Check the handle parameter */
	if (hsd == NULL) {
		return -EINVAL;
	}

	/* Write to RES bit in CCCR register 6 to reset the card
	 * Register address: 0x06 (I/O Abort register in CCCR)
	 * Bit 3 (RES): Reset bit
	 */
	cmd_arg.IOFunctionNbr = 0;  /* Function 0 (common) */
	cmd_arg.Reg_Addr = 0x06U;   /* CCCR I/O Abort register */
	cmd_arg.ReadAfterWrite = 0; /* Write only */

	status = sdhc_stm32_ll_sdio_write_direct(hsd, &cmd_arg, data);
	if (status != 0) {
		LOG_ERR("Failed to reset SDIO card");
		return status;
	}

	hsd->State = SDMMC_STATE_RESET;

	LOG_DBG("SDIO card reset successful");

	return 0;
}

/**
 * @brief SDIO interrupt handler
 *
 * This function handles SDIO interrupts. It checks for various flags including
 * DATAEND, DCRCFAIL, DTIMEOUT, RXOVERR, and TXUNDERR.
 *
 * For DMA transfers, it disables DMA and clears the data path after completion.
 * This is a simplified version that doesn't handle multi-part transfers or callbacks.
 * Callbacks are handled by the Zephyr SDHC driver layer.
 * @param hsd Pointer to SDMMC handle
 */
void sdhc_stm32_ll_sdio_irq_handler(sdhc_stm32_ll_handle_t *hsd)
{
	uint32_t flags;
	uint32_t errorcode;

	if (hsd == NULL) {
		return;
	}

	/* Read interrupt flags */
	flags = READ_REG(hsd->Instance->STA);

	/* Check for data transfer completion */
	if (READ_BIT(flags, SDMMC_FLAG_DATAEND) != 0U) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND);

		hsd->State = SDMMC_STATE_READY;

		/* Disable all data transfer interrupts */
		sdhc_stm32_ll_disable_data_interrupts(hsd);
		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		/* If DMA was used, clean up DMA configuration */
		hsd->Instance->DLEN = 0;
		hsd->Instance->IDMACTRL = SDMMC_DISABLE_IDMA;

		/* Reset DCTRL register, preserving SDIOEN bit if it was set */
		if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
			hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
		} else {
			hsd->Instance->DCTRL = 0U;
		}

		LOG_DBG("SDIO data transfer completed");
	}

	/* Check for errors */
	errorcode = sdhc_stm32_ll_check_data_errors(hsd);
	if (errorcode != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorcode;

		/* Clear error flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
							  SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR);

		/* Disable interrupts */
		sdhc_stm32_ll_disable_data_interrupts(hsd);

		hsd->State = SDMMC_STATE_READY;

		LOG_ERR("SDIO transfer error: 0x%x", hsd->ErrorCode);
	}
}
