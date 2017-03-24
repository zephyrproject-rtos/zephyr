/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_SDHC_H_
#define _FSL_SDHC_H_

#include "fsl_common.h"

/*!
 * @addtogroup sdhc
 * @{
 */

/******************************************************************************
 * Definitions.
 *****************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Driver version 2.1.5. */
#define FSL_SDHC_DRIVER_VERSION (MAKE_VERSION(2U, 1U, 5U))
/*@}*/

/*! @brief Maximum block count can be set one time */
#define SDHC_MAX_BLOCK_COUNT (SDHC_BLKATTR_BLKCNT_MASK >> SDHC_BLKATTR_BLKCNT_SHIFT)

/*! @brief SDHC status */
enum _sdhc_status
{
    kStatus_SDHC_BusyTransferring = MAKE_STATUS(kStatusGroup_SDHC, 0U),            /*!< Transfer is on-going */
    kStatus_SDHC_PrepareAdmaDescriptorFailed = MAKE_STATUS(kStatusGroup_SDHC, 1U), /*!< Set DMA descriptor failed */
    kStatus_SDHC_SendCommandFailed = MAKE_STATUS(kStatusGroup_SDHC, 2U),           /*!< Send command failed */
    kStatus_SDHC_TransferDataFailed = MAKE_STATUS(kStatusGroup_SDHC, 3U),          /*!< Transfer data failed */
    kStatus_SDHC_DMADataBufferAddrNotAlign =
        MAKE_STATUS(kStatusGroup_SDHC, 4U), /*!< data buffer addr not align in DMA mode */
};

/*! @brief Host controller capabilities flag mask */
enum _sdhc_capability_flag
{
    kSDHC_SupportAdmaFlag = SDHC_HTCAPBLT_ADMAS_MASK,        /*!< Support ADMA */
    kSDHC_SupportHighSpeedFlag = SDHC_HTCAPBLT_HSS_MASK,     /*!< Support high-speed */
    kSDHC_SupportDmaFlag = SDHC_HTCAPBLT_DMAS_MASK,          /*!< Support DMA */
    kSDHC_SupportSuspendResumeFlag = SDHC_HTCAPBLT_SRS_MASK, /*!< Support suspend/resume */
    kSDHC_SupportV330Flag = SDHC_HTCAPBLT_VS33_MASK,         /*!< Support voltage 3.3V */
#if defined FSL_FEATURE_SDHC_HAS_V300_SUPPORT && FSL_FEATURE_SDHC_HAS_V300_SUPPORT
    kSDHC_SupportV300Flag = SDHC_HTCAPBLT_VS30_MASK, /*!< Support voltage 3.0V */
#endif
#if defined FSL_FEATURE_SDHC_HAS_V180_SUPPORT && FSL_FEATURE_SDHC_HAS_V180_SUPPORT
    kSDHC_SupportV180Flag = SDHC_HTCAPBLT_VS18_MASK, /*!< Support voltage 1.8V */
#endif
    /* Put additional two flags in HTCAPBLT_MBL's position. */
    kSDHC_Support4BitFlag = (SDHC_HTCAPBLT_MBL_SHIFT << 0U), /*!< Support 4 bit mode */
    kSDHC_Support8BitFlag = (SDHC_HTCAPBLT_MBL_SHIFT << 1U), /*!< Support 8 bit mode */
};

/*! @brief Wakeup event mask */
enum _sdhc_wakeup_event
{
    kSDHC_WakeupEventOnCardInt = SDHC_PROCTL_WECINT_MASK,    /*!< Wakeup on card interrupt */
    kSDHC_WakeupEventOnCardInsert = SDHC_PROCTL_WECINS_MASK, /*!< Wakeup on card insertion */
    kSDHC_WakeupEventOnCardRemove = SDHC_PROCTL_WECRM_MASK,  /*!< Wakeup on card removal */

    kSDHC_WakeupEventsAll = (kSDHC_WakeupEventOnCardInt | kSDHC_WakeupEventOnCardInsert |
                             kSDHC_WakeupEventOnCardRemove), /*!< All wakeup events */
};

/*! @brief Reset type mask */
enum _sdhc_reset
{
    kSDHC_ResetAll = SDHC_SYSCTL_RSTA_MASK,     /*!< Reset all except card detection */
    kSDHC_ResetCommand = SDHC_SYSCTL_RSTC_MASK, /*!< Reset command line */
    kSDHC_ResetData = SDHC_SYSCTL_RSTD_MASK,    /*!< Reset data line */

    kSDHC_ResetsAll = (kSDHC_ResetAll | kSDHC_ResetCommand | kSDHC_ResetData), /*!< All reset types */
};

/*! @brief Transfer flag mask */
enum _sdhc_transfer_flag
{
    kSDHC_EnableDmaFlag = SDHC_XFERTYP_DMAEN_MASK, /*!< Enable DMA */

    kSDHC_CommandTypeSuspendFlag = (SDHC_XFERTYP_CMDTYP(1U)), /*!< Suspend command */
    kSDHC_CommandTypeResumeFlag = (SDHC_XFERTYP_CMDTYP(2U)),  /*!< Resume command */
    kSDHC_CommandTypeAbortFlag = (SDHC_XFERTYP_CMDTYP(3U)),   /*!< Abort command */

    kSDHC_EnableBlockCountFlag = SDHC_XFERTYP_BCEN_MASK,      /*!< Enable block count */
    kSDHC_EnableAutoCommand12Flag = SDHC_XFERTYP_AC12EN_MASK, /*!< Enable auto CMD12 */
    kSDHC_DataReadFlag = SDHC_XFERTYP_DTDSEL_MASK,            /*!< Enable data read */
    kSDHC_MultipleBlockFlag = SDHC_XFERTYP_MSBSEL_MASK,       /*!< Multiple block data read/write */

    kSDHC_ResponseLength136Flag = SDHC_XFERTYP_RSPTYP(1U),    /*!< 136 bit response length */
    kSDHC_ResponseLength48Flag = SDHC_XFERTYP_RSPTYP(2U),     /*!< 48 bit response length */
    kSDHC_ResponseLength48BusyFlag = SDHC_XFERTYP_RSPTYP(3U), /*!< 48 bit response length with busy status */

    kSDHC_EnableCrcCheckFlag = SDHC_XFERTYP_CCCEN_MASK,   /*!< Enable CRC check */
    kSDHC_EnableIndexCheckFlag = SDHC_XFERTYP_CICEN_MASK, /*!< Enable index check */
    kSDHC_DataPresentFlag = SDHC_XFERTYP_DPSEL_MASK,      /*!< Data present flag */
};

/*! @brief Present status flag mask */
enum _sdhc_present_status_flag
{
    kSDHC_CommandInhibitFlag = SDHC_PRSSTAT_CIHB_MASK,     /*!< Command inhibit */
    kSDHC_DataInhibitFlag = SDHC_PRSSTAT_CDIHB_MASK,       /*!< Data inhibit */
    kSDHC_DataLineActiveFlag = SDHC_PRSSTAT_DLA_MASK,      /*!< Data line active */
    kSDHC_SdClockStableFlag = SDHC_PRSSTAT_SDSTB_MASK,     /*!< SD bus clock stable */
    kSDHC_WriteTransferActiveFlag = SDHC_PRSSTAT_WTA_MASK, /*!< Write transfer active */
    kSDHC_ReadTransferActiveFlag = SDHC_PRSSTAT_RTA_MASK,  /*!< Read transfer active */
    kSDHC_BufferWriteEnableFlag = SDHC_PRSSTAT_BWEN_MASK,  /*!< Buffer write enable */
    kSDHC_BufferReadEnableFlag = SDHC_PRSSTAT_BREN_MASK,   /*!< Buffer read enable */
    kSDHC_CardInsertedFlag = SDHC_PRSSTAT_CINS_MASK,       /*!< Card inserted */
    kSDHC_CommandLineLevelFlag = SDHC_PRSSTAT_CLSL_MASK,   /*!< Command line signal level */
    kSDHC_Data0LineLevelFlag = (1U << 24U),                /*!< Data0 line signal level */
    kSDHC_Data1LineLevelFlag = (1U << 25U),                /*!< Data1 line signal level */
    kSDHC_Data2LineLevelFlag = (1U << 26U),                /*!< Data2 line signal level */
    kSDHC_Data3LineLevelFlag = (1U << 27U),                /*!< Data3 line signal level */
    kSDHC_Data4LineLevelFlag = (1U << 28U),                /*!< Data4 line signal level */
    kSDHC_Data5LineLevelFlag = (1U << 29U),                /*!< Data5 line signal level */
    kSDHC_Data6LineLevelFlag = (1U << 30U),                /*!< Data6 line signal level */
    kSDHC_Data7LineLevelFlag = (1U << 31U),                /*!< Data7 line signal level */
};

/*! @brief Interrupt status flag mask */
enum _sdhc_interrupt_status_flag
{
    kSDHC_CommandCompleteFlag = SDHC_IRQSTAT_CC_MASK,       /*!< Command complete */
    kSDHC_DataCompleteFlag = SDHC_IRQSTAT_TC_MASK,          /*!< Data complete */
    kSDHC_BlockGapEventFlag = SDHC_IRQSTAT_BGE_MASK,        /*!< Block gap event */
    kSDHC_DmaCompleteFlag = SDHC_IRQSTAT_DINT_MASK,         /*!< DMA interrupt */
    kSDHC_BufferWriteReadyFlag = SDHC_IRQSTAT_BWR_MASK,     /*!< Buffer write ready */
    kSDHC_BufferReadReadyFlag = SDHC_IRQSTAT_BRR_MASK,      /*!< Buffer read ready */
    kSDHC_CardInsertionFlag = SDHC_IRQSTAT_CINS_MASK,       /*!< Card inserted */
    kSDHC_CardRemovalFlag = SDHC_IRQSTAT_CRM_MASK,          /*!< Card removed */
    kSDHC_CardInterruptFlag = SDHC_IRQSTAT_CINT_MASK,       /*!< Card interrupt */
    kSDHC_CommandTimeoutFlag = SDHC_IRQSTAT_CTOE_MASK,      /*!< Command timeout error */
    kSDHC_CommandCrcErrorFlag = SDHC_IRQSTAT_CCE_MASK,      /*!< Command CRC error */
    kSDHC_CommandEndBitErrorFlag = SDHC_IRQSTAT_CEBE_MASK,  /*!< Command end bit error */
    kSDHC_CommandIndexErrorFlag = SDHC_IRQSTAT_CIE_MASK,    /*!< Command index error */
    kSDHC_DataTimeoutFlag = SDHC_IRQSTAT_DTOE_MASK,         /*!< Data timeout error */
    kSDHC_DataCrcErrorFlag = SDHC_IRQSTAT_DCE_MASK,         /*!< Data CRC error */
    kSDHC_DataEndBitErrorFlag = SDHC_IRQSTAT_DEBE_MASK,     /*!< Data end bit error */
    kSDHC_AutoCommand12ErrorFlag = SDHC_IRQSTAT_AC12E_MASK, /*!< Auto CMD12 error */
    kSDHC_DmaErrorFlag = SDHC_IRQSTAT_DMAE_MASK,            /*!< DMA error */

    kSDHC_CommandErrorFlag = (kSDHC_CommandTimeoutFlag | kSDHC_CommandCrcErrorFlag | kSDHC_CommandEndBitErrorFlag |
                              kSDHC_CommandIndexErrorFlag), /*!< Command error */
    kSDHC_DataErrorFlag = (kSDHC_DataTimeoutFlag | kSDHC_DataCrcErrorFlag | kSDHC_DataEndBitErrorFlag |
                           kSDHC_AutoCommand12ErrorFlag),                                  /*!< Data error */
    kSDHC_ErrorFlag = (kSDHC_CommandErrorFlag | kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag), /*!< All error */
    kSDHC_DataFlag = (kSDHC_DataCompleteFlag | kSDHC_DmaCompleteFlag | kSDHC_BufferWriteReadyFlag |
                      kSDHC_BufferReadReadyFlag | kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag), /*!< Data interrupts */
    kSDHC_CommandFlag = (kSDHC_CommandErrorFlag | kSDHC_CommandCompleteFlag),                /*!< Command interrupts */
    kSDHC_CardDetectFlag = (kSDHC_CardInsertionFlag | kSDHC_CardRemovalFlag), /*!< Card detection interrupts */

    kSDHC_AllInterruptFlags = (kSDHC_BlockGapEventFlag | kSDHC_CardInterruptFlag | kSDHC_CommandFlag | kSDHC_DataFlag |
                               kSDHC_ErrorFlag), /*!< All flags mask */
};

/*! @brief Auto CMD12 error status flag mask */
enum _sdhc_auto_command12_error_status_flag
{
    kSDHC_AutoCommand12NotExecutedFlag = SDHC_AC12ERR_AC12NE_MASK,  /*!< Not executed error */
    kSDHC_AutoCommand12TimeoutFlag = SDHC_AC12ERR_AC12TOE_MASK,     /*!< Timeout error */
    kSDHC_AutoCommand12EndBitErrorFlag = SDHC_AC12ERR_AC12EBE_MASK, /*!< End bit error */
    kSDHC_AutoCommand12CrcErrorFlag = SDHC_AC12ERR_AC12CE_MASK,     /*!< CRC error */
    kSDHC_AutoCommand12IndexErrorFlag = SDHC_AC12ERR_AC12IE_MASK,   /*!< Index error */
    kSDHC_AutoCommand12NotIssuedFlag = SDHC_AC12ERR_CNIBAC12E_MASK, /*!< Not issued error */
};

/*! @brief ADMA error status flag mask */
enum _sdhc_adma_error_status_flag
{
    kSDHC_AdmaLenghMismatchFlag = SDHC_ADMAES_ADMALME_MASK,   /*!< Length mismatch error */
    kSDHC_AdmaDescriptorErrorFlag = SDHC_ADMAES_ADMADCE_MASK, /*!< Descriptor error */
};

/*!
 * @brief ADMA error state
 *
 * This state is the detail state when ADMA error has occurred.
 */
typedef enum _sdhc_adma_error_state
{
    kSDHC_AdmaErrorStateStopDma = 0x00U,         /*!< Stop DMA */
    kSDHC_AdmaErrorStateFetchDescriptor = 0x01U, /*!< Fetch descriptor */
    kSDHC_AdmaErrorStateChangeAddress = 0x02U,   /*!< Change address */
    kSDHC_AdmaErrorStateTransferData = 0x03U,    /*!< Transfer data */
} sdhc_adma_error_state_t;

/*! @brief Force event mask */
enum _sdhc_force_event
{
    kSDHC_ForceEventAutoCommand12NotExecuted = SDHC_FEVT_AC12NE_MASK,  /*!< Auto CMD12 not executed error */
    kSDHC_ForceEventAutoCommand12Timeout = SDHC_FEVT_AC12TOE_MASK,     /*!< Auto CMD12 timeout error */
    kSDHC_ForceEventAutoCommand12CrcError = SDHC_FEVT_AC12CE_MASK,     /*!< Auto CMD12 CRC error */
    kSDHC_ForceEventEndBitError = SDHC_FEVT_AC12EBE_MASK,              /*!< Auto CMD12 end bit error */
    kSDHC_ForceEventAutoCommand12IndexError = SDHC_FEVT_AC12IE_MASK,   /*!< Auto CMD12 index error */
    kSDHC_ForceEventAutoCommand12NotIssued = SDHC_FEVT_CNIBAC12E_MASK, /*!< Auto CMD12 not issued error */
    kSDHC_ForceEventCommandTimeout = SDHC_FEVT_CTOE_MASK,              /*!< Command timeout error */
    kSDHC_ForceEventCommandCrcError = SDHC_FEVT_CCE_MASK,              /*!< Command CRC error */
    kSDHC_ForceEventCommandEndBitError = SDHC_FEVT_CEBE_MASK,          /*!< Command end bit error */
    kSDHC_ForceEventCommandIndexError = SDHC_FEVT_CIE_MASK,            /*!< Command index error */
    kSDHC_ForceEventDataTimeout = SDHC_FEVT_DTOE_MASK,                 /*!< Data timeout error */
    kSDHC_ForceEventDataCrcError = SDHC_FEVT_DCE_MASK,                 /*!< Data CRC error */
    kSDHC_ForceEventDataEndBitError = SDHC_FEVT_DEBE_MASK,             /*!< Data end bit error */
    kSDHC_ForceEventAutoCommand12Error = SDHC_FEVT_AC12E_MASK,         /*!< Auto CMD12 error */
    kSDHC_ForceEventCardInt = SDHC_FEVT_CINT_MASK,                     /*!< Card interrupt */
    kSDHC_ForceEventDmaError = SDHC_FEVT_DMAE_MASK,                    /*!< Dma error */

    kSDHC_ForceEventsAll =
        (kSDHC_ForceEventAutoCommand12NotExecuted | kSDHC_ForceEventAutoCommand12Timeout |
         kSDHC_ForceEventAutoCommand12CrcError | kSDHC_ForceEventEndBitError | kSDHC_ForceEventAutoCommand12IndexError |
         kSDHC_ForceEventAutoCommand12NotIssued | kSDHC_ForceEventCommandTimeout | kSDHC_ForceEventCommandCrcError |
         kSDHC_ForceEventCommandEndBitError | kSDHC_ForceEventCommandIndexError | kSDHC_ForceEventDataTimeout |
         kSDHC_ForceEventDataCrcError | kSDHC_ForceEventDataEndBitError | kSDHC_ForceEventAutoCommand12Error |
         kSDHC_ForceEventCardInt | kSDHC_ForceEventDmaError), /*!< All force event flags mask */
};

/*! @brief Data transfer width */
typedef enum _sdhc_data_bus_width
{
    kSDHC_DataBusWidth1Bit = 0U, /*!< 1-bit mode */
    kSDHC_DataBusWidth4Bit = 1U, /*!< 4-bit mode */
    kSDHC_DataBusWidth8Bit = 2U, /*!< 8-bit mode */
} sdhc_data_bus_width_t;

/*! @brief Endian mode */
typedef enum _sdhc_endian_mode
{
    kSDHC_EndianModeBig = 0U,         /*!< Big endian mode */
    kSDHC_EndianModeHalfWordBig = 1U, /*!< Half word big endian mode */
    kSDHC_EndianModeLittle = 2U,      /*!< Little endian mode */
} sdhc_endian_mode_t;

/*! @brief DMA mode */
typedef enum _sdhc_dma_mode
{
    kSDHC_DmaModeNo = 0U,    /*!< No DMA */
    kSDHC_DmaModeAdma1 = 1U, /*!< ADMA1 is selected */
    kSDHC_DmaModeAdma2 = 2U, /*!< ADMA2 is selected */
} sdhc_dma_mode_t;

/*! @brief SDIO control flag mask */
enum _sdhc_sdio_control_flag
{
    kSDHC_StopAtBlockGapFlag = 0x01,       /*!< Stop at block gap */
    kSDHC_ReadWaitControlFlag = 0x02,      /*!< Read wait control */
    kSDHC_InterruptAtBlockGapFlag = 0x04,  /*!< Interrupt at block gap */
    kSDHC_ExactBlockNumberReadFlag = 0x08, /*!< Exact block number read */
};

/*! @brief MMC card boot mode */
typedef enum _sdhc_boot_mode
{
    kSDHC_BootModeNormal = 0U,      /*!< Normal boot */
    kSDHC_BootModeAlternative = 1U, /*!< Alternative boot */
} sdhc_boot_mode_t;

/*! @brief The command type */
typedef enum _sdhc_card_command_type
{
    kCARD_CommandTypeNormal = 0U,  /*!< Normal command */
    kCARD_CommandTypeSuspend = 1U, /*!< Suspend command */
    kCARD_CommandTypeResume = 2U,  /*!< Resume command */
    kCARD_CommandTypeAbort = 3U,   /*!< Abort command */
} sdhc_card_command_type_t;

/*!
 * @brief The command response type.
 *
 * Define the command response type from card to host controller.
 */
typedef enum _sdhc_card_response_type
{
    kCARD_ResponseTypeNone = 0U, /*!< Response type: none */
    kCARD_ResponseTypeR1 = 1U,   /*!< Response type: R1 */
    kCARD_ResponseTypeR1b = 2U,  /*!< Response type: R1b */
    kCARD_ResponseTypeR2 = 3U,   /*!< Response type: R2 */
    kCARD_ResponseTypeR3 = 4U,   /*!< Response type: R3 */
    kCARD_ResponseTypeR4 = 5U,   /*!< Response type: R4 */
    kCARD_ResponseTypeR5 = 6U,   /*!< Response type: R5 */
    kCARD_ResponseTypeR5b = 7U,  /*!< Response type: R5b */
    kCARD_ResponseTypeR6 = 8U,   /*!< Response type: R6 */
    kCARD_ResponseTypeR7 = 9U,   /*!< Response type: R7 */
} sdhc_card_response_type_t;

/*! @brief The alignment size for ADDRESS filed in ADMA1's descriptor */
#define SDHC_ADMA1_ADDRESS_ALIGN (4096U)
/*! @brief The alignment size for LENGTH field in ADMA1's descriptor */
#define SDHC_ADMA1_LENGTH_ALIGN (4096U)
/*! @brief The alignment size for ADDRESS field in ADMA2's descriptor */
#define SDHC_ADMA2_ADDRESS_ALIGN (4U)
/*! @brief The alignment size for LENGTH filed in ADMA2's descriptor */
#define SDHC_ADMA2_LENGTH_ALIGN (4U)

/* ADMA1 descriptor table
 * |------------------------|---------|--------------------------|
 * | Address/page field     |Reserved |         Attribute        |
 * |------------------------|---------|--------------------------|
 * |31                    12|11      6|05  |04  |03|02 |01 |00   |
 * |------------------------|---------|----|----|--|---|---|-----|
 * | address or data length | 000000  |Act2|Act1| 0|Int|End|Valid|
 * |------------------------|---------|----|----|--|---|---|-----|
 *
 *
 * |------|------|-----------------|-------|-------------|
 * | Act2 | Act1 |     Comment     | 31-28 | 27 - 12     |
 * |------|------|-----------------|---------------------|
 * |   0  |   0  | No op           | Don't care          |
 * |------|------|-----------------|-------|-------------|
 * |   0  |   1  | Set data length |  0000 | Data Length |
 * |------|------|-----------------|-------|-------------|
 * |   1  |   0  | Transfer data   | Data address        |
 * |------|------|-----------------|---------------------|
 * |   1  |   1  | Link descriptor | Descriptor address  |
 * |------|------|-----------------|---------------------|
 */
/*! @brief The bit shift for ADDRESS filed in ADMA1's descriptor */
#define SDHC_ADMA1_DESCRIPTOR_ADDRESS_SHIFT (12U)
/*! @brief The bit mask for ADDRESS field in ADMA1's descriptor */
#define SDHC_ADMA1_DESCRIPTOR_ADDRESS_MASK (0xFFFFFU)
/*! @brief The bit shift for LENGTH filed in ADMA1's descriptor */
#define SDHC_ADMA1_DESCRIPTOR_LENGTH_SHIFT (12U)
/*! @brief The mask for LENGTH field in ADMA1's descriptor */
#define SDHC_ADMA1_DESCRIPTOR_LENGTH_MASK (0xFFFFU)
/*! @brief The maximum value of LENGTH filed in ADMA1's descriptor */
#define SDHC_ADMA1_DESCRIPTOR_MAX_LENGTH_PER_ENTRY (SDHC_ADMA1_DESCRIPTOR_LENGTH_MASK + 1U)

/*! @brief The mask for the control/status field in ADMA1 descriptor */
enum _sdhc_adma1_descriptor_flag
{
    kSDHC_Adma1DescriptorValidFlag = (1U << 0U),                     /*!< Valid flag */
    kSDHC_Adma1DescriptorEndFlag = (1U << 1U),                       /*!< End flag */
    kSDHC_Adma1DescriptorInterrupFlag = (1U << 2U),                  /*!< Interrupt flag */
    kSDHC_Adma1DescriptorActivity1Flag = (1U << 4U),                 /*!< Activity 1 flag */
    kSDHC_Adma1DescriptorActivity2Flag = (1U << 5U),                 /*!< Activity 2 flag */
    kSDHC_Adma1DescriptorTypeNop = (kSDHC_Adma1DescriptorValidFlag), /*!< No operation */
    kSDHC_Adma1DescriptorTypeTransfer =
        (kSDHC_Adma1DescriptorActivity2Flag | kSDHC_Adma1DescriptorValidFlag), /*!< Transfer data */
    kSDHC_Adma1DescriptorTypeLink = (kSDHC_Adma1DescriptorActivity1Flag | kSDHC_Adma1DescriptorActivity2Flag |
                                     kSDHC_Adma1DescriptorValidFlag), /*!< Link descriptor */
    kSDHC_Adma1DescriptorTypeSetLength =
        (kSDHC_Adma1DescriptorActivity1Flag | kSDHC_Adma1DescriptorValidFlag), /*!< Set data length */
};

/* ADMA2 descriptor table
 * |----------------|---------------|-------------|--------------------------|
 * | Address field  |     Length    | Reserved    |         Attribute        |
 * |----------------|---------------|-------------|--------------------------|
 * |63            32|31           16|15         06|05  |04  |03|02 |01 |00   |
 * |----------------|---------------|-------------|----|----|--|---|---|-----|
 * | 32-bit address | 16-bit length | 0000000000  |Act2|Act1| 0|Int|End|Valid|
 * |----------------|---------------|-------------|----|----|--|---|---|-----|
 *
 *
 * | Act2 | Act1 |     Comment     | Operation                                                         |
 * |------|------|-----------------|-------------------------------------------------------------------|
 * |   0  |   0  | No op           | Don't care                                                        |
 * |------|------|-----------------|-------------------------------------------------------------------|
 * |   0  |   1  | Reserved        | Read this line and go to next one                                 |
 * |------|------|-----------------|-------------------------------------------------------------------|
 * |   1  |   0  | Transfer data   | Transfer data with address and length set in this descriptor line |
 * |------|------|-----------------|-------------------------------------------------------------------|
 * |   1  |   1  | Link descriptor | Link to another descriptor                                        |
 * |------|------|-----------------|-------------------------------------------------------------------|
 */
/*! @brief The bit shift for LENGTH field in ADMA2's descriptor */
#define SDHC_ADMA2_DESCRIPTOR_LENGTH_SHIFT (16U)
/*! @brief The bit mask for LENGTH field in ADMA2's descriptor */
#define SDHC_ADMA2_DESCRIPTOR_LENGTH_MASK (0xFFFFU)
/*! @brief The maximum value of LENGTH field in ADMA2's descriptor */
#define SDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY (SDHC_ADMA2_DESCRIPTOR_LENGTH_MASK)

/*! @brief ADMA1 descriptor control and status mask */
enum _sdhc_adma2_descriptor_flag
{
    kSDHC_Adma2DescriptorValidFlag = (1U << 0U),     /*!< Valid flag */
    kSDHC_Adma2DescriptorEndFlag = (1U << 1U),       /*!< End flag */
    kSDHC_Adma2DescriptorInterruptFlag = (1U << 2U), /*!< Interrupt flag */
    kSDHC_Adma2DescriptorActivity1Flag = (1U << 4U), /*!< Activity 1 mask */
    kSDHC_Adma2DescriptorActivity2Flag = (1U << 5U), /*!< Activity 2 mask */

    kSDHC_Adma2DescriptorTypeNop = (kSDHC_Adma2DescriptorValidFlag), /*!< No operation */
    kSDHC_Adma2DescriptorTypeReserved =
        (kSDHC_Adma2DescriptorActivity1Flag | kSDHC_Adma2DescriptorValidFlag), /*!< Reserved */
    kSDHC_Adma2DescriptorTypeTransfer =
        (kSDHC_Adma2DescriptorActivity2Flag | kSDHC_Adma2DescriptorValidFlag), /*!< Transfer type */
    kSDHC_Adma2DescriptorTypeLink = (kSDHC_Adma2DescriptorActivity1Flag | kSDHC_Adma2DescriptorActivity2Flag |
                                     kSDHC_Adma2DescriptorValidFlag), /*!< Link type */
};

/*! @brief Defines the adma1 descriptor structure. */
typedef uint32_t sdhc_adma1_descriptor_t;

/*! @brief Defines the ADMA2 descriptor structure. */
typedef struct _sdhc_adma2_descriptor
{
    uint32_t attribute;      /*!< The control and status field */
    const uint32_t *address; /*!< The address field */
} sdhc_adma2_descriptor_t;

/*!
 * @brief SDHC capability information.
 *
 * Defines a structure to save the capability information of SDHC.
 */
typedef struct _sdhc_capability
{
    uint32_t specVersion;    /*!< Specification version */
    uint32_t vendorVersion;  /*!< Vendor version */
    uint32_t maxBlockLength; /*!< Maximum block length united as byte */
    uint32_t maxBlockCount;  /*!< Maximum block count can be set one time */
    uint32_t flags;          /*!< Capability flags to indicate the support information(_sdhc_capability_flag) */
} sdhc_capability_t;

/*! @brief Card transfer configuration.
 *
 * Define structure to configure the transfer-related command index/argument/flags and data block
 * size/data block numbers. This structure needs to be filled each time a command is sent to the card.
 */
typedef struct _sdhc_transfer_config
{
    size_t dataBlockSize;     /*!< Data block size */
    uint32_t dataBlockCount;  /*!< Data block count */
    uint32_t commandArgument; /*!< Command argument */
    uint32_t commandIndex;    /*!< Command index */
    uint32_t flags;           /*!< Transfer flags(_sdhc_transfer_flag) */
} sdhc_transfer_config_t;

/*! @brief Data structure to configure the MMC boot feature */
typedef struct _sdhc_boot_config
{
    uint32_t ackTimeoutCount;      /*!< Timeout value for the boot ACK. The available range is 0 ~ 15. */
    sdhc_boot_mode_t bootMode;     /*!< Boot mode selection. */
    uint32_t blockCount;           /*!< Stop at block gap value of automatic mode. Available range is 0 ~ 65535. */
    bool enableBootAck;            /*!< Enable or disable boot ACK */
    bool enableBoot;               /*!< Enable or disable fast boot */
    bool enableAutoStopAtBlockGap; /*!< Enable or disable auto stop at block gap function in boot period */
} sdhc_boot_config_t;

/*! @brief Data structure to initialize the SDHC */
typedef struct _sdhc_config
{
    bool cardDetectDat3;           /*!< Enable DAT3 as card detection pin */
    sdhc_endian_mode_t endianMode; /*!< Endian mode */
    sdhc_dma_mode_t dmaMode;       /*!< DMA mode */
    uint32_t readWatermarkLevel;   /*!< Watermark level for DMA read operation. Available range is 1 ~ 128. */
    uint32_t writeWatermarkLevel;  /*!< Watermark level for DMA write operation. Available range is 1 ~ 128. */
} sdhc_config_t;

/*!
 * @brief Card data descriptor
 *
 * Defines a structure to contain data-related attribute. 'enableIgnoreError' is used for the case that upper card
 * driver
 * want to ignore the error event to read/write all the data not to stop read/write immediately when error event
 * happen for example bus testing procedure for MMC card.
 */
typedef struct _sdhc_data
{
    bool enableAutoCommand12; /*!< Enable auto CMD12 */
    bool enableIgnoreError;   /*!< Enable to ignore error event to read/write all the data */
    size_t blockSize;         /*!< Block size */
    uint32_t blockCount;      /*!< Block count */
    uint32_t *rxData;         /*!< Buffer to save data read */
    const uint32_t *txData;   /*!< Data buffer to write */
} sdhc_data_t;

/*!
 * @brief Card command descriptor
 *
 * Define card command-related attribute.
 */
typedef struct _sdhc_command
{
    uint32_t index;                         /*!< Command index */
    uint32_t argument;                      /*!< Command argument */
    sdhc_card_command_type_t type;          /*!< Command type */
    sdhc_card_response_type_t responseType; /*!< Command response type */
    uint32_t response[4U];                  /*!< Response for this command */
    uint32_t responseErrorFlags;            /*!< response error flag, the flag which need to check
                                                the command reponse*/
} sdhc_command_t;

/*! @brief Transfer state */
typedef struct _sdhc_transfer
{
    sdhc_data_t *data;       /*!< Data to transfer */
    sdhc_command_t *command; /*!< Command to send */
} sdhc_transfer_t;

/*! @brief SDHC handle typedef */
typedef struct _sdhc_handle sdhc_handle_t;

/*! @brief SDHC callback functions. */
typedef struct _sdhc_transfer_callback
{
    void (*CardInserted)(void);  /*!< Card inserted occurs when DAT3/CD pin is for card detect */
    void (*CardRemoved)(void);   /*!< Card removed occurs */
    void (*SdioInterrupt)(void); /*!< SDIO card interrupt occurs */
    void (*SdioBlockGap)(void);  /*!< SDIO card stopped at block gap occurs */
    void (*TransferComplete)(SDHC_Type *base,
                             sdhc_handle_t *handle,
                             status_t status,
                             void *userData); /*!< Transfer complete callback */
} sdhc_transfer_callback_t;

/*!
 * @brief SDHC handle
 *
 * Defines the structure to save the SDHC state information and callback function. The detailed interrupt status when
 * sending a command or transfering data can be obtained from the interruptFlags field by using the mask defined in
 * sdhc_interrupt_flag_t.
 *
 * @note All the fields except interruptFlags and transferredWords must be allocated by the user.
 */
struct _sdhc_handle
{
    /* Transfer parameter */
    sdhc_data_t *volatile data;       /*!< Data to transfer */
    sdhc_command_t *volatile command; /*!< Command to send */

    /* Transfer status */
    volatile uint32_t interruptFlags;   /*!< Interrupt flags of last transaction */
    volatile uint32_t transferredWords; /*!< Words transferred by DATAPORT way */

    /* Callback functions */
    sdhc_transfer_callback_t callback; /*!< Callback function */
    void *userData;                    /*!< Parameter for transfer complete callback */
};

/*! @brief SDHC transfer function. */
typedef status_t (*sdhc_transfer_function_t)(SDHC_Type *base, sdhc_transfer_t *content);

/*! @brief SDHC host descriptor */
typedef struct _sdhc_host
{
    SDHC_Type *base;                   /*!< SDHC peripheral base address */
    uint32_t sourceClock_Hz;           /*!< SDHC source clock frequency united in Hz */
    sdhc_config_t config;              /*!< SDHC configuration */
    sdhc_capability_t capability;      /*!< SDHC capability information */
    sdhc_transfer_function_t transfer; /*!< SDHC transfer function */
} sdhc_host_t;

/*************************************************************************************************
 * API
 ************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief SDHC module initialization function.
 *
 * Configures the SDHC according to the user configuration.
 *
 * Example:
   @code
   sdhc_config_t config;
   config.cardDetectDat3 = false;
   config.endianMode = kSDHC_EndianModeLittle;
   config.dmaMode = kSDHC_DmaModeAdma2;
   config.readWatermarkLevel = 128U;
   config.writeWatermarkLevel = 128U;
   SDHC_Init(SDHC, &config);
   @endcode
 *
 * @param base SDHC peripheral base address.
 * @param config SDHC configuration information.
 * @retval kStatus_Success Operate successfully.
 */
void SDHC_Init(SDHC_Type *base, const sdhc_config_t *config);

/*!
 * @brief Deinitializes the SDHC.
 *
 * @param base SDHC peripheral base address.
 */
void SDHC_Deinit(SDHC_Type *base);

/*!
 * @brief Resets the SDHC.
 *
 * @param base SDHC peripheral base address.
 * @param mask The reset type mask(_sdhc_reset).
 * @param timeout Timeout for reset.
 * @retval true Reset successfully.
 * @retval false Reset failed.
 */
bool SDHC_Reset(SDHC_Type *base, uint32_t mask, uint32_t timeout);

/* @} */

/*!
 * @name DMA Control
 * @{
 */

/*!
 * @brief Sets the ADMA descriptor table configuration.
 *
 * @param base SDHC peripheral base address.
 * @param dmaMode DMA mode.
 * @param table ADMA table address.
 * @param tableWords ADMA table buffer length united as Words.
 * @param data Data buffer address.
 * @param dataBytes Data length united as bytes.
 * @retval kStatus_OutOfRange ADMA descriptor table length isn't enough to describe data.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDHC_SetAdmaTableConfig(SDHC_Type *base,
                                 sdhc_dma_mode_t dmaMode,
                                 uint32_t *table,
                                 uint32_t tableWords,
                                 const uint32_t *data,
                                 uint32_t dataBytes);

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables the interrupt status.
 *
 * @param base SDHC peripheral base address.
 * @param mask Interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline void SDHC_EnableInterruptStatus(SDHC_Type *base, uint32_t mask)
{
    base->IRQSTATEN |= mask;
}

/*!
 * @brief Disables the interrupt status.
 *
 * @param base SDHC peripheral base address.
 * @param mask The interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline void SDHC_DisableInterruptStatus(SDHC_Type *base, uint32_t mask)
{
    base->IRQSTATEN &= ~mask;
}

/*!
 * @brief Enables the interrupt signal corresponding to the interrupt status flag.
 *
 * @param base SDHC peripheral base address.
 * @param mask The interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline void SDHC_EnableInterruptSignal(SDHC_Type *base, uint32_t mask)
{
    base->IRQSIGEN |= mask;
}

/*!
 * @brief Disables the interrupt signal corresponding to the interrupt status flag.
 *
 * @param base SDHC peripheral base address.
 * @param mask The interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline void SDHC_DisableInterruptSignal(SDHC_Type *base, uint32_t mask)
{
    base->IRQSIGEN &= ~mask;
}

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets the current interrupt status.
 *
 * @param base SDHC peripheral base address.
 * @return Current interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline uint32_t SDHC_GetInterruptStatusFlags(SDHC_Type *base)
{
    return base->IRQSTAT;
}

/*!
 * @brief Clears a specified interrupt status.
 *
 * @param base SDHC peripheral base address.
 * @param mask The interrupt status flags mask(_sdhc_interrupt_status_flag).
 */
static inline void SDHC_ClearInterruptStatusFlags(SDHC_Type *base, uint32_t mask)
{
    base->IRQSTAT = mask;
}

/*!
 * @brief Gets the status of auto command 12 error.
 *
 * @param base SDHC peripheral base address.
 * @return Auto command 12 error status flags mask(_sdhc_auto_command12_error_status_flag).
 */
static inline uint32_t SDHC_GetAutoCommand12ErrorStatusFlags(SDHC_Type *base)
{
    return base->AC12ERR;
}

/*!
 * @brief Gets the status of the ADMA error.
 *
 * @param base SDHC peripheral base address.
 * @return ADMA error status flags mask(_sdhc_adma_error_status_flag).
 */
static inline uint32_t SDHC_GetAdmaErrorStatusFlags(SDHC_Type *base)
{
    return base->ADMAES;
}

/*!
 * @brief Gets a present status.
 *
 * This function gets the present SDHC's status except for an interrupt status and an error status.
 *
 * @param base SDHC peripheral base address.
 * @return Present SDHC's status flags mask(_sdhc_present_status_flag).
 */
static inline uint32_t SDHC_GetPresentStatusFlags(SDHC_Type *base)
{
    return base->PRSSTAT;
}

/* @} */

/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Gets the capability information.
 *
 * @param base SDHC peripheral base address.
 * @param capability Structure to save capability information.
 */
void SDHC_GetCapability(SDHC_Type *base, sdhc_capability_t *capability);

/*!
 * @brief Enables or disables the SD bus clock.
 *
 * @param base SDHC peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void SDHC_EnableSdClock(SDHC_Type *base, bool enable)
{
    if (enable)
    {
        base->SYSCTL |= SDHC_SYSCTL_SDCLKEN_MASK;
    }
    else
    {
        base->SYSCTL &= ~SDHC_SYSCTL_SDCLKEN_MASK;
    }
}

/*!
 * @brief Sets the SD bus clock frequency.
 *
 * @param base SDHC peripheral base address.
 * @param srcClock_Hz SDHC source clock frequency united in Hz.
 * @param busClock_Hz SD bus clock frequency united in Hz.
 *
 * @return The nearest frequency of busClock_Hz configured to SD bus.
 */
uint32_t SDHC_SetSdClock(SDHC_Type *base, uint32_t srcClock_Hz, uint32_t busClock_Hz);

/*!
 * @brief Sends 80 clocks to the card to set it to the active state.
 *
 * This function must be called each time the card is inserted to ensure that the card can receive the command
 * correctly.
 *
 * @param base SDHC peripheral base address.
 * @param timeout Timeout to initialize card.
 * @retval true Set card active successfully.
 * @retval false Set card active failed.
 */
bool SDHC_SetCardActive(SDHC_Type *base, uint32_t timeout);

/*!
 * @brief Sets the data transfer width.
 *
 * @param base SDHC peripheral base address.
 * @param width Data transfer width.
 */
static inline void SDHC_SetDataBusWidth(SDHC_Type *base, sdhc_data_bus_width_t width)
{
    base->PROCTL = ((base->PROCTL & ~SDHC_PROCTL_DTW_MASK) | SDHC_PROCTL_DTW(width));
}

/*!
 * @brief Sets the card transfer-related configuration.
 *
 * This function fills the card transfer-related command argument/transfer flag/data size. The command and data are sent
 by
 * SDHC after calling this function.
 *
 * Example:
   @code
   sdhc_transfer_config_t transferConfig;
   transferConfig.dataBlockSize = 512U;
   transferConfig.dataBlockCount = 2U;
   transferConfig.commandArgument = 0x01AAU;
   transferConfig.commandIndex = 8U;
   transferConfig.flags |= (kSDHC_EnableDmaFlag | kSDHC_EnableAutoCommand12Flag | kSDHC_MultipleBlockFlag);
   SDHC_SetTransferConfig(SDHC, &transferConfig);
   @endcode
 *
 * @param base SDHC peripheral base address.
 * @param config Command configuration structure.
 */
void SDHC_SetTransferConfig(SDHC_Type *base, const sdhc_transfer_config_t *config);

/*!
 * @brief Gets the command response.
 *
 * @param base SDHC peripheral base address.
 * @param index The index of response register, range from 0 to 3.
 * @return Response register transfer.
 */
static inline uint32_t SDHC_GetCommandResponse(SDHC_Type *base, uint32_t index)
{
    assert(index < 4U);

    return base->CMDRSP[index];
}

/*!
 * @brief Fills the the data port.
 *
 * This function is used to implement the data transfer by Data Port instead of DMA.
 *
 * @param base SDHC peripheral base address.
 * @param data The data about to be sent.
 */
static inline void SDHC_WriteData(SDHC_Type *base, uint32_t data)
{
    base->DATPORT = data;
}

/*!
 * @brief Retrieves the data from the data port.
 *
 * This function is used to implement the data transfer by Data Port instead of DMA.
 *
 * @param base SDHC peripheral base address.
 * @return The data has been read.
 */
static inline uint32_t SDHC_ReadData(SDHC_Type *base)
{
    return base->DATPORT;
}

/*!
 * @brief Enables or disables a wakeup event in low-power mode.
 *
 * @param base SDHC peripheral base address.
 * @param mask Wakeup events mask(_sdhc_wakeup_event).
 * @param enable True to enable, false to disable.
 */
static inline void SDHC_EnableWakeupEvent(SDHC_Type *base, uint32_t mask, bool enable)
{
    if (enable)
    {
        base->PROCTL |= mask;
    }
    else
    {
        base->PROCTL &= ~mask;
    }
}

/*!
 * @brief Enables or disables the card detection level for testing.
 *
 * @param base SDHC peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void SDHC_EnableCardDetectTest(SDHC_Type *base, bool enable)
{
    if (enable)
    {
        base->PROCTL |= SDHC_PROCTL_CDSS_MASK;
    }
    else
    {
        base->PROCTL &= ~SDHC_PROCTL_CDSS_MASK;
    }
}

/*!
 * @brief Sets the card detection test level.
 *
 * This function sets the card detection test level to indicate whether the card is inserted into the SDHC when DAT[3]/
 * CD pin is selected as a card detection pin. This function can also assert the pin logic when DAT[3]/CD pin is
 * selected
 * as the card detection pin.
 *
 * @param base SDHC peripheral base address.
 * @param high True to set the card detect level to high.
 */
static inline void SDHC_SetCardDetectTestLevel(SDHC_Type *base, bool high)
{
    if (high)
    {
        base->PROCTL |= SDHC_PROCTL_CDTL_MASK;
    }
    else
    {
        base->PROCTL &= ~SDHC_PROCTL_CDTL_MASK;
    }
}

/*!
 * @brief Enables or disables the SDIO card control.
 *
 * @param base SDHC peripheral base address.
 * @param mask SDIO card control flags mask(_sdhc_sdio_control_flag).
 * @param enable True to enable, false to disable.
 */
void SDHC_EnableSdioControl(SDHC_Type *base, uint32_t mask, bool enable);

/*!
 * @brief Restarts a transaction which has stopped at the block GAP for the SDIO card.
 *
 * @param base SDHC peripheral base address.
 */
static inline void SDHC_SetContinueRequest(SDHC_Type *base)
{
    base->PROCTL |= SDHC_PROCTL_CREQ_MASK;
}

/*!
 * @brief Configures the MMC boot feature.
 *
 * Example:
   @code
   sdhc_boot_config_t config;
   config.ackTimeoutCount = 4;
   config.bootMode = kSDHC_BootModeNormal;
   config.blockCount = 5;
   config.enableBootAck = true;
   config.enableBoot = true;
   config.enableAutoStopAtBlockGap = true;
   SDHC_SetMmcBootConfig(SDHC, &config);
   @endcode
 *
 * @param base SDHC peripheral base address.
 * @param config The MMC boot configuration information.
 */
void SDHC_SetMmcBootConfig(SDHC_Type *base, const sdhc_boot_config_t *config);

/*!
 * @brief Forces generating events according to the given mask.
 *
 * @param base SDHC peripheral base address.
 * @param mask The force events mask(_sdhc_force_event).
 */
static inline void SDHC_SetForceEvent(SDHC_Type *base, uint32_t mask)
{
    base->FEVT = mask;
}

/* @} */

/*!
 * @name Transactional
 * @{
 */

/*!
 * @brief Transfers the command/data using a blocking method.
 *
 * This function waits until the command response/data is received or the SDHC encounters an error by polling the status
 * flag.
 * This function support non word align data addr transfer support, if data buffer addr is not align in DMA mode,
 * the API will continue finish the transfer by polling IO directly
 * The application must not call this API in multiple threads at the same time. Because of that this API doesn't support
 * the re-entry mechanism.
 *
 * @note There is no need to call the API 'SDHC_TransferCreateHandle' when calling this API.
 *
 * @param base SDHC peripheral base address.
 * @param admaTable ADMA table address, can't be null if transfer way is ADMA1/ADMA2.
 * @param admaTableWords ADMA table length united as words, can't be 0 if transfer way is ADMA1/ADMA2.
 * @param transfer Transfer content.
 * @retval kStatus_InvalidArgument Argument is invalid.
 * @retval kStatus_SDHC_PrepareAdmaDescriptorFailed Prepare ADMA descriptor failed.
 * @retval kStatus_SDHC_SendCommandFailed Send command failed.
 * @retval kStatus_SDHC_TransferDataFailed Transfer data failed.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDHC_TransferBlocking(SDHC_Type *base,
                               uint32_t *admaTable,
                               uint32_t admaTableWords,
                               sdhc_transfer_t *transfer);

/*!
 * @brief Creates the SDHC handle.
 *
 * @param base SDHC peripheral base address.
 * @param handle SDHC handle pointer.
 * @param callback Structure pointer to contain all callback functions.
 * @param userData Callback function parameter.
 */
void SDHC_TransferCreateHandle(SDHC_Type *base,
                               sdhc_handle_t *handle,
                               const sdhc_transfer_callback_t *callback,
                               void *userData);

/*!
 * @brief Transfers the command/data using an interrupt and an asynchronous method.
 *
 * This function sends a command and data and returns immediately. It doesn't wait the transfer complete or encounter an
 * error.
 * This function support non word align data addr transfer support, if data buffer addr is not align in DMA mode,
 * the API will continue finish the transfer by polling IO directly
 * The application must not call this API in multiple threads at the same time. Because of that this API doesn't support
 * the re-entry mechanism.
 *
 * @note Call the API 'SDHC_TransferCreateHandle' when calling this API.
 *
 * @param base SDHC peripheral base address.
 * @param handle SDHC handle.
 * @param admaTable ADMA table address, can't be null if transfer way is ADMA1/ADMA2.
 * @param admaTableWords ADMA table length united as words, can't be 0 if transfer way is ADMA1/ADMA2.
 * @param transfer Transfer content.
 * @retval kStatus_InvalidArgument Argument is invalid.
 * @retval kStatus_SDHC_BusyTransferring Busy transferring.
 * @retval kStatus_SDHC_PrepareAdmaDescriptorFailed Prepare ADMA descriptor failed.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDHC_TransferNonBlocking(
    SDHC_Type *base, sdhc_handle_t *handle, uint32_t *admaTable, uint32_t admaTableWords, sdhc_transfer_t *transfer);

/*!
 * @brief IRQ handler for the SDHC.
 *
 * This function deals with the IRQs on the given host controller.
 *
 * @param base SDHC peripheral base address.
 * @param handle SDHC handle.
 */
void SDHC_TransferHandleIRQ(SDHC_Type *base, sdhc_handle_t *handle);

/* @} */

#if defined(__cplusplus)
}
#endif
/*! @} */

#endif /* _FSL_SDHC_H_*/
