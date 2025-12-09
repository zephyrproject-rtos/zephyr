/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SDHC_STM32_LL_H__
#define __SDHC_STM32_LL_H__

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/**
 * @brief SDMMC card state enumeration
 */
typedef uint32_t SDMMC_CardStateTypeDef;
#define SDMMC_CARD_READY                   0x00000001U /* Card state is ready */
#define SDMMC_CARD_IDENTIFICATION          0x00000002U /* Card is in identification state */
#define SDMMC_CARD_STANDBY                 0x00000003U /* Card is in standby state */
#define SDMMC_CARD_TRANSFER                0x00000004U /* Card is in transfer state */
#define SDMMC_CARD_SENDING                 0x00000005U /* Card is sending an operation */
#define SDMMC_CARD_RECEIVING               0x00000006U /* Card is receiving operation information */
#define SDMMC_CARD_PROGRAMMING             0x00000007U /* Card is in programming state */
#define SDMMC_CARD_DISCONNECTED            0x00000008U /* Card is disconnected */
#define SDMMC_CARD_ERROR                   0x000000FFU /* Card response error */

/**
 * @brief SDMMC context flags
 */
#define SDMMC_CONTEXT_NONE                 0x00000000U /* No context */
#define SDMMC_CONTEXT_READ_SINGLE_BLOCK    0x00000001U /* Read single block operation */
#define SDMMC_CONTEXT_READ_MULTIPLE_BLOCK  0x00000002U /* Read multiple blocks operation */
#define SDMMC_CONTEXT_WRITE_SINGLE_BLOCK   0x00000010U /* Write single block operation */
#define SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK 0x00000020U /* Write multiple blocks operation */
#define SDMMC_CONTEXT_IT                   0x00000008U /* Process in interrupt mode */
#define SDMMC_CONTEXT_DMA                  0x00000080U /* Process in DMA mode */

/*
 * @brief SDMMC Timing and block definitions
 */
#define SD_TIMEOUT                         0x00100000U
#define SDHC_CMD_TIMEOUT                   K_MSEC(200)
#define SDMMC_INIT_FREQ                    400000U /* Initialization phase: max 400 kHz */

/**
 * @brief SDMMC state definitions
 */
#define SDMMC_STATE_RESET                  0x00000000U /* SDIO not yet initialized or disabled */
#define SDMMC_STATE_READY                  0x00000001U /* SDIO initialized and ready for use */
#define SDMMC_STATE_BUSY                   0x00000002U /* SDIO operation ongoing */
#define SDMMC_STATE_ERROR                  0x00000004U /* SDIO error state */
#define SDMMC_STATE_PROGRAMMING            0x00000004U /* SDIO error state */

/**
 * @brief SDMMC return status codes
 */
typedef enum {
	SDMMC_OK      = 0x00,  /* Operation successful */
	SDMMC_ERROR   = 0x01,  /* Generic error */
	SDMMC_BUSY    = 0x02,  /* Busy state */
	SDMMC_TIMEOUT = 0x03   /* Timeout occurred */
} SDMMC_StatusTypeDef;

/**
 * @brief SDMMC card status
 */
typedef struct {
	__IO uint8_t  DataBusWidth;           /* Currently defined data bus width */
	__IO uint8_t  SecuredMode;            /* Card secured mode status */
	__IO uint16_t CardType;               /* Card type information */
	__IO uint32_t ProtectedAreaSize;      /* Capacity of protected area */
	__IO uint8_t  SpeedClass;             /* Speed class of the card */
	__IO uint8_t  PerformanceMove;        /* Performance move indicator */
	__IO uint8_t  AllocationUnitSize;     /* Allocation unit size */
	__IO uint16_t EraseSize;              /* Number of AUs erased per operation */
	__IO uint8_t  EraseTimeout;           /* Timeout for AU erase */
	__IO uint8_t  EraseOffset;            /* Erase offset */
	__IO uint8_t  UhsSpeedGrade;          /* UHS speed grade */
	__IO uint8_t  UhsAllocationUnitSize;  /* UHS card allocation unit size */
	__IO uint8_t  VideoSpeedClass;        /* Video speed class */
} SDMMC_CardStatusTypeDef;

/**
 * @brief SDMMC lock type definition
 */
typedef enum {
SDMMC_UNLOCKED = 0x00,
SDMMC_LOCKED   = 0x01
} SDMMC_LockTypeDef;

/**
 * @brief SD Card Information Structure definition
 */
typedef struct {
	uint32_t CardType;      /* Specifies the card Type */

	uint32_t CardVersion;   /* Specifies the card version */

	uint32_t Class;         /* Specifies the class of the card class */

	uint32_t RelCardAdd;    /* Specifies the Relative Card Address */

	uint32_t BlockNbr;      /* Specifies the Card Capacity in blocks */

	uint32_t BlockSize;     /* Specifies one block size in bytes */

	uint32_t LogBlockNbr;   /* Specifies the Card logical Capacity in blocks */

	uint32_t LogBlockSize;  /* Specifies logical block size in bytes */

	uint32_t CardSpeed;     /* Specifies the card Speed */

} SDMMC_CardInfoTypeDef;

/**
 * @brief SD handle Structure definition
 */
typedef struct {
	SDMMC_TypeDef         *Instance;     /* SD registers base address */

	SDMMC_InitTypeDef      Init;         /* SD required parameters */

	SDMMC_LockTypeDef      Lock;         /* SD locking object */

	const uint8_t         *pTxBuffPtr;   /* Pointer to SD Tx transfer Buffer */

	uint32_t               TxXferSize;   /* SD Tx Transfer size */

	uint8_t               *pRxBuffPtr;   /* Pointer to SD Rx transfer Buffer */

	uint32_t               RxXferSize;   /* SD Rx Transfer size */

	__IO uint32_t          Context;      /* SD transfer context */

	__IO uint32_t          State;        /* SD card State */

	__IO uint32_t          ErrorCode;    /* SD Card Error codes */

	SDMMC_CardInfoTypeDef  SdCard;       /* SD Card information */

	uint32_t               CSD[4];       /* SD card specific data table */

	uint32_t               CID[4];       /* SD card identification number table */

	uint32_t               block_size;   /* Block size for SDIO data transfer */
} SDMMC_HandleTypeDef;

/**
 * @brief SDIO Direct Command argument structure (for CMD52)
 */
typedef struct {
	uint32_t Reg_Addr;          /* Register address to read/write */
	uint32_t ReadAfterWrite;    /* Read after write flag */
	uint32_t IOFunctionNbr;     /* IO function number */
} SDIO_LL_DirectCmd_TypeDef;

/**
 * @brief SDIO Extended Command argument structure (for CMD53)
 */
typedef struct {
	uint32_t Reg_Addr;          /* Register address */
	uint32_t IOFunctionNbr;     /* IO function number */
	uint32_t Block_Mode;        /* Block or byte mode */
	uint32_t OpCode;            /* Operation code (increment/fixed address) */
} SDIO_LL_ExtendedCmd_TypeDef;

/**
 * @brief SD/MMC Functions Prototypes
 */
SDMMC_CardStateTypeDef SDMMC_GetCardState(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_WriteBlocks_DMA(SDMMC_HandleTypeDef *hsd, const uint8_t *pData,
										  uint32_t BlockAdd, uint32_t NumberOfBlocks);
SDMMC_StatusTypeDef SDMMC_WriteBlocks(SDMMC_HandleTypeDef *hsd, const uint8_t *pData,
									  uint32_t BlockAdd, uint32_t NumberOfBlocks,
									  uint32_t Timeout);
SDMMC_StatusTypeDef SDMMC_ReadBlocks_DMA(SDMMC_HandleTypeDef *hsd, uint8_t *pData,
										 uint32_t BlockAdd, uint32_t NumberOfBlocks);
SDMMC_StatusTypeDef SDMMC_ReadBlocks(SDMMC_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd,
									 uint32_t NumberOfBlocks, uint32_t Timeout);
SDMMC_StatusTypeDef SDMMC_Erase(SDMMC_HandleTypeDef *hsd, uint32_t BlockStartAdd,
								uint32_t BlockEndAdd);
uint32_t SDMMC_SwitchSpeed(SDMMC_HandleTypeDef *hsd, uint32_t SwitchSpeedMode);
uint32_t SDMMC_FindSCR(SDMMC_HandleTypeDef *hsd, uint32_t *pSCR);
SDMMC_StatusTypeDef SDMMC_DeInit(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_Interface_Init(SDMMC_HandleTypeDef *hsd);
void SDMMC_IRQHandler(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_LL_ConfigFrequency(SDMMC_HandleTypeDef *hsd, uint32_t ClockSpeed);
SDMMC_StatusTypeDef SDMMC_LL_Init(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_LL_DeInit(SDMMC_HandleTypeDef *hsd);
uint32_t SDMMC_LL_GetState(const SDMMC_HandleTypeDef *hsd);
uint32_t SDMMC_LL_GetError(const SDMMC_HandleTypeDef *hsd);

/**
 * @brief SDIO Functions Prototypes
 */
SDMMC_StatusTypeDef SDIO_LL_ReadDirect(SDMMC_HandleTypeDef *hsd,
									   SDIO_LL_DirectCmd_TypeDef *Argument, uint8_t *pData);
SDMMC_StatusTypeDef SDIO_LL_WriteDirect(SDMMC_HandleTypeDef *hsd,
										SDIO_LL_DirectCmd_TypeDef *Argument, uint8_t Data);
SDMMC_StatusTypeDef SDIO_LL_ReadExtended(SDMMC_HandleTypeDef *hsd,
										 SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
										 uint32_t Size_byte, uint32_t Timeout_Ms);
SDMMC_StatusTypeDef SDIO_LL_WriteExtended(SDMMC_HandleTypeDef *hsd,
										  SDIO_LL_ExtendedCmd_TypeDef *Argument, uint8_t *pData,
										  uint32_t Size_byte, uint32_t Timeout_Ms);
SDMMC_StatusTypeDef SDIO_LL_ReadExtended_DMA(SDMMC_HandleTypeDef *hsd,
											 SDIO_LL_ExtendedCmd_TypeDef *Argument,
											 uint8_t *pData, uint32_t Size_byte);
SDMMC_StatusTypeDef SDIO_LL_WriteExtended_DMA(SDMMC_HandleTypeDef *hsd,
											  SDIO_LL_ExtendedCmd_TypeDef *Argument,
											  uint8_t *pData, uint32_t Size_byte);
SDMMC_StatusTypeDef SDIO_LL_CardReset(SDMMC_HandleTypeDef *hsd);
void SDIO_IRQHandler(SDMMC_HandleTypeDef *hsd);

#endif /*__SDHC_STM32_LL_H__*/
