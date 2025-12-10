/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdhc_stm32_ll.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc_stm32_ll, CONFIG_SDHC_LOG_LEVEL);

/* Private validation macros for SDIO parameters */
#define IS_SDIO_RAW_FLAG(ReadAfterWrite) (((ReadAfterWrite) == 0U) || ((ReadAfterWrite) == 1U))
#define IS_SDIO_FUNCTION(FN)             (((FN) >= 0U) && ((FN) <= 7U))
#define IS_SDIO_SUPPORTED_BLOCK_SIZE(BLOCKSIZE)                                                    \
	(((BLOCKSIZE) == 1U) || ((BLOCKSIZE) == 2U) || ((BLOCKSIZE) == 4U) ||                      \
	 ((BLOCKSIZE) == 8U) || ((BLOCKSIZE) == 16U) || ((BLOCKSIZE) == 32U) ||                    \
	 ((BLOCKSIZE) == 64U) || ((BLOCKSIZE) == 128U) || ((BLOCKSIZE) == 256U) ||                 \
	 ((BLOCKSIZE) == 512U) || ((BLOCKSIZE) == 1024U) || ((BLOCKSIZE) == 2048U))

/**
 * @brief Tx Transfer completed callbacks
 * @param hsd: Pointer to SD handle
 * @retval None
 */
__weak void SDMMC_TxCpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_TxCpltCallback can be implemented in the user file
	 */
}

/**
 * @brief Rx Transfer completed callbacks
 * @param hsd: Pointer SD handle
 * @retval None
 */
__weak void SDMMC_RxCpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_RxCpltCallback can be implemented in the user file
	 */
}

/**
 * @brief SD error callbacks
 * @param hsd: Pointer SD handle
 * @retval None
 */
__weak void SDMMC_ErrorCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_ErrorCallback can be implemented in the user file
	 */
}

/**
 * @brief Read DMA Buffer 0 Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
__weak void SDMMCEx_Read_DMADoubleBuf0CpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMCEx_Read_DMADoubleBuf0CpltCallback can be implemented in the user file
	 */
}

/**
 * @brief Read DMA Buffer 1 Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
__weak void SDMMC_Read_DMADoubleBuf1CpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_Read_DMADoubleBuf1CpltCallback can be implemented in the user file
	 */
}

/**
 * @brief Write DMA Buffer 0 Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
__weak void SDMMC_Write_DMADoubleBuf0CpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_Write_DMADoubleBuf0CpltCallback can be implemented in the user file
	 */
}

/**
 * @brief Write DMA Buffer 1 Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
__weak void SDMMC_Write_DMADoubleBuf1CpltCallback(SDMMC_HandleTypeDef *hsd)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsd);

	/* NOTE : This function should not be modified, when the callback is needed,
	 * the SDMMC_Write_DMADoubleBuf1CpltCallback can be implemented in the user file
	 */
}

SDMMC_StatusTypeDef SDMMC_Erase(SDMMC_HandleTypeDef *hsd, uint32_t BlockStartAdd,
				uint32_t BlockEndAdd)
{
	uint32_t errorstate;
	uint32_t start_add = BlockStartAdd;
	uint32_t end_add = BlockEndAdd;

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if (end_add < start_add) {
			hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
			return SDMMC_ERROR;
		}

		if (end_add > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return SDMMC_ERROR;
		}

		hsd->State = SDMMC_STATE_BUSY;

		/* Check if the card command class supports erase command */
		if (((hsd->SdCard.Class) & SDMMC_CCCC_ERASE) == 0U) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}

		if ((SDMMC_GetResponse(hsd->Instance, SDMMC_RESP1) & SDMMC_CARD_LOCKED) ==
		    SDMMC_CARD_LOCKED) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_LOCK_UNLOCK_FAILED;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
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
			if (errorstate != SDMMC_ERROR_NONE) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= errorstate;
				hsd->State = SDMMC_STATE_READY;
				return SDMMC_ERROR;
			}

			/* Send CMD33 SD_ERASE_GRP_END with argument as addr  */
			errorstate = SDMMC_CmdSDEraseEndAdd(hsd->Instance, end_add);
			if (errorstate != SDMMC_ERROR_NONE) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= errorstate;
				hsd->State = SDMMC_STATE_READY;
				return SDMMC_ERROR;
			}
		}

		/* Send CMD38 ERASE */
		errorstate = SDMMC_CmdErase(hsd->Instance, 0UL);
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}

		hsd->State = SDMMC_STATE_READY;

		return SDMMC_OK;
	} else {
		return SDMMC_BUSY;
	}
}

uint32_t SDMMC_SendStatus(SDMMC_HandleTypeDef *hsd, uint32_t *pCardStatus)
{
	uint32_t errorstate;

	if (pCardStatus == NULL) {
		return SDMMC_ERROR_INVALID_PARAMETER;
	}

	/* Send Status command */
	errorstate = SDMMC_CmdSendStatus(hsd->Instance, (uint32_t)(hsd->SdCard.RelCardAdd));
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	/* Get SD card status */
	*pCardStatus = SDMMC_GetResponse(hsd->Instance, SDMMC_RESP1);

	return SDMMC_ERROR_NONE;
}

/**
 * @brief  Switches the SD card to High Speed mode.
 *         This API must be used after "Transfer State"
 * @note   This operation should be followed by the configuration
 *         of PLL to have SDMMCCK clock between 25 and 50 MHz
 * @param  hsd: SD handle
 * @param  SwitchSpeedMode: SD speed mode(SDMMC_SDR12_SWITCH_PATTERN, SDMMC_SDR25_SWITCH_PATTERN)
 * @retval SD Card error state
 */
uint32_t SDMMC_SwitchSpeed(SDMMC_HandleTypeDef *hsd, uint32_t SwitchSpeedMode)
{
	uint32_t errorstate = SDMMC_ERROR_NONE;
	SDMMC_DataInitTypeDef sdmmc_datainitstructure;
	uint32_t SD_hs[16] = {0};
	uint32_t count;
	uint32_t loop = 0;
	uint32_t Timeout = HAL_GetTick();

	if (hsd->SdCard.CardSpeed == CARD_NORMAL_SPEED) {
		/* Standard Speed Card <= 12.5Mhz  */
		return SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
	}

	if (hsd->SdCard.CardSpeed >= CARD_HIGH_SPEED) {
		/* Initialize the Data control register */
		hsd->Instance->DCTRL = 0;
		errorstate = SDMMC_CmdBlockLength(hsd->Instance, 64U);

		if (errorstate != SDMMC_ERROR_NONE) {
			return errorstate;
		}

		/* Configure the SD DPSM (Data Path State Machine) */
		sdmmc_datainitstructure.DataTimeOut = SDMMC_DATATIMEOUT;
		sdmmc_datainitstructure.DataLength = 64U;
		sdmmc_datainitstructure.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
		sdmmc_datainitstructure.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
		sdmmc_datainitstructure.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
		sdmmc_datainitstructure.DPSM = SDMMC_DPSM_ENABLE;

		(void)SDMMC_ConfigData(hsd->Instance, &sdmmc_datainitstructure);

		errorstate = SDMMC_CmdSwitch(hsd->Instance, SwitchSpeedMode);
		if (errorstate != SDMMC_ERROR_NONE) {
			return errorstate;
		}

		while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
								SDMMC_FLAG_DTIMEOUT |
								SDMMC_FLAG_DBCKEND |
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

			return errorstate;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL);

			errorstate = SDMMC_ERROR_DATA_CRC_FAIL;

			return errorstate;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR);

			errorstate = SDMMC_ERROR_RX_OVERRUN;

			return errorstate;
		}

		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

		/* Test if the switch mode HS is ok */
		if ((((uint8_t *)SD_hs)[13] & 2U) != 2U) {
			errorstate = SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}
	}

	return errorstate;
}

uint32_t SDMMC_FindSCR(SDMMC_HandleTypeDef *hsd, uint32_t *pSCR)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t index = 0U;
	uint32_t tempscr[2U] = {0UL, 0UL};
	uint32_t *scr = pSCR;

	/* Set Block Size To 8 Bytes */
	errorstate = SDMMC_CmdBlockLength(hsd->Instance, 8U);
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	/* Send CMD55 APP_CMD with argument as card's RCA */
	errorstate = SDMMC_CmdAppCommand(hsd->Instance, (uint32_t)((hsd->SdCard.RelCardAdd)));
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	config.DataTimeOut = SDMMC_DATATIMEOUT;
	config.DataLength = 8U;
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

SDMMC_CardStateTypeDef SDMMC_GetCardState(SDMMC_HandleTypeDef *hsd)
{
	uint32_t cardstate;
	uint32_t errorstate;
	uint32_t resp1 = 0;

	errorstate = SDMMC_SendStatus(hsd, &resp1);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
	}

	cardstate = ((resp1 >> 9U) & 0x0FU);

	return (SDMMC_CardStateTypeDef)cardstate;
}

/**
 * @brief  Initializes the SD Card.
 * @param  hsd: Pointer to SD handle
 * @note   This function initializes the SD card. It could be used when a card
 * re-initialization is needed.
 * @retval SDMMC status
 */
SDMMC_StatusTypeDef SDMMC_InitCard(SDMMC_HandleTypeDef *hsd)
{
	SDMMC_InitTypeDef Init;
	uint32_t sdmmc_clk;

	/* Default SDMMC peripheral configuration for SD card initialization */
	Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	Init.BusWide = SDMMC_BUS_WIDE_1B;
	Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;

	/* Init Clock should be less or equal to 400Khz*/
	sdmmc_clk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
	if (sdmmc_clk == 0U) {
		hsd->State = SDMMC_STATE_READY;
		hsd->ErrorCode = SDMMC_ERROR_INVALID_PARAMETER;
		return SDMMC_ERROR;
	}
	Init.ClockDiv = sdmmc_clk / (2U * SDMMC_INIT_FREQ);

#if defined(USE_SD_DIRPOL)
	/* Set Transceiver polarity */
	hsd->Instance->POWER |= SDMMC_POWER_DIRPOL;
#endif

	/* Initialize SDMMC peripheral interface with default configuration */
	(void)SDMMC_Init(hsd->Instance, Init);

	return SDMMC_OK;
}

SDMMC_StatusTypeDef SDMMC_DeInit(SDMMC_HandleTypeDef *hsd)
{
	/* Check the SD handle allocation */
	if (hsd == NULL) {
		return SDMMC_ERROR;
	}

	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));

	hsd->State = SDMMC_STATE_BUSY;

	/* Set SD power state to off */
	(void)SDMMC_PowerState_OFF(hsd->Instance);

	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_RESET;

	return SDMMC_OK;
}

/**
 * @brief  Reads block(s) from a specified address in a card. The Data transfer
 *         is managed by polling mode.
 * @param  hsd: Pointer to SD handle
 * @param  pData: pointer to the buffer that will contain the received data
 * @param  BlockAdd: Block Address from where data is to be read
 * @param  NumberOfBlocks: Number of SD blocks to read
 * @param  Timeout: Specify timeout value
 * @retval SDMMC status
 */
SDMMC_StatusTypeDef SDMMC_ReadBlocks(SDMMC_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd,
				     uint32_t NumberOfBlocks, uint32_t Timeout)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t count;
	uint32_t data;
	uint32_t dataremaining;
	uint32_t add = BlockAdd;
	uint8_t *tempbuff = pData;

	if (NULL == pData) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return SDMMC_ERROR;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return SDMMC_ERROR;
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
			hsd->Context = SD_CONTEXT_READ_SINGLE_BLOCK;

			/* Read Single Block command */
			errorstate = SDMMC_CmdReadSingleBlock(hsd->Instance, add);
		}
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Poll on SDMMC flags */
		dataremaining = config.DataLength;
		while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
								SDMMC_FLAG_DTIMEOUT |
								SDMMC_FLAG_DATAEND)) {
			if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) &&
			    (dataremaining >= SDMMC_FIFO_SIZE)) {
				/* Read data from SDMMC Rx FIFO */
				for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
					data = SDMMC_ReadFIFO(hsd->Instance);
					*tempbuff = (uint8_t)(data & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 8U) & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 16U) & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 24U) & 0xFFU);
					tempbuff++;
				}
				dataremaining -= SDMMC_FIFO_SIZE;
			}

			if (((HAL_GetTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= SDMMC_ERROR_TIMEOUT;
				hsd->State = SDMMC_STATE_READY;
				hsd->Context = SDMMC_CONTEXT_NONE;
				return SDMMC_TIMEOUT;
			}
		}
		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		/* Send stop transmission command in case of multiblock read */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND) && (NumberOfBlocks > 1U)) {
			if (hsd->SdCard.CardType != CARD_SECURED) {
				/* Send stop transmission command */
				errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
				if (errorstate != SDMMC_ERROR_NONE) {
					/* Clear all the static flags */
					__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
					hsd->ErrorCode |= errorstate;
					hsd->State = SDMMC_STATE_READY;
					hsd->Context = SDMMC_CONTEXT_NONE;
					return SDMMC_ERROR;
				}
			}
		}

		/* Get error state */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

		hsd->State = SDMMC_STATE_READY;

		return SDMMC_OK;
	}
	hsd->ErrorCode |= SDMMC_ERROR_BUSY;
	return SDMMC_ERROR;
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
SDMMC_StatusTypeDef SDMMC_WriteBlocks(SDMMC_HandleTypeDef *hsd, const uint8_t *pData,
				      uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = HAL_GetTick();
	uint32_t count;
	uint32_t data;
	uint32_t dataremaining;
	uint32_t add = BlockAdd;
	const uint8_t *tempbuff = pData;

	if (NULL == pData) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return SDMMC_ERROR;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return SDMMC_ERROR;
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

			/* Write Multi Block command */
			errorstate = SDMMC_CmdWriteMultiBlock(hsd->Instance, add);
		} else {
			hsd->Context = SDMMC_CONTEXT_WRITE_SINGLE_BLOCK;

			/* Write Single Block command */
			errorstate = SDMMC_CmdWriteSingleBlock(hsd->Instance, add);
		}
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Write block(s) in polling mode */
		dataremaining = config.DataLength;
		while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL |
								SDMMC_FLAG_DTIMEOUT |
								SDMMC_FLAG_DATAEND)) {
			if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) &&
			    (dataremaining >= SDMMC_FIFO_SIZE)) {
				/* Write data to SDMMC Tx FIFO */
				for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
					data = (uint32_t)(*tempbuff);
					tempbuff++;
					data |= ((uint32_t)(*tempbuff) << 8U);
					tempbuff++;
					data |= ((uint32_t)(*tempbuff) << 16U);
					tempbuff++;
					data |= ((uint32_t)(*tempbuff) << 24U);
					tempbuff++;
					(void)SDMMC_WriteFIFO(hsd->Instance, &data);
				}
				dataremaining -= SDMMC_FIFO_SIZE;
			}

			if (((HAL_GetTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
				/* Clear all the static flags */
				__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
				hsd->ErrorCode |= errorstate;
				hsd->State = SDMMC_STATE_READY;
				hsd->Context = SDMMC_CONTEXT_NONE;
				return SDMMC_TIMEOUT;
			}
		}
		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		/* Send stop transmission command in case of multiblock write */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND) && (NumberOfBlocks > 1U)) {
			if (hsd->SdCard.CardType != CARD_SECURED) {
				/* Send stop transmission command */
				errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
				if (errorstate != SDMMC_ERROR_NONE) {
					/* Clear all the static flags */
					__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
					hsd->ErrorCode |= errorstate;
					hsd->State = SDMMC_STATE_READY;
					hsd->Context = SDMMC_CONTEXT_NONE;
					return SDMMC_ERROR;
				}
			}
		}

		/* Get error state */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR)) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_TX_UNDERRUN;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

		hsd->State = SDMMC_STATE_READY;

		return SDMMC_OK;
	}
	hsd->ErrorCode |= SDMMC_ERROR_BUSY;
	return SDMMC_ERROR;
}

/**
 * @brief  Writes block(s) to a specified address in a card. The Data transfer
 *         is managed by DMA mode.
 * @note   This API should be followed by a check on the card state through
 *         SDMMC_GetCardState().
 * @note   You could also check the DMA transfer process through the SD Tx
 *         interrupt event.
 * @param  hsd: Pointer to SD handle
 * @param  pData: Pointer to the buffer that will contain the data to transmit
 * @param  BlockAdd: Block Address where data will be written
 * @param  NumberOfBlocks: Number of blocks to write
 * @retval SDMMC status
 */
SDMMC_StatusTypeDef SDMMC_WriteBlocks_DMA(SDMMC_HandleTypeDef *hsd, const uint8_t *pData,
					  uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t add = BlockAdd;

	if (NULL == pData) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return SDMMC_ERROR;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return SDMMC_ERROR;
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
			hsd->Context = (SD_CONTEXT_WRITE_MULTIPLE_BLOCK | SD_CONTEXT_DMA);

			/* Write Multi Block command */
			errorstate = SDMMC_CmdWriteMultiBlock(hsd->Instance, add);
		} else {
			hsd->Context = (SD_CONTEXT_WRITE_SINGLE_BLOCK | SD_CONTEXT_DMA);

			/* Write Single Block command */
			errorstate = SDMMC_CmdWriteSingleBlock(hsd->Instance, add);
		}
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SD_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Enable transfer interrupts */
		__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
						  SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND));

		return SDMMC_OK;
	} else {
		return SDMMC_BUSY;
	}
}

/**
 * @brief  Reads block(s) from a specified address in a card. The Data transfer
 *         is managed by DMA mode.
 * @note   This API should be followed by a check on the card state through
 *         SDMMC_GetCardState().
 * @note   You could also check the DMA transfer process through the SD Rx
 *         interrupt event.
 * @param  hsd: Pointer SD handle
 * @param  pData: Pointer to the buffer that will contain the received data
 * @param  BlockAdd: Block Address from where data is to be read
 * @param  NumberOfBlocks: Number of blocks to read.
 * @retval SDMMC status
 */
SDMMC_StatusTypeDef SDMMC_ReadBlocks_DMA(SDMMC_HandleTypeDef *hsd, uint8_t *pData,
					 uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t add = BlockAdd;

	if (NULL == pData) {
		hsd->ErrorCode |= SDMMC_ERROR_INVALID_PARAMETER;
		return SDMMC_ERROR;
	}

	if (hsd->State == SDMMC_STATE_READY) {
		hsd->ErrorCode = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (hsd->SdCard.LogBlockNbr)) {
			hsd->ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return SDMMC_ERROR;
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
			hsd->Context = (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA);

			/* Read Multi Block command */
			errorstate = SDMMC_CmdReadMultiBlock(hsd->Instance, add);
		} else {
			hsd->Context = (SD_CONTEXT_READ_SINGLE_BLOCK | SD_CONTEXT_DMA);

			/* Read Single Block command */
			errorstate = SDMMC_CmdReadSingleBlock(hsd->Instance, add);
		}
		if (errorstate != SDMMC_ERROR_NONE) {
			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= errorstate;
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SD_CONTEXT_NONE;
			return SDMMC_ERROR;
		}

		/* Enable transfer interrupts */
		__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
						  SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND));

		return SDMMC_OK;
	} else {
		return SDMMC_BUSY;
	}
}

/**
 * @brief  Wrap up reading in non-blocking mode.
 * @param  hsd: pointer to a SDMMC_HandleTypeDef structure that contains
 *              the configuration information.
 * @retval None
 */
void SD_Read_IT(SDMMC_HandleTypeDef *hsd)
{
	uint32_t count;
	uint32_t data;
	uint8_t *tmp;

	tmp = hsd->pRxBuffPtr;

	if (hsd->RxXferSize >= SDMMC_FIFO_SIZE) {
		/* Read data from SDMMC Rx FIFO */
		for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
			data = SDMMC_ReadFIFO(hsd->Instance);
			*tmp = (uint8_t)(data & 0xFFU);
			tmp++;
			*tmp = (uint8_t)((data >> 8U) & 0xFFU);
			tmp++;
			*tmp = (uint8_t)((data >> 16U) & 0xFFU);
			tmp++;
			*tmp = (uint8_t)((data >> 24U) & 0xFFU);
			tmp++;
		}

		hsd->pRxBuffPtr = tmp;
		hsd->RxXferSize -= SDMMC_FIFO_SIZE;
	}
}

/**
 * @brief  Wrap up writing in non-blocking mode.
 * @param  hsd: pointer to a SDMMC_HandleTypeDef structure that contains
 *              the configuration information.
 * @retval None
 */
void SD_Write_IT(SDMMC_HandleTypeDef *hsd)
{
	uint32_t count;
	uint32_t data;
	const uint8_t *tmp;

	tmp = hsd->pTxBuffPtr;

	if (hsd->TxXferSize >= SDMMC_FIFO_SIZE) {
		/* Write data to SDMMC Tx FIFO */
		for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++) {
			data = (uint32_t)(*tmp);
			tmp++;
			data |= ((uint32_t)(*tmp) << 8U);
			tmp++;
			data |= ((uint32_t)(*tmp) << 16U);
			tmp++;
			data |= ((uint32_t)(*tmp) << 24U);
			tmp++;
			(void)SDMMC_WriteFIFO(hsd->Instance, &data);
		}

		hsd->pTxBuffPtr = tmp;
		hsd->TxXferSize -= SDMMC_FIFO_SIZE;
	}
}

/**
 * @brief  This function handles SD card interrupt request.
 * @param  hsd: Pointer to SD handle
 * @retval None
 */
void SDMMC_IRQHandler(SDMMC_HandleTypeDef *hsd)
{
	uint32_t errorstate;
	uint32_t context = hsd->Context;

	/* Check for SDMMC interrupt flags */
	if ((__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) != RESET) &&
	    ((context & SDMMC_CONTEXT_IT) != 0U)) {
		SD_Read_IT(hsd);
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND) != RESET) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DATAEND);

		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL |
							  SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
							  SDMMC_IT_RXOVERR | SDMMC_IT_TXFIFOHE |
							  SDMMC_IT_RXFIFOHF);

		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_IDMABTC);
		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		if ((context & SDMMC_CONTEXT_IT) != 0U) {
			if (((context & SDMMC_CONTEXT_READ_MULTIPLE_BLOCK) != 0U) ||
			    ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U)) {
				errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
				if (errorstate != SDMMC_ERROR_NONE) {
					hsd->ErrorCode |= errorstate;
					SDMMC_ErrorCallback(hsd);
				}
			}

			/* Clear all the static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			if (((context & SDMMC_CONTEXT_READ_SINGLE_BLOCK) != 0U) ||
			    ((context & SDMMC_CONTEXT_READ_MULTIPLE_BLOCK) != 0U)) {
				SDMMC_RxCpltCallback(hsd);
			} else {
				SDMMC_TxCpltCallback(hsd);
			}
		} else if ((context & SDMMC_CONTEXT_DMA) != 0U) {
			hsd->Instance->DLEN = 0;
			hsd->Instance->DCTRL = 0;
			hsd->Instance->IDMACTRL = SDMMC_DISABLE_IDMA;

			/* Stop Transfer for Write Multi blocks or Read Multi blocks */
			if (((context & SDMMC_CONTEXT_READ_MULTIPLE_BLOCK) != 0U) ||
			    ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U)) {
				errorstate = SDMMC_CmdStopTransfer(hsd->Instance);
				if (errorstate != SDMMC_ERROR_NONE) {
					hsd->ErrorCode |= errorstate;
					SDMMC_ErrorCallback(hsd);
				}
			}

			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			if (((context & SDMMC_CONTEXT_WRITE_SINGLE_BLOCK) != 0U) ||
			    ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U)) {
				SDMMC_TxCpltCallback(hsd);
			}
			if (((context & SDMMC_CONTEXT_READ_SINGLE_BLOCK) != 0U) ||
			    ((context & SDMMC_CONTEXT_READ_MULTIPLE_BLOCK) != 0U)) {
				SDMMC_RxCpltCallback(hsd);
			}
		} else {
			/* Nothing to do */
		}
	} else if ((__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) != RESET) &&
		   ((context & SDMMC_CONTEXT_IT) != 0U)) {
		SD_Write_IT(hsd);
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
							   SDMMC_FLAG_RXOVERR |
							   SDMMC_FLAG_TXUNDERR) != RESET) {
		/* Set Error code */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_IT_DCRCFAIL) != RESET) {
			hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_IT_DTIMEOUT) != RESET) {
			hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_IT_RXOVERR) != RESET) {
			hsd->ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_IT_TXUNDERR) != RESET) {
			hsd->ErrorCode |= SDMMC_ERROR_TX_UNDERRUN;
		}

		/* Clear All flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

		/* Disable all interrupts */
		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL |
							  SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
							  SDMMC_IT_RXOVERR);

		__SDMMC_CMDTRANS_DISABLE(hsd->Instance);
		hsd->Instance->DCTRL |= SDMMC_DCTRL_FIFORST;
		hsd->Instance->CMD |= SDMMC_CMD_CMDSTOP;
		hsd->ErrorCode |= SDMMC_CmdStopTransfer(hsd->Instance);
		hsd->Instance->CMD &= ~(SDMMC_CMD_CMDSTOP);
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DABORT);

		if ((context & SDMMC_CONTEXT_IT) != 0U) {
			/* Set the SD state to ready to be able to start again the process */
			hsd->State = SDMMC_STATE_READY;
			hsd->Context = SDMMC_CONTEXT_NONE;
			SDMMC_ErrorCallback(hsd);
		} else if ((context & SDMMC_CONTEXT_DMA) != 0U) {
			if (hsd->ErrorCode != SDMMC_ERROR_NONE) {
				/* Disable Internal DMA */
				__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_IDMABTC);
				hsd->Instance->IDMACTRL = SDMMC_DISABLE_IDMA;

				/* Set the SD state to ready to be able to start again the process
				 */
				hsd->State = SDMMC_STATE_READY;
				SDMMC_ErrorCallback(hsd);
			}
		} else {
			/* Nothing to do */
		}
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_IDMABTC) != RESET) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_IDMABTC);
		if (READ_BIT(hsd->Instance->IDMACTRL, SDMMC_IDMA_IDMABACT) == 0U) {
			/* Current buffer is buffer0, Transfer complete for buffer1 */
			if ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U) {
				SDMMC_Write_DMADoubleBuf1CpltCallback(hsd);
			} else { /* SD_CONTEXT_READ_MULTIPLE_BLOCK */
				SDMMC_Read_DMADoubleBuf1CpltCallback(hsd);
			}
		} else { /* SD_DMA_BUFFER1 */
			/* Current buffer is buffer1, Transfer complete for buffer0 */
			if ((context & SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0U) {
				SDMMC_Write_DMADoubleBuf0CpltCallback(hsd);
			} else { /* SD_CONTEXT_READ_MULTIPLE_BLOCK */
				SDMMCEx_Read_DMADoubleBuf0CpltCallback(hsd);
			}
		}
	} else {
		/* Nothing to do */
	}
}

/**
 * @brief Helper function to convert block size to SDMMC_DataBlockSize
 * @param hsd Pointer to SDMMC handle
 * @param block_size Block size in bytes
 * @return SDMMC DataBlockSize value
 */
static uint32_t SDMMC_LL_Convert_Block_Size(SDMMC_HandleTypeDef *hsd, uint32_t block_size)
{
	uint32_t datablock_size = SDMMC_DATABLOCK_SIZE_1B;

	/* Find the matching SDMMC_DATABLOCK_SIZE_* constant */
	switch (block_size) {
	case 1:
		datablock_size = SDMMC_DATABLOCK_SIZE_1B;
		break;
	case 2:
		datablock_size = SDMMC_DATABLOCK_SIZE_2B;
		break;
	case 4:
		datablock_size = SDMMC_DATABLOCK_SIZE_4B;
		break;
	case 8:
		datablock_size = SDMMC_DATABLOCK_SIZE_8B;
		break;
	case 16:
		datablock_size = SDMMC_DATABLOCK_SIZE_16B;
		break;
	case 32:
		datablock_size = SDMMC_DATABLOCK_SIZE_32B;
		break;
	case 64:
		datablock_size = SDMMC_DATABLOCK_SIZE_64B;
		break;
	case 128:
		datablock_size = SDMMC_DATABLOCK_SIZE_128B;
		break;
	case 256:
		datablock_size = SDMMC_DATABLOCK_SIZE_256B;
		break;
	case 512:
		datablock_size = SDMMC_DATABLOCK_SIZE_512B;
		break;
	case 1024:
		datablock_size = SDMMC_DATABLOCK_SIZE_1024B;
		break;
	case 2048:
		datablock_size = SDMMC_DATABLOCK_SIZE_2048B;
		break;
	case 4096:
		datablock_size = SDMMC_DATABLOCK_SIZE_4096B;
		break;
	case 8192:
		datablock_size = SDMMC_DATABLOCK_SIZE_8192B;
		break;
	case 16384:
		datablock_size = SDMMC_DATABLOCK_SIZE_16384B;
		break;
	default:
		/* Default to 512 bytes if invalid */
		datablock_size = SDMMC_DATABLOCK_SIZE_512B;
		break;
	}

	return datablock_size;
}

/**
 * @brief Get the SDMMC peripheral clock frequency
 * @return Clock frequency in Hz
 */
static uint32_t SDMMC_LL_GetClockFreq(void)
{
/* Get SDMMC peripheral clock frequency */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
#else
	/* Add support for other STM32 series as needed */
	return 0;
#endif
}

/**
 * @brief Configure SDIO/SDMMC clock frequency
 *
 * This function configures the SDMMC clock divider to achieve the desired
 * clock frequency. It directly manipulates the CLKCR register.
 * @param hsd Pointer to SDIO LL handle
 * @param ClockSpeed Desired clock speed in Hz
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDMMC_LL_ConfigFrequency(SDMMC_HandleTypeDef *hsd, uint32_t ClockSpeed)
{
	uint32_t ClockDiv;

	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));

	/* Check the handle parameter */
	if (hsd == NULL) {
		return SDMMC_ERROR;
	}

	/* Check if peripheral is in ready state */
	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_ERROR;
	}

	/* Calculate clock divider
	 * Formula: ClockDiv = PeripheralClock / (2 * DesiredClock)
	 * This is the STM32 SDMMC clock divider calculation
	 */
	ClockDiv = SDMMC_LL_GetClockFreq() / (2U * ClockSpeed);

	/* Modify the CLKCR register to set the clock divider
	 * This is direct LL register manipulation
	 */
	MODIFY_REG(hsd->Instance->CLKCR, SDMMC_CLKCR_CLKDIV, ClockDiv);

	LOG_DBG("Configured SDMMC clock: freq=%u Hz, div=%u", ClockSpeed, ClockDiv);

	return SDMMC_OK;
}

/**
 * @brief Get SDIO state
 * @param hsd Pointer to SDIO LL handle
 * @return Current state
 */
uint32_t SDMMC_LL_GetState(const SDMMC_HandleTypeDef *hsd)
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
uint32_t SDMMC_LL_GetError(const SDMMC_HandleTypeDef *hsd)
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
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDMMC_LL_Init(SDMMC_HandleTypeDef *hsd)
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
		return SDMMC_ERROR;
	}

	/* If state is already initialized, we can just reconfigure */
	if (hsd->State == SDMMC_STATE_RESET) {
		if (IS_ENABLED(CONFIG_SDMMC_STACK)) {
			hsd->Lock = SDMMC_UNLOCKED;
		} else {
			/* Initialize with default values for first-time init */
			Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
			Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
			Init.BusWide = SDMMC_BUS_WIDE_1B;
			Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;

			/* Calculate initial clock divider for 400 kHz */
			sdmmc_clk = SDMMC_LL_GetClockFreq();
			if (sdmmc_clk == 0U) {
				hsd->ErrorCode = SDMMC_ERROR_INVALID_PARAMETER;
				return SDMMC_ERROR;
			}
			Init.ClockDiv = sdmmc_clk / (2U * init_freq);

			/* Initialize SDMMC peripheral with default configuration */
			if (SDMMC_Init(hsd->Instance, Init) != (HAL_StatusTypeDef)SDMMC_OK) {
				return SDMMC_ERROR;
			}

			/* Set Power State to ON */
			SDMMC_PowerState_ON(hsd->Instance);

			/* Wait 74 cycles: required power up time before starting SDIO operations
			* At 400 kHz, this is ~185 us. Wait 1 ms to be safe.
			*/
			sdmmc_clk = sdmmc_clk / (2U * Init.ClockDiv);
			k_msleep(1U + (74U * 1000U / (sdmmc_clk)));
		}
	}

	hsd->State = SDMMC_STATE_PROGRAMMING;

	/* Configure the SDMMC with user parameters from handle */
	Init.ClockEdge = hsd->Init.ClockEdge;
	Init.ClockPowerSave = hsd->Init.ClockPowerSave;
	Init.BusWide = hsd->Init.BusWide;
	Init.HardwareFlowControl = hsd->Init.HardwareFlowControl;
	Init.ClockDiv = hsd->Init.ClockDiv;

	if (IS_ENABLED(CONFIG_SDMMC_STACK)) {
		/* Init Clock should be less or equal to 400Khz*/
		sdmmc_clk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
		if (sdmmc_clk == 0U) {
			hsd->State = SDMMC_STATE_READY;
			hsd->ErrorCode = SDMMC_ERROR_INVALID_PARAMETER;
			return SDMMC_ERROR;
		}
		Init.ClockDiv = sdmmc_clk / (2U * SDMMC_INIT_FREQ);

		#if defined(USE_SD_DIRPOL)
			/* Set Transceiver polarity */
			hsd->Instance->POWER |= SDMMC_POWER_DIRPOL;
		#endif
	}
	/* Apply user configuration to SDMMC peripheral */
	if (SDMMC_Init(hsd->Instance, Init) != (HAL_StatusTypeDef)SDMMC_OK) {
		return SDMMC_ERROR;
	}

	/* Clear error code and set state to ready */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->Context = SDMMC_CONTEXT_NONE;
	hsd->State = SDMMC_STATE_READY;

	LOG_DBG("SDMMC peripheral initialized successfully");

	return SDMMC_OK;
}

/**
 * @brief Deinitialize SDIO peripheral
 *
 * This function deinitializes the SDMMC peripheral hardware.
 * It powers off the peripheral and resets the state.
 * @param hsd Pointer to SDMMC handle
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDMMC_LL_DeInit(SDMMC_HandleTypeDef *hsd)
{
	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(hsd->Instance));

	/* Check the handle parameter */
	if (hsd == NULL) {
		return SDMMC_ERROR;
	}

	/* Set Power State to OFF */
	SDMMC_PowerState_OFF(hsd->Instance);

	/* Clear error code and reset state */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_RESET;

	LOG_DBG("SDMMC peripheral deinitialized");

	return SDMMC_OK;
}

/**
 * @brief Read direct (CMD52)
 *
 * This function performs a direct read operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param hsd Pointer to SDMMC handle
 * @param Argument Direct command argument structure
 * @param pData Pointer to receive data
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_ReadDirect(SDMMC_HandleTypeDef *hsd,
				       SDIO_LL_DirectCmd_TypeDef *Argument, uint8_t *pData)
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
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
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

	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		/* Check if it's a critical error */
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			/* Clear all static flags */
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return SDMMC_OK;
}

/**
 * @brief Write direct (CMD52)
 *
 * This function performs a direct write operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param hsd Pointer to SDMMC handle
 * @param Argument Direct command argument structure
 * @param Data Data to write
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_WriteDirect(SDMMC_HandleTypeDef *hsd,
					SDIO_LL_DirectCmd_TypeDef *Argument, uint8_t Data)
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
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
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
			return SDMMC_ERROR;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return SDMMC_OK;
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
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_ReadExtended(SDMMC_HandleTypeDef *hsd,
					 SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
					 uint32_t Size_byte, uint32_t Timeout_Ms)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = k_uptime_get_32();
	uint32_t regCount;
	uint8_t byteCount;
	uint32_t data;
	uint32_t dataremaining;
	uint8_t *tempbuff = pData;
	uint32_t nbr_of_block;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Compute number of blocks to receive */
	nbr_of_block = (Size_byte & ~(hsd->block_size & 1U)) >> __CLZ(__RBIT(hsd->block_size));

	/* Initialize data control register */
	if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		hsd->Instance->DCTRL = 0U;
	}

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * hsd->block_size);
		config.DataBlockSize = SDMMC_LL_Convert_Block_Size(hsd, hsd->block_size);
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
			MODIFY_REG(hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}
	}

	/* Poll on SDMMC flags and read data from FIFO */
	dataremaining = config.DataLength;

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOHF) &&
		    (dataremaining >= 32U)) {
			/* Read 32 bytes from FIFO (8 x 4-byte words) */
			for (regCount = 0U; regCount < 8U; regCount++) {
				data = SDMMC_ReadFIFO(hsd->Instance);
				*tempbuff = (uint8_t)(data & 0xFFU);
				tempbuff++;
				*tempbuff = (uint8_t)((data >> 8U) & 0xFFU);
				tempbuff++;
				*tempbuff = (uint8_t)((data >> 16U) & 0xFFU);
				tempbuff++;
				*tempbuff = (uint8_t)((data >> 24U) & 0xFFU);
				tempbuff++;
			}
			dataremaining -= 32U;
		} else if (dataremaining < 32U) {
			/* Read remaining bytes */
			while ((dataremaining > 0U) &&
			       !(__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXFIFOE))) {
				data = SDMMC_ReadFIFO(hsd->Instance);
				for (byteCount = 0U; byteCount < 4U; byteCount++) {
					if (dataremaining > 0U) {
						*tempbuff = (uint8_t)((data >> (byteCount * 8U)) &
								      0xFFU);
						tempbuff++;
						dataremaining--;
					}
				}
			}
		}

		/* Check timeout */
		if ((k_uptime_get_32() - tickstart) >= Timeout_Ms) {
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_TIMEOUT;
		}
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Check for errors */
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return SDMMC_OK;
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
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_WriteExtended(SDMMC_HandleTypeDef *hsd,
					  SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
					  uint32_t Size_byte, uint32_t Timeout_Ms)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = k_uptime_get_32();
	uint32_t regCount;
	uint8_t byteCount;
	uint32_t data;
	uint32_t dataremaining;
	uint32_t *u32tempbuff = (uint32_t *)pData;
	uint32_t nbr_of_block;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
	}

	/* Set state to busy */
	hsd->ErrorCode = SDMMC_ERROR_NONE;
	hsd->State = SDMMC_STATE_BUSY;

	/* Compute number of blocks to send */
	nbr_of_block = (Size_byte & ~(hsd->block_size & 1U)) >> __CLZ(__RBIT(hsd->block_size));

	/* Initialize data control register */
	if ((hsd->Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		hsd->Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		hsd->Instance->DCTRL = 0U;
	}

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * hsd->block_size);
		config.DataBlockSize = SDMMC_LL_Convert_Block_Size(hsd, hsd->block_size);
	} else {
		config.DataLength = (Size_byte > 0U) ? Size_byte : 512U;
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
	cmd |= ((nbr_of_block == 0U) ? Size_byte : nbr_of_block) & 0x1FFU;

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			MODIFY_REG(hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}
	}

	/* Poll on SDMMC flags and write data to FIFO */
	dataremaining = config.DataLength;

	while (!__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL |
							SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXFIFOHE) &&
		    (dataremaining >= 32U)) {
			/* Write 32 bytes to FIFO (8 x 4-byte words) */
			for (regCount = 0U; regCount < 8U; regCount++) {
				hsd->Instance->FIFO = *u32tempbuff;
				u32tempbuff++;
			}
			dataremaining -= 32U;
		} else if ((dataremaining < 32U) &&
			   (__SDMMC_GET_FLAG(hsd->Instance,
					     SDMMC_FLAG_TXFIFOHE | SDMMC_FLAG_TXFIFOE))) {
			/* Write remaining bytes */
			uint8_t *u8buff = (uint8_t *)u32tempbuff;

			while (dataremaining > 0U) {
				data = 0U;
				for (byteCount = 0U; (byteCount < 4U) && (dataremaining > 0U);
				     byteCount++) {
					data |= ((uint32_t)(*u8buff) << (byteCount << 3U));
					u8buff++;
					dataremaining--;
				}
				hsd->Instance->FIFO = data;
			}
		}

		/* Check timeout */
		if ((k_uptime_get_32() - tickstart) >= Timeout_Ms) {
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			hsd->ErrorCode |= SDMMC_ERROR_TIMEOUT;
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_TIMEOUT;
		}
	}

	__SDMMC_CMDTRANS_DISABLE(hsd->Instance);

	/* Check for errors */
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	} else if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR)) {
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
		hsd->ErrorCode |= SDMMC_ERROR_TX_UNDERRUN;
		hsd->State = SDMMC_STATE_READY;
		return SDMMC_ERROR;
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);

	hsd->State = SDMMC_STATE_READY;

	return SDMMC_OK;
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
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_ReadExtended_DMA(SDMMC_HandleTypeDef *hsd,
					     SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
					     uint32_t Size_byte)
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
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
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
		config.DataBlockSize = SDMMC_LL_Convert_Block_Size(hsd, hsd->block_size);
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
			MODIFY_REG(hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR |
					  SDMMC_IT_DATAEND));

	return SDMMC_OK;
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
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_WriteExtended_DMA(SDMMC_HandleTypeDef *hsd,
					      SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
					      uint32_t Size_byte)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t nbr_of_block;

	/* Check the parameters */
	assert_param(hsd != NULL);
	assert_param(Argument != NULL);
	assert_param(pData != NULL);
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Check parameters */
	if ((hsd == NULL) || (Argument == NULL) || (pData == NULL)) {
		return SDMMC_ERROR;
	}

	if (hsd->State != SDMMC_STATE_READY) {
		return SDMMC_BUSY;
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
		config.DataBlockSize = SDMMC_LL_Convert_Block_Size(hsd, hsd->block_size);
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
	cmd |= ((nbr_of_block == 0U) ? ((Size_byte > 512U) ? 512U : Size_byte) : nbr_of_block) &
	       0x1FFU;

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(hsd->Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		hsd->ErrorCode |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			MODIFY_REG(hsd->Instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_STATIC_DATA_FLAGS);
			hsd->State = SDMMC_STATE_READY;
			return SDMMC_ERROR;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(hsd->Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
					  SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND));

	return SDMMC_OK;
}

/**
 * @brief Reset SDIO card
 *
 * This function resets the SDIO card by writing to the RES bit in the CCCR register
 * using CMD52. This is the proper way to reset an I/O card or the I/O portion of
 * a combo card.
 * @param hsd Pointer to SDMMC handle
 * @return SDMMC_StatusTypeDef Status
 */
SDMMC_StatusTypeDef SDIO_LL_CardReset(SDMMC_HandleTypeDef *hsd)
{
	SDIO_LL_DirectCmd_TypeDef cmd_arg;
	SDMMC_StatusTypeDef status;
	uint8_t data = 0x08U; /* RES bit (bit 3) in CCCR register 6 */

	/* Check the parameters */
	assert_param(hsd != NULL);

	/* Check the handle parameter */
	if (hsd == NULL) {
		return SDMMC_ERROR;
	}

	/* Write to RES bit in CCCR register 6 to reset the card
	 * Register address: 0x06 (I/O Abort register in CCCR)
	 * Bit 3 (RES): Reset bit
	 */
	cmd_arg.IOFunctionNbr = 0;  /* Function 0 (common) */
	cmd_arg.Reg_Addr = 0x06U;   /* CCCR I/O Abort register */
	cmd_arg.ReadAfterWrite = 0; /* Write only */

	status = SDIO_LL_WriteDirect(hsd, &cmd_arg, data);
	if (status != SDMMC_OK) {
		LOG_ERR("Failed to reset SDIO card");
		return status;
	}

	hsd->State = SDMMC_STATE_RESET;

	LOG_DBG("SDIO card reset successful");

	return SDMMC_OK;
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
void SDIO_IRQHandler(SDMMC_HandleTypeDef *hsd)
{
	uint32_t flags;

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
		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL |
							  SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
							  SDMMC_IT_RXOVERR | SDMMC_IT_TXFIFOHE |
							  SDMMC_IT_RXFIFOHF);

		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_IDMABTC);
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
	if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
						    SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR)) {
		/* Update error code based on flags */
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL)) {
			hsd->ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_DTIMEOUT)) {
			hsd->ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_RXOVERR)) {
			hsd->ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
		}
		if (__SDMMC_GET_FLAG(hsd->Instance, SDMMC_FLAG_TXUNDERR)) {
			hsd->ErrorCode |= SDMMC_ERROR_TX_UNDERRUN;
		}

		/* Clear error flags */
		__SDMMC_CLEAR_FLAG(hsd->Instance, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT |
							  SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR);

		/* Disable interrupts */
		__SDMMC_DISABLE_IT(hsd->Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL |
							  SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
							  SDMMC_IT_RXOVERR);

		hsd->State = SDMMC_STATE_READY;

		LOG_ERR("SDIO transfer error: 0x%x", hsd->ErrorCode);
	}
}
