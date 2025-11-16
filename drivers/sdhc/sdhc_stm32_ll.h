/*
* Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/**
 * @brief SDMMC card state enumeration
 */
typedef uint32_t SDMMC_CardStateTypeDef;

#define SDMMC_CARD_READY          0x00000001U  /*!< Card state is ready                     */
#define SDMMC_CARD_IDENTIFICATION 0x00000002U  /*!< Card is in identification state         */
#define SDMMC_CARD_STANDBY        0x00000003U  /*!< Card is in standby state                */
#define SDMMC_CARD_TRANSFER       0x00000004U  /*!< Card is in transfer state               */
#define SDMMC_CARD_SENDING        0x00000005U  /*!< Card is sending an operation            */
#define SDMMC_CARD_RECEIVING      0x00000006U  /*!< Card is receiving operation information */
#define SDMMC_CARD_PROGRAMMING    0x00000007U  /*!< Card is in programming state            */
#define SDMMC_CARD_DISCONNECTED   0x00000008U  /*!< Card is disconnected                    */
#define SDMMC_CARD_ERROR          0x000000FFU  /*!< Card response error                     */

/**
 * @brief SDMMC internal driver state
 */
typedef enum {
    SDMMC_STATE_RESET       = 0x00000000U,  /*!< Not yet initialized or disabled  */
    SDMMC_STATE_READY       = 0x00000001U,  /*!< Initialized and ready for use    */
    SDMMC_STATE_TIMEOUT     = 0x00000002U,  /*!< Timeout state                    */
    SDMMC_STATE_BUSY        = 0x00000003U,  /*!< Process ongoing                  */
    SDMMC_STATE_PROGRAMMING = 0x00000004U,  /*!< Programming state                */
    SDMMC_STATE_RECEIVING   = 0x00000005U,  /*!< Receiving state                  */
    SDMMC_STATE_TRANSFER    = 0x00000006U,  /*!< Transfer state                   */
    SDMMC_STATE_ERROR       = 0x0000000FU   /*!< Error state                      */
} SDMMC_StateTypeDef;

/**
 * @brief SDMMC return status codes
 */
typedef enum {
    SDMMC_OK      = 0x00,  /*!< Operation successful */
    SDMMC_ERROR   = 0x01,  /*!< Generic error        */
    SDMMC_BUSY    = 0x02,  /*!< Busy state           */
    SDMMC_TIMEOUT = 0x03   /*!< Timeout occurred     */
} SDMMC_StatusTypeDef;

/**
 * @brief SDMMC card status
 */
typedef struct {
    __IO uint8_t  DataBusWidth;           /*!< Currently defined data bus width                 */
    __IO uint8_t  SecuredMode;            /*!< Card secured mode status                         */
    __IO uint16_t CardType;               /*!< Card type information                            */
    __IO uint32_t ProtectedAreaSize;      /*!< Capacity of protected area                       */
    __IO uint8_t  SpeedClass;             /*!< Speed class of the card                          */
    __IO uint8_t  PerformanceMove;        /*!< Performance move indicator                       */
    __IO uint8_t  AllocationUnitSize;     /*!< Allocation unit size                             */
    __IO uint16_t EraseSize;              /*!< Number of AUs erased per operation               */
    __IO uint8_t  EraseTimeout;           /*!< Timeout for AU erase                             */
    __IO uint8_t  EraseOffset;            /*!< Erase offset                                     */
    __IO uint8_t  UhsSpeedGrade;          /*!< UHS speed grade                                  */
    __IO uint8_t  UhsAllocationUnitSize;  /*!< UHS card allocation unit size                    */
    __IO uint8_t  VideoSpeedClass;        /*!< Video speed class                                */
} SDMMC_CardStatusTypeDef;

/**
 * @brief SDMMC lock type definition
 */
typedef enum {
    SDMMC_UNLOCKED = 0x00,
    SDMMC_LOCKED   = 0x01
} SDMMC_LockTypeDef;


/* ================================================================
 * SDMMC CONTEXT FLAGS
 * ================================================================ */

#define SDMMC_CONTEXT_NONE                 0x00000000U  /*!< No context                           */
#define SDMMC_CONTEXT_READ_SINGLE_BLOCK    0x00000001U  /*!< Read single block operation          */
#define SDMMC_CONTEXT_READ_MULTIPLE_BLOCK  0x00000002U  /*!< Read multiple blocks operation       */
#define SDMMC_CONTEXT_WRITE_SINGLE_BLOCK   0x00000010U  /*!< Write single block operation         */
#define SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK 0x00000020U  /*!< Write multiple blocks operation      */
#define SDMMC_CONTEXT_IT                   0x00000008U  /*!< Process in interrupt mode            */
#define SDMMC_CONTEXT_DMA                  0x00000080U  /*!< Process in DMA mode                  */


/*
 * SDMMC Timing and block definitions
 */

#define SD_TIMEOUT               0x00100000U
#define SDHC_CMD_TIMEOUT         K_MSEC(200)
#define BLOCKSIZE                512U       /*!< Block size in bytes                   */

#define SDMMC_INIT_FREQ          400000U    /*!< Initialization phase: max 400 kHz     */
#define SD_NORMAL_SPEED_FREQ     25000000U  /*!< Normal speed phase: max 25 MHz        */
#define SD_HIGH_SPEED_FREQ       50000000U  /*!< High speed phase: max 50 MHz          */


#define SD_InitTypeDef      SDMMC_InitTypeDef
#define SD_TypeDef          SDMMC_TypeDef

/**
  * @brief  SD Card Information Structure definition
  */
typedef struct
{
  uint32_t CardType;                     /*!< Specifies the card Type                         */

  uint32_t CardVersion;                  /*!< Specifies the card version                      */

  uint32_t Class;                        /*!< Specifies the class of the card class           */

  uint32_t RelCardAdd;                   /*!< Specifies the Relative Card Address             */

  uint32_t BlockNbr;                     /*!< Specifies the Card Capacity in blocks           */

  uint32_t BlockSize;                    /*!< Specifies one block size in bytes               */

  uint32_t LogBlockNbr;                  /*!< Specifies the Card logical Capacity in blocks   */

  uint32_t LogBlockSize;                 /*!< Specifies logical block size in bytes           */

  uint32_t CardSpeed;                    /*!< Specifies the card Speed                        */

} SDMMC_CardInfoTypeDef;

/**
  * @brief  SD handle Structure definition
  */
typedef struct
{
  SD_TypeDef                   *Instance;        /*!< SD registers base address           */

  SD_InitTypeDef               Init;             /*!< SD required parameters              */

  HAL_LockTypeDef              Lock;             /*!< SD locking object                   */

  const uint8_t                *pTxBuffPtr;      /*!< Pointer to SD Tx transfer Buffer    */

  uint32_t                     TxXferSize;       /*!< SD Tx Transfer size                 */

  uint8_t                      *pRxBuffPtr;      /*!< Pointer to SD Rx transfer Buffer    */

  uint32_t                     RxXferSize;       /*!< SD Rx Transfer size                 */

  __IO uint32_t                Context;          /*!< SD transfer context                 */

  __IO SDMMC_StateTypeDef     State;            /*!< SD card State                       */

  __IO uint32_t                ErrorCode;        /*!< SD Card Error codes                 */

  SDMMC_CardInfoTypeDef       SdCard;           /*!< SD Card information                 */

  uint32_t                     CSD[4];           /*!< SD card specific data table         */

  uint32_t                     CID[4];           /*!< SD card identification number table */
} SDMMC_HandleTypeDef;


/* Functions' Prototypes*/
SDMMC_CardStateTypeDef SDMMC_GetCardState(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_WriteBlocks_DMA(SDMMC_HandleTypeDef *hsd, const uint8_t *pData, uint32_t BlockAdd,
                                         uint32_t NumberOfBlocks);
SDMMC_StatusTypeDef SDMMC_WriteBlocks(SDMMC_HandleTypeDef *hsd, const uint8_t *pData, uint32_t BlockAdd,
                                     uint32_t NumberOfBlocks, uint32_t Timeout);
SDMMC_StatusTypeDef SDMMC_ReadBlocks_DMA(SDMMC_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd,
                                        uint32_t NumberOfBlocks);
SDMMC_StatusTypeDef SDMMC_ReadBlocks(SDMMC_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks,
                                    uint32_t Timeout);
SDMMC_StatusTypeDef SDMMC_Erase(SDMMC_HandleTypeDef *hsd, uint32_t BlockStartAdd, uint32_t BlockEndAdd);
uint32_t SDMMC_SwitchSpeed(SDMMC_HandleTypeDef *hsd, uint32_t SwitchSpeedMode);
uint32_t SDMMC_FindSCR(SDMMC_HandleTypeDef *hsd, uint32_t *pSCR);
SDMMC_StatusTypeDef SDMMC_DeInit(SDMMC_HandleTypeDef *hsd);
SDMMC_StatusTypeDef SDMMC_Interface_Init(SDMMC_HandleTypeDef *hsd);
void SDMMC_IRQHandler(SDMMC_HandleTypeDef *hsd);
