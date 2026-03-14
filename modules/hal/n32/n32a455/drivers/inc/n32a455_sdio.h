/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_sdio.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_SDIO_H__
#define __N32A455_SDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup SDIO
 * @{
 */

/** @addtogroup SDIO_Exported_Types
 * @{
 */

typedef struct
{
    uint32_t ClkEdge; /*!< Specifies the clock transition on which the bit capture is made.
                                  This parameter can be a value of @ref SDIO_Clock_Edge */

    uint32_t ClkBypass; /*!< Specifies whether the SDIO Clock divider bypass is
                                    enabled or disabled.
                                    This parameter can be a value of @ref SDIO_Clock_Bypass */

    uint32_t ClkPwrSave; /*!< Specifies whether SDIO Clock output is enabled or
                                       disabled when the bus is idle.
                                       This parameter can be a value of @ref SDIO_Clock_Power_Save */

    uint32_t BusWidth; /*!< Specifies the SDIO bus width.
                                This parameter can be a value of @ref SDIO_Bus_Wide */

    uint32_t HardwareClkCtrl; /*!< Specifies whether the SDIO hardware flow control is enabled or disabled.
                                            This parameter can be a value of @ref SDIO_Hardware_Flow_Control */

    uint8_t ClkDiv; /*!< Specifies the clock frequency of the SDIO controller.
                                This parameter can be a value between 0x00 and 0xFF. */

} SDIO_InitType;

typedef struct
{
    uint32_t CmdArgument; /*!< Specifies the SDIO command argument which is sent
                                 to a card as part of a command message. If a command
                                 contains an argument, it must be loaded into this register
                                 before writing the command to the command register */

    uint32_t CmdIndex; /*!< Specifies the SDIO command index. It must be lower than 0x40. */

    uint32_t ResponseType; /*!< Specifies the SDIO response type.
                                 This parameter can be a value of @ref SDIO_Response_Type */

    uint32_t WaitType; /*!< Specifies whether SDIO wait-for-interrupt request is enabled or disabled.
                             This parameter can be a value of @ref SDIO_Wait_Interrupt_State */

    uint32_t CPSMConfig; /*!< Specifies whether SDIO Command path state machine (CPSM)
                             is enabled or disabled.
                             This parameter can be a value of @ref SDIO_CPSM_State */
} SDIO_CmdInitType;

typedef struct
{
    uint32_t DatTimeout; /*!< Specifies the data timeout period in card bus clock periods. */

    uint32_t DatLen; /*!< Specifies the number of data bytes to be transferred. */

    uint32_t DatBlkSize; /*!< Specifies the data block size for block transfer.
                                      This parameter can be a value of @ref SDIO_Data_Block_Size */

    uint32_t TransferDirection; /*!< Specifies the data transfer direction, whether the transfer
                                    is a read or write.
                                    This parameter can be a value of @ref SDIO_Transfer_Direction */

    uint32_t TransferMode; /*!< Specifies whether data transfer is in stream or block mode.
                                     This parameter can be a value of @ref SDIO_Transfer_Type */

    uint32_t DPSMConfig; /*!< Specifies whether SDIO Data path state machine (DPSM)
                             is enabled or disabled.
                             This parameter can be a value of @ref SDIO_DPSM_State */
} SDIO_DataInitType;

/**
 * @}
 */

/** @addtogroup SDIO_Exported_Constants
 * @{
 */

/** @addtogroup SDIO_Clock_Edge
 * @{
 */

#define SDIO_CLKEDGE_RISING    ((uint32_t)0x00000000)
#define SDIO_CLKEDGE_FALLING   ((uint32_t)0x00002000)
#define IS_SDIO_CLK_EDGE(EDGE) (((EDGE) == SDIO_CLKEDGE_RISING) || ((EDGE) == SDIO_CLKEDGE_FALLING))
/**
 * @}
 */

/** @addtogroup SDIO_Clock_Bypass
 * @{
 */

#define SDIO_ClkBYPASS_DISABLE     ((uint32_t)0x00000000)
#define SDIO_ClkBYPASS_ENABLE      ((uint32_t)0x00000400)
#define IS_SDIO_CLK_BYPASS(BYPASS) (((BYPASS) == SDIO_ClkBYPASS_DISABLE) || ((BYPASS) == SDIO_ClkBYPASS_ENABLE))
/**
 * @}
 */

/** @addtogroup SDIO_Clock_Power_Save
 * @{
 */

#define SDIO_CLKPOWERSAVE_DISABLE    ((uint32_t)0x00000000)
#define SDIO_CLKPOWERSAVE_ENABLE     ((uint32_t)0x00000200)
#define IS_SDIO_CLK_POWER_SAVE(SAVE) (((SAVE) == SDIO_CLKPOWERSAVE_DISABLE) || ((SAVE) == SDIO_CLKPOWERSAVE_ENABLE))
/**
 * @}
 */

/** @addtogroup SDIO_Bus_Wide
 * @{
 */

#define SDIO_BUSWIDTH_1B ((uint32_t)0x00000000)
#define SDIO_BUSWIDTH_4B ((uint32_t)0x00000800)
#define SDIO_BUSWIDTH_8B ((uint32_t)0x00001000)
#define IS_SDIO_BUS_WIDTH(WIDE)                                                                                        \
    (((WIDE) == SDIO_BUSWIDTH_1B) || ((WIDE) == SDIO_BUSWIDTH_4B) || ((WIDE) == SDIO_BUSWIDTH_8B))

/**
 * @}
 */

/** @addtogroup SDIO_Hardware_Flow_Control
 * @{
 */

#define SDIO_HARDWARE_CLKCTRL_DISABLE ((uint32_t)0x00000000)
#define SDIO_HARDWARE_CLKCTRL_ENABLE  ((uint32_t)0x00004000)
#define IS_SDIO_HARDWARE_CLKCTRL(CONTROL)                                                                              \
    (((CONTROL) == SDIO_HARDWARE_CLKCTRL_DISABLE) || ((CONTROL) == SDIO_HARDWARE_CLKCTRL_ENABLE))
/**
 * @}
 */

/** @addtogroup SDIO_Power_State
 * @{
 */

#define SDIO_POWER_CTRL_OFF       ((uint32_t)0x00000000)
#define SDIO_POWER_CTRL_ON        ((uint32_t)0x00000003)
#define IS_SDIO_POWER_CTRL(STATE) (((STATE) == SDIO_POWER_CTRL_OFF) || ((STATE) == SDIO_POWER_CTRL_ON))
/**
 * @}
 */

/** @addtogroup SDIO_Interrupt_sources
 * @{
 */

#define SDIO_INT_CCRCERR     ((uint32_t)0x00000001)
#define SDIO_INT_DCRCERR     ((uint32_t)0x00000002)
#define SDIO_INT_CMDTIMEOUT  ((uint32_t)0x00000004)
#define SDIO_INT_DATTIMEOUT  ((uint32_t)0x00000008)
#define SDIO_INT_TXURERR     ((uint32_t)0x00000010)
#define SDIO_INT_RXORERR     ((uint32_t)0x00000020)
#define SDIO_INT_CMDRESPRECV ((uint32_t)0x00000040)
#define SDIO_INT_CMDSEND     ((uint32_t)0x00000080)
#define SDIO_INT_DATEND      ((uint32_t)0x00000100)
#define SDIO_INT_SBERR       ((uint32_t)0x00000200)
#define SDIO_INT_DATBLKEND   ((uint32_t)0x00000400)
#define SDIO_INT_CMDRUN      ((uint32_t)0x00000800)
#define SDIO_INT_TXRUN       ((uint32_t)0x00001000)
#define SDIO_INT_RXRUN       ((uint32_t)0x00002000)
#define SDIO_INT_TFIFOHE     ((uint32_t)0x00004000)
#define SDIO_INT_RFIFOHF     ((uint32_t)0x00008000)
#define SDIO_INT_TFIFOF      ((uint32_t)0x00010000)
#define SDIO_INT_RFIFOF      ((uint32_t)0x00020000)
#define SDIO_INT_TFIFOE      ((uint32_t)0x00040000)
#define SDIO_INT_RFIFOE      ((uint32_t)0x00080000)
#define SDIO_INT_TDATVALID   ((uint32_t)0x00100000)
#define SDIO_INT_RDATVALID   ((uint32_t)0x00200000)
#define SDIO_INT_SDIOINT     ((uint32_t)0x00400000)
#define SDIO_INT_CEATAF      ((uint32_t)0x00800000)
#define IS_SDIO_INT(IT)      ((((IT) & (uint32_t)0xFF000000) == 0x00) && ((IT) != (uint32_t)0x00))
/**
 * @}
 */

/** @addtogroup SDIO_Command_Index
 * @{
 */

#define IS_SDIO_CMD_INDEX(INDEX) ((INDEX) < 0x40)
/**
 * @}
 */

/** @addtogroup SDIO_Response_Type
 * @{
 */

#define SDIO_RESP_NO    ((uint32_t)0x00000000)
#define SDIO_RESP_SHORT ((uint32_t)0x00000040)
#define SDIO_RESP_LONG  ((uint32_t)0x000000C0)
#define IS_SDIO_RESP(RESPONSE)                                                                                         \
    (((RESPONSE) == SDIO_RESP_NO) || ((RESPONSE) == SDIO_RESP_SHORT) || ((RESPONSE) == SDIO_RESP_LONG))
/**
 * @}
 */

/** @addtogroup SDIO_Wait_Interrupt_State
 * @{
 */

#define SDIO_WAIT_NO       ((uint32_t)0x00000000) /*!< SDIO No Wait, TimeOut is enabled */
#define SDIO_WAIT_INT      ((uint32_t)0x00000100) /*!< SDIO Wait Interrupt Request */
#define SDIO_WAIT_PEND     ((uint32_t)0x00000200) /*!< SDIO Wait End of transfer */
#define IS_SDIO_WAIT(WAIT) (((WAIT) == SDIO_WAIT_NO) || ((WAIT) == SDIO_WAIT_INT) || ((WAIT) == SDIO_WAIT_PEND))
/**
 * @}
 */

/** @addtogroup SDIO_CPSM_State
 * @{
 */

#define SDIO_CPSM_DISABLE  ((uint32_t)0x00000000)
#define SDIO_CPSM_ENABLE   ((uint32_t)0x00000400)
#define IS_SDIO_CPSM(CPSM) (((CPSM) == SDIO_CPSM_ENABLE) || ((CPSM) == SDIO_CPSM_DISABLE))
/**
 * @}
 */

/** @addtogroup SDIO_Response_Registers
 * @{
 */

#define SDIO_RESPONSE_1 ((uint32_t)0x00000000)
#define SDIO_RESPONSE_2 ((uint32_t)0x00000004)
#define SDIO_RESPONSE_3 ((uint32_t)0x00000008)
#define SDIO_RESPONSE_4 ((uint32_t)0x0000000C)
#define IS_SDIO_RESPONSE(RESP)                                                                                         \
    (((RESP) == SDIO_RESPONSE_1) || ((RESP) == SDIO_RESPONSE_2) || ((RESP) == SDIO_RESPONSE_3)                         \
     || ((RESP) == SDIO_RESPONSE_4))
/**
 * @}
 */

/** @addtogroup SDIO_Data_Length
 * @{
 */

#define IS_SDIO_DAT_LEN(LENGTH) ((LENGTH) <= 0x01FFFFFF)
/**
 * @}
 */

/** @addtogroup SDIO_Data_Block_Size
 * @{
 */

#define SDIO_DATBLK_SIZE_1B     ((uint32_t)0x00000000)
#define SDIO_DATBLK_SIZE_2B     ((uint32_t)0x00000010)
#define SDIO_DATBLK_SIZE_4B     ((uint32_t)0x00000020)
#define SDIO_DATBLK_SIZE_8B     ((uint32_t)0x00000030)
#define SDIO_DATBLK_SIZE_16B    ((uint32_t)0x00000040)
#define SDIO_DATBLK_SIZE_32B    ((uint32_t)0x00000050)
#define SDIO_DATBLK_SIZE_64B    ((uint32_t)0x00000060)
#define SDIO_DATBLK_SIZE_128B   ((uint32_t)0x00000070)
#define SDIO_DATBLK_SIZE_256B   ((uint32_t)0x00000080)
#define SDIO_DATBLK_SIZE_512B   ((uint32_t)0x00000090)
#define SDIO_DATBLK_SIZE_1024B  ((uint32_t)0x000000A0)
#define SDIO_DATBLK_SIZE_2048B  ((uint32_t)0x000000B0)
#define SDIO_DATBLK_SIZE_4096B  ((uint32_t)0x000000C0)
#define SDIO_DATBLK_SIZE_8192B  ((uint32_t)0x000000D0)
#define SDIO_DATBLK_SIZE_16384B ((uint32_t)0x000000E0)
#define IS_SDIO_BLK_SIZE(SIZE)                                                                                         \
    (((SIZE) == SDIO_DATBLK_SIZE_1B) || ((SIZE) == SDIO_DATBLK_SIZE_2B) || ((SIZE) == SDIO_DATBLK_SIZE_4B)             \
     || ((SIZE) == SDIO_DATBLK_SIZE_8B) || ((SIZE) == SDIO_DATBLK_SIZE_16B) || ((SIZE) == SDIO_DATBLK_SIZE_32B)        \
     || ((SIZE) == SDIO_DATBLK_SIZE_64B) || ((SIZE) == SDIO_DATBLK_SIZE_128B) || ((SIZE) == SDIO_DATBLK_SIZE_256B)     \
     || ((SIZE) == SDIO_DATBLK_SIZE_512B) || ((SIZE) == SDIO_DATBLK_SIZE_1024B) || ((SIZE) == SDIO_DATBLK_SIZE_2048B)  \
     || ((SIZE) == SDIO_DATBLK_SIZE_4096B) || ((SIZE) == SDIO_DATBLK_SIZE_8192B)                                       \
     || ((SIZE) == SDIO_DATBLK_SIZE_16384B))
/**
 * @}
 */

/** @addtogroup SDIO_Transfer_Direction
 * @{
 */

#define SDIO_TRANSDIR_TOCARD            ((uint32_t)0x00000000)
#define SDIO_TRANSDIR_TOSDIO            ((uint32_t)0x00000002)
#define IS_SDIO_TRANSFER_DIRECTION(DIR) (((DIR) == SDIO_TRANSDIR_TOCARD) || ((DIR) == SDIO_TRANSDIR_TOSDIO))
/**
 * @}
 */

/** @addtogroup SDIO_Transfer_Type
 * @{
 */

#define SDIO_TRANSMODE_BLOCK     ((uint32_t)0x00000000)
#define SDIO_TRANSMODE_STREAM    ((uint32_t)0x00000004)
#define IS_SDIO_TRANS_MODE(MODE) (((MODE) == SDIO_TRANSMODE_STREAM) || ((MODE) == SDIO_TRANSMODE_BLOCK))
/**
 * @}
 */

/** @addtogroup SDIO_DPSM_State
 * @{
 */

#define SDIO_DPSM_DISABLE  ((uint32_t)0x00000000)
#define SDIO_DPSM_ENABLE   ((uint32_t)0x00000001)
#define IS_SDIO_DPSM(DPSM) (((DPSM) == SDIO_DPSM_ENABLE) || ((DPSM) == SDIO_DPSM_DISABLE))
/**
 * @}
 */

/** @addtogroup SDIO_Flags
 * @{
 */

#define SDIO_FLAG_CCRCERR     ((uint32_t)0x00000001)
#define SDIO_FLAG_DCRCERR     ((uint32_t)0x00000002)
#define SDIO_FLAG_CMDTIMEOUT  ((uint32_t)0x00000004)
#define SDIO_FLAG_DATTIMEOUT  ((uint32_t)0x00000008)
#define SDIO_FLAG_TXURERR     ((uint32_t)0x00000010)
#define SDIO_FLAG_RXORERR     ((uint32_t)0x00000020)
#define SDIO_FLAG_CMDRESPRECV ((uint32_t)0x00000040)
#define SDIO_FLAG_CMDSEND     ((uint32_t)0x00000080)
#define SDIO_FLAG_DATEND      ((uint32_t)0x00000100)
#define SDIO_FLAG_SBERR       ((uint32_t)0x00000200)
#define SDIO_FLAG_DATBLKEND   ((uint32_t)0x00000400)
#define SDIO_FLAG_CMDRUN      ((uint32_t)0x00000800)
#define SDIO_FLAG_TXRUN       ((uint32_t)0x00001000)
#define SDIO_FLAG_RXRUN       ((uint32_t)0x00002000)
#define SDIO_FLAG_TFIFOHE     ((uint32_t)0x00004000)
#define SDIO_FLAG_RFIFOHF     ((uint32_t)0x00008000)
#define SDIO_FLAG_TFIFOF      ((uint32_t)0x00010000)
#define SDIO_FLAG_RFIFOF      ((uint32_t)0x00020000)
#define SDIO_FLAG_TFIFOE      ((uint32_t)0x00040000)
#define SDIO_FLAG_RFIFOE      ((uint32_t)0x00080000)
#define SDIO_FLAG_TDATVALID   ((uint32_t)0x00100000)
#define SDIO_FLAG_RDATVALID   ((uint32_t)0x00200000)
#define SDIO_FLAG_SDIOINT     ((uint32_t)0x00400000)
#define SDIO_FLAG_CEATAF      ((uint32_t)0x00800000)
#define IS_SDIO_FLAG(FLAG)                                                                                             \
    (((FLAG) == SDIO_FLAG_CCRCERR) || ((FLAG) == SDIO_FLAG_DCRCERR) || ((FLAG) == SDIO_FLAG_CMDTIMEOUT)                \
     || ((FLAG) == SDIO_FLAG_DATTIMEOUT) || ((FLAG) == SDIO_FLAG_TXURERR) || ((FLAG) == SDIO_FLAG_RXORERR)             \
     || ((FLAG) == SDIO_FLAG_CMDRESPRECV) || ((FLAG) == SDIO_FLAG_CMDSEND) || ((FLAG) == SDIO_FLAG_DATEND)             \
     || ((FLAG) == SDIO_FLAG_SBERR) || ((FLAG) == SDIO_FLAG_DATBLKEND) || ((FLAG) == SDIO_FLAG_CMDRUN)                 \
     || ((FLAG) == SDIO_FLAG_TXRUN) || ((FLAG) == SDIO_FLAG_RXRUN) || ((FLAG) == SDIO_FLAG_TFIFOHE)                    \
     || ((FLAG) == SDIO_FLAG_RFIFOHF) || ((FLAG) == SDIO_FLAG_TFIFOF) || ((FLAG) == SDIO_FLAG_RFIFOF)                  \
     || ((FLAG) == SDIO_FLAG_TFIFOE) || ((FLAG) == SDIO_FLAG_RFIFOE) || ((FLAG) == SDIO_FLAG_TDATVALID)                \
     || ((FLAG) == SDIO_FLAG_RDATVALID) || ((FLAG) == SDIO_FLAG_SDIOINT) || ((FLAG) == SDIO_FLAG_CEATAF))

#define IS_SDIO_CLR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFF3FF800) == 0x00) && ((FLAG) != (uint32_t)0x00))

#define IS_SDIO_GET_INT(IT)                                                                                            \
    (((IT) == SDIO_INT_CCRCERR) || ((IT) == SDIO_INT_DCRCERR) || ((IT) == SDIO_INT_CMDTIMEOUT)                         \
     || ((IT) == SDIO_INT_DATTIMEOUT) || ((IT) == SDIO_INT_TXURERR) || ((IT) == SDIO_INT_RXORERR)                      \
     || ((IT) == SDIO_INT_CMDRESPRECV) || ((IT) == SDIO_INT_CMDSEND) || ((IT) == SDIO_INT_DATEND)                      \
     || ((IT) == SDIO_INT_SBERR) || ((IT) == SDIO_INT_DATBLKEND) || ((IT) == SDIO_INT_CMDRUN)                          \
     || ((IT) == SDIO_INT_TXRUN) || ((IT) == SDIO_INT_RXRUN) || ((IT) == SDIO_INT_TFIFOHE)                             \
     || ((IT) == SDIO_INT_RFIFOHF) || ((IT) == SDIO_INT_TFIFOF) || ((IT) == SDIO_INT_RFIFOF)                           \
     || ((IT) == SDIO_INT_TFIFOE) || ((IT) == SDIO_INT_RFIFOE) || ((IT) == SDIO_INT_TDATVALID)                         \
     || ((IT) == SDIO_INT_RDATVALID) || ((IT) == SDIO_INT_SDIOINT) || ((IT) == SDIO_INT_CEATAF))

#define IS_SDIO_CLR_INT(IT) ((((IT) & (uint32_t)0xFF3FF800) == 0x00) && ((IT) != (uint32_t)0x00))

/**
 * @}
 */

/** @addtogroup SDIO_Read_Wait_Mode
 * @{
 */

#define SDIO_RDWAIT_MODE_CLK      ((uint32_t)0x00000001)
#define SDIO_RDWAIT_MODE_DAT2     ((uint32_t)0x00000000)
#define IS_SDIO_RDWAIT_MODE(MODE) (((MODE) == SDIO_RDWAIT_MODE_CLK) || ((MODE) == SDIO_RDWAIT_MODE_DAT2))
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup SDIO_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup SDIO_Exported_Functions
 * @{
 */

void SDIO_DeInit(void);
void SDIO_Init(SDIO_InitType* SDIO_InitStruct);
void SDIO_InitStruct(SDIO_InitType* SDIO_InitStruct);
void SDIO_EnableClock(FunctionalState Cmd);
void SDIO_SetPower(uint32_t SDIO_PowerState);
uint32_t SDIO_GetPower(void);
void SDIO_ConfigInt(uint32_t SDIO_IT, FunctionalState Cmd);
void SDIO_DMACmd(FunctionalState Cmd);
void SDIO_SendCmd(SDIO_CmdInitType* SDIO_CmdInitStruct);
void SDIO_InitCmdStruct(SDIO_CmdInitType* SDIO_CmdInitStruct);
uint8_t SDIO_GetCmdResp(void);
uint32_t SDIO_GetResp(uint32_t SDIO_RESP);
void SDIO_ConfigData(SDIO_DataInitType* SDIO_DataInitStruct);
void SDIO_InitDataStruct(SDIO_DataInitType* SDIO_DataInitStruct);
uint32_t SDIO_GetDataCountValue(void);
uint32_t SDIO_ReadData(void);
void SDIO_WriteData(uint32_t Data);
uint32_t SDIO_GetFifoCounter(void);
void SDIO_EnableReadWait(FunctionalState Cmd);
void SDIO_DisableReadWait(FunctionalState Cmd);
void SDIO_EnableSdioReadWaitMode(uint32_t SDIO_ReadWaitMode);
void SDIO_EnableSdioOperation(FunctionalState Cmd);
void SDIO_EnableSendSdioSuspend(FunctionalState Cmd);
void SDIO_EnableCommandCompletion(FunctionalState Cmd);
void SDIO_EnableCEATAInt(FunctionalState Cmd);
void SDIO_EnableSendCEATA(FunctionalState Cmd);
FlagStatus SDIO_GetFlag(uint32_t SDIO_FLAG);
void SDIO_ClrFlag(uint32_t SDIO_FLAG);
INTStatus SDIO_GetIntStatus(uint32_t SDIO_IT);
void SDIO_ClrIntPendingBit(uint32_t SDIO_IT);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_SDIO_H__ */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
