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
 * @file n32a455_can.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_CAN_H__
#define __N32A455_CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup CAN
 * @{
 */

/** @addtogroup CAN_Exported_Types
 * @{
 */

#define IS_CAN_ALL_PERIPH(PERIPH) (((PERIPH) == CAN1) || ((PERIPH) == CAN2))

/**
 * @brief  CAN init structure definition
 */

typedef struct
{
    uint16_t BaudRatePrescaler; /*!< Specifies the length of a time quantum.
                                 It ranges from 1 to 1024. */

    uint8_t OperatingMode; /*!< Specifies the CAN operating mode.
                           This parameter can be a value of
                          @ref CAN_operating_mode */

    uint8_t RSJW; /*!< Specifies the maximum number of time quanta
                          the CAN hardware is allowed to lengthen or
                          shorten a bit to perform resynchronization.
                          This parameter can be a value of
                          @ref CAN_synchronisation_jump_width */

    uint8_t TBS1; /*!< Specifies the number of time quanta in Bit
                          Segment 1. This parameter can be a value of
                          @ref CAN_time_quantum_in_bit_segment_1 */

    uint8_t TBS2; /*!< Specifies the number of time quanta in Bit
                          Segment 2.
                          This parameter can be a value of
                          @ref CAN_time_quantum_in_bit_segment_2 */

    FunctionalState TTCM; /*!< Enable or disable the time triggered
                                   communication mode. This parameter can be set
                                   either to ENABLE or DISABLE. */

    FunctionalState ABOM; /*!< Enable or disable the automatic bus-off
                                   management. This parameter can be set either
                                   to ENABLE or DISABLE. */

    FunctionalState AWKUM; /*!< Enable or disable the automatic wake-up mode.
                                   This parameter can be set either to ENABLE or
                                   DISABLE. */

    FunctionalState NART; /*!< Enable or disable the no-automatic
                                   retransmission mode. This parameter can be
                                   set either to ENABLE or DISABLE. */

    FunctionalState RFLM; /*!< Enable or disable the Receive DATFIFO Locked mode.
                                   This parameter can be set either to ENABLE
                                   or DISABLE. */

    FunctionalState TXFP; /*!< Enable or disable the transmit DATFIFO priority.
                                   This parameter can be set either to ENABLE
                                   or DISABLE. */
} CAN_InitType;

/**
 * @brief  CAN filter init structure definition
 */

typedef struct
{
    uint16_t Filter_HighId; /*!< Specifies the filter identification number (MSBs for a 32-bit
                                        configuration, first one for a 16-bit configuration).
                                        This parameter can be a value between 0x0000 and 0xFFFF */

    uint16_t Filter_LowId; /*!< Specifies the filter identification number (LSBs for a 32-bit
                                       configuration, second one for a 16-bit configuration).
                                       This parameter can be a value between 0x0000 and 0xFFFF */

    uint16_t FilterMask_HighId; /*!< Specifies the filter mask number or identification number,
                                            according to the mode (MSBs for a 32-bit configuration,
                                            first one for a 16-bit configuration).
                                            This parameter can be a value between 0x0000 and 0xFFFF */

    uint16_t FilterMask_LowId; /*!< Specifies the filter mask number or identification number,
                                           according to the mode (LSBs for a 32-bit configuration,
                                           second one for a 16-bit configuration).
                                           This parameter can be a value between 0x0000 and 0xFFFF */

    uint16_t Filter_FIFOAssignment; /*!< Specifies the DATFIFO (0 or 1) which will be assigned to the filter.
                                                This parameter can be a value of @ref CAN_filter_FIFO */

    uint8_t Filter_Num; /*!< Specifies the filter which will be initialized. It ranges from 0 to 13. */

    uint8_t Filter_Mode; /*!< Specifies the filter mode to be initialized.
                                     This parameter can be a value of @ref CAN_filter_mode */

    uint8_t Filter_Scale; /*!< Specifies the filter scale.
                                      This parameter can be a value of @ref CAN_filter_scale */

    FunctionalState Filter_Act; /*!< Enable or disable the filter.
                                                This parameter can be set either to ENABLE or DISABLE. */
} CAN_FilterInitType;

/**
 * @brief  CAN Tx message structure definition
 */

typedef struct
{
    uint32_t StdId; /*!< Specifies the standard identifier.
                         This parameter can be a value between 0 to 0x7FF. */

    uint32_t ExtId; /*!< Specifies the extended identifier.
                         This parameter can be a value between 0 to 0x1FFFFFFF. */

    uint8_t IDE; /*!< Specifies the type of identifier for the message that
                      will be transmitted. This parameter can be a value
                      of @ref CAN_identifier_type */

    uint8_t RTR; /*!< Specifies the type of frame for the message that will
                      be transmitted. This parameter can be a value of
                      @ref CAN_remote_transmission_request */

    uint8_t DLC; /*!< Specifies the length of the frame that will be
                      transmitted. This parameter can be a value between
                      0 to 8 */

    uint8_t Data[8]; /*!< Contains the data to be transmitted. It ranges from 0
                          to 0xFF. */
} CanTxMessage;

/**
 * @brief  CAN Rx message structure definition
 */

typedef struct
{
    uint32_t StdId; /*!< Specifies the standard identifier.
                         This parameter can be a value between 0 to 0x7FF. */

    uint32_t ExtId; /*!< Specifies the extended identifier.
                         This parameter can be a value between 0 to 0x1FFFFFFF. */

    uint8_t IDE; /*!< Specifies the type of identifier for the message that
                      will be received. This parameter can be a value of
                      @ref CAN_identifier_type */

    uint8_t RTR; /*!< Specifies the type of frame for the received message.
                      This parameter can be a value of
                      @ref CAN_remote_transmission_request */

    uint8_t DLC; /*!< Specifies the length of the frame that will be received.
                      This parameter can be a value between 0 to 8 */

    uint8_t Data[8]; /*!< Contains the data to be received. It ranges from 0 to
                          0xFF. */

    uint8_t FMI; /*!< Specifies the index of the filter the message stored in
                      the mailbox passes through. This parameter can be a
                      value between 0 to 0xFF */
} CanRxMessage;

/**
 * @}
 */

/** @addtogroup CAN_Exported_Constants
 * @{
 */

/** @addtogroup CAN_sleep_constants
 * @{
 */

#define CAN_InitSTS_Failed  ((uint8_t)0x00) /*!< CAN initialization failed */
#define CAN_InitSTS_Success ((uint8_t)0x01) /*!< CAN initialization OK */

/**
 * @}
 */

/** @addtogroup OperatingMode
 * @{
 */

#define CAN_Normal_Mode          ((uint8_t)0x00) /*!< normal mode */
#define CAN_LoopBack_Mode        ((uint8_t)0x01) /*!< loopback mode */
#define CAN_Silent_Mode          ((uint8_t)0x02) /*!< silent mode */
#define CAN_Silent_LoopBack_Mode ((uint8_t)0x03) /*!< loopback combined with silent mode */

#define IS_CAN_MODE(MODE)                                                                                              \
    (((MODE) == CAN_Normal_Mode) || ((MODE) == CAN_LoopBack_Mode) || ((MODE) == CAN_Silent_Mode)                       \
     || ((MODE) == CAN_Silent_LoopBack_Mode))
/**
 * @}
 */

/**
 * @addtogroup CAN_operating_mode
 * @{
 */
#define CAN_Operating_InitMode   ((uint8_t)0x00) /*!< Initialization mode */
#define CAN_Operating_NormalMode ((uint8_t)0x01) /*!< Normal mode */
#define CAN_Operating_SleepMode  ((uint8_t)0x02) /*!< sleep mode */

#define IS_CAN_OPERATING_MODE(MODE)                                                                                    \
    (((MODE) == CAN_Operating_InitMode) || ((MODE) == CAN_Operating_NormalMode) || ((MODE) == CAN_Operating_SleepMode))
/**
 * @}
 */

/**
 * @addtogroup CAN_Mode_Status
 * @{
 */

#define CAN_ModeSTS_Failed  ((uint8_t)0x00)                /*!< CAN entering the specific mode failed */
#define CAN_ModeSTS_Success ((uint8_t)!CAN_ModeSTS_Failed) /*!< CAN entering the specific mode Succeed */

/**
 * @}
 */

/** @addtogroup CAN_synchronisation_jump_width
 * @{
 */

#define CAN_RSJW_1tq ((uint8_t)0x00) /*!< 1 time quantum */
#define CAN_RSJW_2tq ((uint8_t)0x01) /*!< 2 time quantum */
#define CAN_RSJW_3tq ((uint8_t)0x02) /*!< 3 time quantum */
#define CAN_RSJW_4tq ((uint8_t)0x03) /*!< 4 time quantum */

#define IS_CAN_RSJW(SJW)                                                                                               \
    (((SJW) == CAN_RSJW_1tq) || ((SJW) == CAN_RSJW_2tq) || ((SJW) == CAN_RSJW_3tq) || ((SJW) == CAN_RSJW_4tq))
/**
 * @}
 */

/** @addtogroup CAN_time_quantum_in_bit_segment_1
 * @{
 */

#define CAN_TBS1_1tq  ((uint8_t)0x00) /*!< 1 time quantum */
#define CAN_TBS1_2tq  ((uint8_t)0x01) /*!< 2 time quantum */
#define CAN_TBS1_3tq  ((uint8_t)0x02) /*!< 3 time quantum */
#define CAN_TBS1_4tq  ((uint8_t)0x03) /*!< 4 time quantum */
#define CAN_TBS1_5tq  ((uint8_t)0x04) /*!< 5 time quantum */
#define CAN_TBS1_6tq  ((uint8_t)0x05) /*!< 6 time quantum */
#define CAN_TBS1_7tq  ((uint8_t)0x06) /*!< 7 time quantum */
#define CAN_TBS1_8tq  ((uint8_t)0x07) /*!< 8 time quantum */
#define CAN_TBS1_9tq  ((uint8_t)0x08) /*!< 9 time quantum */
#define CAN_TBS1_10tq ((uint8_t)0x09) /*!< 10 time quantum */
#define CAN_TBS1_11tq ((uint8_t)0x0A) /*!< 11 time quantum */
#define CAN_TBS1_12tq ((uint8_t)0x0B) /*!< 12 time quantum */
#define CAN_TBS1_13tq ((uint8_t)0x0C) /*!< 13 time quantum */
#define CAN_TBS1_14tq ((uint8_t)0x0D) /*!< 14 time quantum */
#define CAN_TBS1_15tq ((uint8_t)0x0E) /*!< 15 time quantum */
#define CAN_TBS1_16tq ((uint8_t)0x0F) /*!< 16 time quantum */

#define IS_CAN_TBS1(BS1) ((BS1) <= CAN_TBS1_16tq)
/**
 * @}
 */

/** @addtogroup CAN_time_quantum_in_bit_segment_2
 * @{
 */

#define CAN_TBS2_1tq ((uint8_t)0x00) /*!< 1 time quantum */
#define CAN_TBS2_2tq ((uint8_t)0x01) /*!< 2 time quantum */
#define CAN_TBS2_3tq ((uint8_t)0x02) /*!< 3 time quantum */
#define CAN_TBS2_4tq ((uint8_t)0x03) /*!< 4 time quantum */
#define CAN_TBS2_5tq ((uint8_t)0x04) /*!< 5 time quantum */
#define CAN_TBS2_6tq ((uint8_t)0x05) /*!< 6 time quantum */
#define CAN_TBS2_7tq ((uint8_t)0x06) /*!< 7 time quantum */
#define CAN_TBS2_8tq ((uint8_t)0x07) /*!< 8 time quantum */

#define IS_CAN_TBS2(BS2) ((BS2) <= CAN_TBS2_8tq)

/**
 * @}
 */

/** @addtogroup CAN_clock_prescaler
 * @{
 */

#define IS_CAN_BAUDRATEPRESCALER(PRESCALER) (((PRESCALER) >= 1) && ((PRESCALER) <= 1024))

/**
 * @}
 */

/** @addtogroup CAN_filter_number
 * @{
 */
#define IS_CAN_FILTER_NUM(NUMBER) ((NUMBER) <= 13)
/**
 * @}
 */

/** @addtogroup CAN_filter_mode
 * @{
 */

#define CAN_Filter_IdMaskMode ((uint8_t)0x00) /*!< identifier/mask mode */
#define CAN_Filter_IdListMode ((uint8_t)0x01) /*!< identifier list mode */

#define IS_CAN_FILTER_MODE(MODE) (((MODE) == CAN_Filter_IdMaskMode) || ((MODE) == CAN_Filter_IdListMode))
/**
 * @}
 */

/** @addtogroup CAN_filter_scale
 * @{
 */

#define CAN_Filter_16bitScale ((uint8_t)0x00) /*!< Two 16-bit filters */
#define CAN_Filter_32bitScale ((uint8_t)0x01) /*!< One 32-bit filter */

#define IS_CAN_FILTER_SCALE(SCALE) (((SCALE) == CAN_Filter_16bitScale) || ((SCALE) == CAN_Filter_32bitScale))

/**
 * @}
 */

/** @addtogroup CAN_filter_FIFO
 * @{
 */

#define CAN_Filter_FIFO0            ((uint8_t)0x00) /*!< Filter DATFIFO 0 assignment for filter x */
#define CAN_Filter_FIFO1            ((uint8_t)0x01) /*!< Filter DATFIFO 1 assignment for filter x */
#define IS_CAN_FILTER_FIFO(DATFIFO) (((DATFIFO) == CAN_FilterFIFO0) || ((DATFIFO) == CAN_FilterFIFO1))
/**
 * @}
 */

/** @addtogroup CAN_Tx
 * @{
 */

#define IS_CAN_TRANSMITMAILBOX(TRANSMITMAILBOX) ((TRANSMITMAILBOX) <= ((uint8_t)0x02))
#define IS_CAN_STDID(STDID)                     ((STDID) <= ((uint32_t)0x7FF))
#define IS_CAN_EXTID(EXTID)                     ((EXTID) <= ((uint32_t)0x1FFFFFFF))
#define IS_CAN_DLC(DLC)                         ((DLC) <= ((uint8_t)0x08))

/**
 * @}
 */

/** @addtogroup CAN_identifier_type
 * @{
 */

#define CAN_Standard_Id   ((uint32_t)0x00000000) /*!< Standard Id */
#define CAN_Extended_Id   ((uint32_t)0x00000004) /*!< Extended Id */
#define IS_CAN_ID(IDTYPE) (((IDTYPE) == CAN_Standard_Id) || ((IDTYPE) == CAN_Extended_Id))
/**
 * @}
 */

/** @addtogroup CAN_remote_transmission_request
 * @{
 */

#define CAN_RTRQ_Data    ((uint32_t)0x00000000) /*!< Data frame */
#define CAN_RTRQ_Remote  ((uint32_t)0x00000002) /*!< Remote frame */
#define IS_CAN_RTRQ(RTR) (((RTR) == CAN_RTRQ_Data) || ((RTR) == CAN_RTRQ_Remote))

/**
 * @}
 */

/** @addtogroup CAN_transmit_constants
 * @{
 */

#define CAN_TxSTS_Failed    ((uint8_t)0x00) /*!< CAN transmission failed */
#define CAN_TxSTS_Ok        ((uint8_t)0x01) /*!< CAN transmission succeeded */
#define CAN_TxSTS_Pending   ((uint8_t)0x02) /*!< CAN transmission pending */
#define CAN_TxSTS_NoMailBox ((uint8_t)0x04) /*!< CAN cell did not provide an empty mailbox */

/**
 * @}
 */

/** @addtogroup CAN_receive_FIFO_number_constants
 * @{
 */

#define CAN_FIFO0 ((uint8_t)0x00) /*!< CAN DATFIFO 0 used to receive */
#define CAN_FIFO1 ((uint8_t)0x01) /*!< CAN DATFIFO 1 used to receive */

#define IS_CAN_FIFO(DATFIFO) (((DATFIFO) == CAN_FIFO0) || ((DATFIFO) == CAN_FIFO1))

/**
 * @}
 */

/** @addtogroup CAN_sleep_constants
 * @{
 */

#define CAN_SLEEP_Failed ((uint8_t)0x00) /*!< CAN did not enter the sleep mode */
#define CAN_SLEEP_Ok     ((uint8_t)0x01) /*!< CAN entered the sleep mode */

/**
 * @}
 */

/** @addtogroup CAN_wake_up_constants
 * @{
 */

#define CAN_WKU_Failed ((uint8_t)0x00) /*!< CAN did not leave the sleep mode */
#define CAN_WKU_Ok     ((uint8_t)0x01) /*!< CAN leaved the sleep mode */

/**
 * @}
 */

/**
 * @addtogroup   CAN_Error_Code_constants
 * @{
 */

#define CAN_ERRCode_NoErr           ((uint8_t)0x00) /*!< No Error */
#define CAN_ERRCode_StuffErr        ((uint8_t)0x10) /*!< Stuff Error */
#define CAN_ERRCode_FormErr         ((uint8_t)0x20) /*!< Form Error */
#define CAN_ERRCode_ACKErr          ((uint8_t)0x30) /*!< Acknowledgment Error */
#define CAN_ERRCode_BitRecessiveErr ((uint8_t)0x40) /*!< Bit Recessive Error */
#define CAN_ERRCode_BitDominantErr  ((uint8_t)0x50) /*!< Bit Dominant Error */
#define CAN_ERRCode_CRCErr          ((uint8_t)0x60) /*!< CRC Error  */
#define CAN_ERRCode_SWSetErr        ((uint8_t)0x70) /*!< Software Set Error */

/**
 * @}
 */

/** @addtogroup CAN_flags
 * @{
 */
/* If the flag is 0x3XXXXXXX, it means that it can be used with CAN_GetFlagSTS()
   and CAN_ClearFlag() functions. */
/* If the flag is 0x1XXXXXXX, it means that it can only be used with CAN_GetFlagSTS() function.  */

/* Transmit Flags */
#define CAN_FLAG_RQCPM0 ((uint32_t)0x38000001) /*!< Request MailBox0 Flag */
#define CAN_FLAG_RQCPM1 ((uint32_t)0x38000100) /*!< Request MailBox1 Flag */
#define CAN_FLAG_RQCPM2 ((uint32_t)0x38010000) /*!< Request MailBox2 Flag */

/* Receive Flags */
#define CAN_FLAG_FFMP0  ((uint32_t)0x12000003) /*!< DATFIFO 0 Message Pending Flag */
#define CAN_FLAG_FFULL0 ((uint32_t)0x32000008) /*!< DATFIFO 0 Full Flag            */
#define CAN_FLAG_FFOVR0 ((uint32_t)0x32000010) /*!< DATFIFO 0 Overrun Flag         */
#define CAN_FLAG_FFMP1  ((uint32_t)0x14000003) /*!< DATFIFO 1 Message Pending Flag */
#define CAN_FLAG_FFULL1 ((uint32_t)0x34000008) /*!< DATFIFO 1 Full Flag            */
#define CAN_FLAG_FFOVR1 ((uint32_t)0x34000010) /*!< DATFIFO 1 Overrun Flag         */

/* Operating Mode Flags */
#define CAN_FLAG_WKU  ((uint32_t)0x31000008) /*!< Wake up Flag */
#define CAN_FLAG_SLAK ((uint32_t)0x31000012) /*!< Sleep acknowledge Flag */
/* Note: When SLAK intterupt is disabled (SLKIE=0), no polling on SLAKI is possible.
         In this case the SLAK bit can be polled.*/

/* Error Flags */
#define CAN_FLAG_EWGFL ((uint32_t)0x10F00001) /*!< Error Warning Flag   */
#define CAN_FLAG_EPVFL ((uint32_t)0x10F00002) /*!< Error Passive Flag   */
#define CAN_FLAG_BOFFL ((uint32_t)0x10F00004) /*!< Bus-Off Flag         */
#define CAN_FLAG_LEC   ((uint32_t)0x30F00070) /*!< Last error code Flag */

#define IS_CAN_GET_FLAG(FLAG)                                                                                          \
    (((FLAG) == CAN_FLAG_LEC) || ((FLAG) == CAN_FLAG_BOFFL) || ((FLAG) == CAN_FLAG_EPVFL)                              \
     || ((FLAG) == CAN_FLAG_EWGFL) || ((FLAG) == CAN_FLAG_WKU) || ((FLAG) == CAN_FLAG_FFOVR0)                          \
     || ((FLAG) == CAN_FLAG_FFULL0) || ((FLAG) == CAN_FLAG_FFMP0) || ((FLAG) == CAN_FLAG_FFOVR1)                       \
     || ((FLAG) == CAN_FLAG_FFULL1) || ((FLAG) == CAN_FLAG_FFMP1) || ((FLAG) == CAN_FLAG_RQCPM2)                       \
     || ((FLAG) == CAN_FLAG_RQCPM1) || ((FLAG) == CAN_FLAG_RQCPM0) || ((FLAG) == CAN_FLAG_SLAK))

#define IS_CAN_CLEAR_FLAG(FLAG)                                                                                        \
    (((FLAG) == CAN_FLAG_LEC) || ((FLAG) == CAN_FLAG_RQCPM2) || ((FLAG) == CAN_FLAG_RQCPM1)                            \
     || ((FLAG) == CAN_FLAG_RQCPM0) || ((FLAG) == CAN_FLAG_FFULL0) || ((FLAG) == CAN_FLAG_FFOVR0)                      \
     || ((FLAG) == CAN_FLAG_FFULL1) || ((FLAG) == CAN_FLAG_FFOVR1) || ((FLAG) == CAN_FLAG_WKU)                         \
     || ((FLAG) == CAN_FLAG_SLAK))
/**
 * @}
 */

/** @addtogroup CAN_interrupts
 * @{
 */

#define CAN_INT_TME ((uint32_t)0x00000001) /*!< Transmit mailbox empty Interrupt*/

/* Receive Interrupts */
#define CAN_INT_FMP0 ((uint32_t)0x00000002) /*!< DATFIFO 0 message pending Interrupt*/
#define CAN_INT_FF0  ((uint32_t)0x00000004) /*!< DATFIFO 0 full Interrupt*/
#define CAN_INT_FOV0 ((uint32_t)0x00000008) /*!< DATFIFO 0 overrun Interrupt*/
#define CAN_INT_FMP1 ((uint32_t)0x00000010) /*!< DATFIFO 1 message pending Interrupt*/
#define CAN_INT_FF1  ((uint32_t)0x00000020) /*!< DATFIFO 1 full Interrupt*/
#define CAN_INT_FOV1 ((uint32_t)0x00000040) /*!< DATFIFO 1 overrun Interrupt*/

/* Operating Mode Interrupts */
#define CAN_INT_WKU ((uint32_t)0x00010000) /*!< Wake-up Interrupt*/
#define CAN_INT_SLK ((uint32_t)0x00020000) /*!< Sleep acknowledge Interrupt*/

/* Error Interrupts */
#define CAN_INT_EWG ((uint32_t)0x00000100) /*!< Error warning Interrupt*/
#define CAN_INT_EPV ((uint32_t)0x00000200) /*!< Error passive Interrupt*/
#define CAN_INT_BOF ((uint32_t)0x00000400) /*!< Bus-off Interrupt*/
#define CAN_INT_LEC ((uint32_t)0x00000800) /*!< Last error code Interrupt*/
#define CAN_INT_ERR ((uint32_t)0x00008000) /*!< Error Interrupt*/

/* Flags named as Interrupts : kept only for FW compatibility */
#define CAN_INT_RQCPM0 CAN_INT_TME
#define CAN_INT_RQCPM1 CAN_INT_TME
#define CAN_INT_RQCPM2 CAN_INT_TME

#define IS_CAN_INT(IT)                                                                                                 \
    (((IT) == CAN_INT_TME) || ((IT) == CAN_INT_FMP0) || ((IT) == CAN_INT_FF0) || ((IT) == CAN_INT_FOV0)                \
     || ((IT) == CAN_INT_FMP1) || ((IT) == CAN_INT_FF1) || ((IT) == CAN_INT_FOV1) || ((IT) == CAN_INT_EWG)             \
     || ((IT) == CAN_INT_EPV) || ((IT) == CAN_INT_BOF) || ((IT) == CAN_INT_LEC) || ((IT) == CAN_INT_ERR)               \
     || ((IT) == CAN_INT_WKU) || ((IT) == CAN_INT_SLK))

#define IS_CAN_CLEAR_INT(IT)                                                                                           \
    (((IT) == CAN_INT_TME) || ((IT) == CAN_INT_FF0) || ((IT) == CAN_INT_FOV0) || ((IT) == CAN_INT_FF1)                 \
     || ((IT) == CAN_INT_FOV1) || ((IT) == CAN_INT_EWG) || ((IT) == CAN_INT_EPV) || ((IT) == CAN_INT_BOF)              \
     || ((IT) == CAN_INT_LEC) || ((IT) == CAN_INT_ERR) || ((IT) == CAN_INT_WKU) || ((IT) == CAN_INT_SLK))

/**
 * @}
 */

/** @addtogroup CAN_Legacy
 * @{
 */
#define CANINITSTSFAILED CAN_InitSTS_Failed
#define CANINITSTSOK     CAN_InitSTS_Success
#define CAN_FilterFIFO0  CAN_Filter_FIFO0
#define CAN_FilterFIFO1  CAN_Filter_FIFO1
#define CAN_ID_STD       CAN_Standard_Id
#define CAN_ID_EXT       CAN_Extended_Id
#define CAN_RTRQ_DATA    CAN_RTRQ_Data
#define CAN_RTRQ_REMOTE  CAN_RTRQ_Remote
#define CANTXSTSFAILE    CAN_TxSTS_Failed
#define CANTXSTSOK       CAN_TxSTS_Ok
#define CANTXSTSPENDING  CAN_TxSTS_Pending
#define CAN_STS_NO_MB    CAN_TxSTS_NoMailBox
#define CANSLEEPFAILED   CAN_SLEEP_Failed
#define CANSLEEPOK       CAN_SLEEP_Ok
#define CANWKUFAILED     CAN_WKU_Failed
#define CANWKUOK         CAN_WKU_Ok

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup CAN_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup CAN_Exported_Functions
 * @{
 */
/*  Function used to set the CAN configuration to the default reset state *****/
void CAN_DeInit(CAN_Module* CANx);

/* Initialization and Configuration functions *********************************/
uint8_t CAN_Init(CAN_Module* CANx, CAN_InitType* CAN_InitParam);
void CAN1_InitFilter(CAN_FilterInitType* CAN_InitFilterStruct);
void CAN2_InitFilter(CAN_FilterInitType* CAN_InitFilterStruct);
void CAN_InitStruct(CAN_InitType* CAN_InitParam);
void CAN_DebugFreeze(CAN_Module* CANx, FunctionalState Cmd);
void CAN_EnTTComMode(CAN_Module* CANx, FunctionalState Cmd);

/* Transmit functions *********************************************************/
uint8_t CAN_TransmitMessage(CAN_Module* CANx, CanTxMessage* TxMessage);
uint8_t CAN_TransmitSTS(CAN_Module* CANx, uint8_t TransmitMailbox);
void CAN_CancelTransmitMessage(CAN_Module* CANx, uint8_t Mailbox);

/* Receive functions **********************************************************/
void CAN_ReceiveMessage(CAN_Module* CANx, uint8_t FIFONum, CanRxMessage* RxMessage);
void CAN_ReleaseFIFO(CAN_Module* CANx, uint8_t FIFONum);
uint8_t CAN_PendingMessage(CAN_Module* CANx, uint8_t FIFONum);

/* Operation modes functions **************************************************/
uint8_t CAN_OperatingModeReq(CAN_Module* CANx, uint8_t CAN_OperatingMode);
uint8_t CAN_EnterSleep(CAN_Module* CANx);
uint8_t CAN_WakeUp(CAN_Module* CANx);

/* Error management functions *************************************************/
uint8_t CAN_GetLastErrCode(CAN_Module* CANx);
uint8_t CAN_GetReceiveErrCounter(CAN_Module* CANx);
uint8_t CAN_GetLSBTransmitErrCounter(CAN_Module* CANx);

/* Interrupts and flags management functions **********************************/
void CAN_INTConfig(CAN_Module* CANx, uint32_t CAN_INT, FunctionalState Cmd);
FlagStatus CAN_GetFlagSTS(CAN_Module* CANx, uint32_t CAN_FLAG);
void CAN_ClearFlag(CAN_Module* CANx, uint32_t CAN_FLAG);
INTStatus CAN_GetIntStatus(CAN_Module* CANx, uint32_t CAN_INT);
void CAN_ClearINTPendingBit(CAN_Module* CANx, uint32_t CAN_INT);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_CAN_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
