/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this
 * list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice,
 * this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_sdhc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Clock setting */
/* Max SD clock divisor from base clock */
#define SDHC_MAX_DVS ((SDHC_SYSCTL_DVS_MASK >> SDHC_SYSCTL_DVS_SHIFT) + 1U)
#define SDHC_INITIAL_DVS (1U)   /* Initial value of SD clock divisor */
#define SDHC_INITIAL_CLKFS (2U) /* Initial value of SD clock frequency selector */
#define SDHC_NEXT_DVS(x) ((x) += 1U)
#define SDHC_PREV_DVS(x) ((x) -= 1U)
#define SDHC_MAX_CLKFS ((SDHC_SYSCTL_SDCLKFS_MASK >> SDHC_SYSCTL_SDCLKFS_SHIFT) + 1U)
#define SDHC_NEXT_CLKFS(x) ((x) <<= 1U)
#define SDHC_PREV_CLKFS(x) ((x) >>= 1U)

/* Typedef for interrupt handler. */
typedef void (*sdhc_isr_t)(SDHC_Type *base, sdhc_handle_t *handle);

/*! @brief ADMA table configuration */
typedef struct _sdhc_adma_table_config
{
    uint32_t *admaTable;     /*!< ADMA table address, can't be null if transfer way is ADMA1/ADMA2 */
    uint32_t admaTableWords; /*!< ADMA table length united as words, can't be 0 if transfer way is ADMA1/ADMA2 */
} sdhc_adma_table_config_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance.
 *
 * @param base SDHC peripheral base address.
 * @return Instance number.
 */
static uint32_t SDHC_GetInstance(SDHC_Type *base);

/*!
 * @brief Set transfer interrupt.
 *
 * @param base SDHC peripheral base address.
 * @param usingInterruptSignal True to use IRQ signal.
 */
static void SDHC_SetTransferInterrupt(SDHC_Type *base, bool usingInterruptSignal);

/*!
 * @brief Start transfer according to current transfer state
 *
 * @param base SDHC peripheral base address.
 * @param command Command to be sent.
 * @param data Data to be transferred.
 */
static void SDHC_StartTransfer(SDHC_Type *base, sdhc_command_t *command, sdhc_data_t *data);

/*!
 * @brief Receive command response
 *
 * @param base SDHC peripheral base address.
 * @param command Command to be sent.
 */
static void SDHC_ReceiveCommandResponse(SDHC_Type *base, sdhc_command_t *command);

/*!
 * @brief Read DATAPORT when buffer enable bit is set.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be read.
 * @param transferredWords The number of data words have been transferred last time transaction.
 * @return The number of total data words have been transferred after this time transaction.
 */
static uint32_t SDHC_ReadDataPort(SDHC_Type *base, sdhc_data_t *data, uint32_t transferredWords);

/*!
 * @brief Read data by using DATAPORT polling way.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be read.
 * @retval kStatus_Fail Read DATAPORT failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_ReadByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data);

/*!
 * @brief Write DATAPORT when buffer enable bit is set.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be read.
 * @param transferredWords The number of data words have been transferred last time.
 * @return The number of total data words have been transferred after this time transaction.
 */
static uint32_t SDHC_WriteDataPort(SDHC_Type *base, sdhc_data_t *data, uint32_t transferredWords);

/*!
 * @brief Write data by using DATAPORT polling way.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be transferred.
 * @retval kStatus_Fail Write DATAPORT failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_WriteByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data);

/*!
 * @brief Send command by using polling way.
 *
 * @param base SDHC peripheral base address.
 * @param command Command to be sent.
 * @retval kStatus_Fail Send command failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_SendCommandBlocking(SDHC_Type *base, sdhc_command_t *command);

/*!
 * @brief Transfer data by DATAPORT and polling way.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be transferred.
 * @retval kStatus_Fail Transfer data failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_TransferByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data);

/*!
 * @brief Transfer data by ADMA2 and polling way.
 *
 * @param base SDHC peripheral base address.
 * @param data Data to be transferred.
 * @retval kStatus_Fail Transfer data failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_TransferByAdma2Blocking(SDHC_Type *base, sdhc_data_t *data);

/*!
 * @brief Transfer data by polling way.
 *
 * @param dmaMode DMA mode.
 * @param base SDHC peripheral base address.
 * @param data Data to be transferred.
 * @retval kStatus_Fail Transfer data failed.
 * @retval kStatus_InvalidArgument Argument is invalid.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDHC_TransferDataBlocking(sdhc_dma_mode_t dmaMode, SDHC_Type *base, sdhc_data_t *data);

/*!
 * @brief Handle card detect interrupt.
 *
 * @param handle SDHC handle.
 * @param interruptFlags Card detect related interrupt flags.
 */
static void SDHC_TransferHandleCardDetect(sdhc_handle_t *handle, uint32_t interruptFlags);

/*!
 * @brief Handle command interrupt.
 *
 * @param base SDHC peripheral base address.
 * @param handle SDHC handle.
 * @param interruptFlags Command related interrupt flags.
 */
static void SDHC_TransferHandleCommand(SDHC_Type *base, sdhc_handle_t *handle, uint32_t interruptFlags);

/*!
 * @brief Handle data interrupt.
 *
 * @param base SDHC peripheral base address.
 * @param handle SDHC handle.
 * @param interruptFlags Data related interrupt flags.
 */
static void SDHC_TransferHandleData(SDHC_Type *base, sdhc_handle_t *handle, uint32_t interruptFlags);

/*!
 * @brief Handle SDIO card interrupt signal.
 *
 * @param handle SDHC handle.
 */
static void SDHC_TransferHandleSdioInterrupt(sdhc_handle_t *handle);

/*!
 * @brief Handle SDIO block gap event.
 *
 * @param handle SDHC handle.
 */
static void SDHC_TransferHandleSdioBlockGap(sdhc_handle_t *handle);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief SDHC internal handle pointer array */
static sdhc_handle_t *s_sdhcHandle[FSL_FEATURE_SOC_SDHC_COUNT];

/*! @brief SDHC base pointer array */
static SDHC_Type *const s_sdhcBase[] = SDHC_BASE_PTRS;

/*! @brief SDHC IRQ name array */
static const IRQn_Type s_sdhcIRQ[] = SDHC_IRQS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief SDHC clock array name */
static const clock_ip_name_t s_sdhcClock[] = SDHC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* SDHC ISR for transactional APIs. */
static sdhc_isr_t s_sdhcIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t SDHC_GetInstance(SDHC_Type *base)
{
    uint8_t instance = 0;

    while ((instance < FSL_FEATURE_SOC_SDHC_COUNT) && (s_sdhcBase[instance] != base))
    {
        instance++;
    }

    assert(instance < FSL_FEATURE_SOC_SDHC_COUNT);

    return instance;
}

static void SDHC_SetTransferInterrupt(SDHC_Type *base, bool usingInterruptSignal)
{
    uint32_t interruptEnabled; /* The Interrupt status flags to be enabled */
    sdhc_dma_mode_t dmaMode = (sdhc_dma_mode_t)((base->PROCTL & SDHC_PROCTL_DMAS_MASK) >> SDHC_PROCTL_DMAS_SHIFT);
    bool cardDetectDat3 = (bool)(base->PROCTL & SDHC_PROCTL_D3CD_MASK);

    /* Disable all interrupts */
    SDHC_DisableInterruptStatus(base, (uint32_t)kSDHC_AllInterruptFlags);
    SDHC_DisableInterruptSignal(base, (uint32_t)kSDHC_AllInterruptFlags);
    DisableIRQ(s_sdhcIRQ[SDHC_GetInstance(base)]);

    interruptEnabled =
        (kSDHC_CommandIndexErrorFlag | kSDHC_CommandCrcErrorFlag | kSDHC_CommandEndBitErrorFlag |
         kSDHC_CommandTimeoutFlag | kSDHC_CommandCompleteFlag | kSDHC_DataTimeoutFlag | kSDHC_DataCrcErrorFlag |
         kSDHC_DataEndBitErrorFlag | kSDHC_DataCompleteFlag | kSDHC_AutoCommand12ErrorFlag);
    if (cardDetectDat3)
    {
        interruptEnabled |= (kSDHC_CardInsertionFlag | kSDHC_CardRemovalFlag);
    }
    switch (dmaMode)
    {
        case kSDHC_DmaModeAdma1:
        case kSDHC_DmaModeAdma2:
            interruptEnabled |= (kSDHC_DmaErrorFlag | kSDHC_DmaCompleteFlag);
            break;
        case kSDHC_DmaModeNo:
            interruptEnabled |= (kSDHC_BufferReadReadyFlag | kSDHC_BufferWriteReadyFlag);
            break;
        default:
            break;
    }

    SDHC_EnableInterruptStatus(base, interruptEnabled);
    if (usingInterruptSignal)
    {
        SDHC_EnableInterruptSignal(base, interruptEnabled);
    }
}

static void SDHC_StartTransfer(SDHC_Type *base, sdhc_command_t *command, sdhc_data_t *data)
{
    uint32_t flags = 0U;
    sdhc_transfer_config_t sdhcTransferConfig = {0};
    sdhc_dma_mode_t dmaMode;

    /* Define the flag corresponding to each response type. */
    switch (command->responseType)
    {
        case kSDHC_ResponseTypeNone:
            break;
        case kSDHC_ResponseTypeR1: /* Response 1 */
            flags |= (kSDHC_ResponseLength48Flag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        case kSDHC_ResponseTypeR1b: /* Response 1 with busy */
            flags |= (kSDHC_ResponseLength48BusyFlag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        case kSDHC_ResponseTypeR2: /* Response 2 */
            flags |= (kSDHC_ResponseLength136Flag | kSDHC_EnableCrcCheckFlag);
            break;
        case kSDHC_ResponseTypeR3: /* Response 3 */
            flags |= (kSDHC_ResponseLength48Flag);
            break;
        case kSDHC_ResponseTypeR4: /* Response 4 */
            flags |= (kSDHC_ResponseLength48Flag);
            break;
        case kSDHC_ResponseTypeR5: /* Response 5 */
            flags |= (kSDHC_ResponseLength48Flag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        case kSDHC_ResponseTypeR5b: /* Response 5 with busy */
            flags |= (kSDHC_ResponseLength48BusyFlag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        case kSDHC_ResponseTypeR6: /* Response 6 */
            flags |= (kSDHC_ResponseLength48Flag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        case kSDHC_ResponseTypeR7: /* Response 7 */
            flags |= (kSDHC_ResponseLength48Flag | kSDHC_EnableCrcCheckFlag | kSDHC_EnableIndexCheckFlag);
            break;
        default:
            break;
    }
    if (command->type == kSDHC_CommandTypeAbort)
    {
        flags |= kSDHC_CommandTypeAbortFlag;
    }

    if (data)
    {
        flags |= kSDHC_DataPresentFlag;
        dmaMode = (sdhc_dma_mode_t)((base->PROCTL & SDHC_PROCTL_DMAS_MASK) >> SDHC_PROCTL_DMAS_SHIFT);
        if (dmaMode != kSDHC_DmaModeNo)
        {
            flags |= kSDHC_EnableDmaFlag;
        }
        if (data->rxData)
        {
            flags |= kSDHC_DataReadFlag;
        }
        if (data->blockCount > 1U)
        {
            flags |= (kSDHC_MultipleBlockFlag | kSDHC_EnableBlockCountFlag);
            if (data->enableAutoCommand12)
            {
                /* Enable Auto command 12. */
                flags |= kSDHC_EnableAutoCommand12Flag;
            }
        }

        sdhcTransferConfig.dataBlockSize = data->blockSize;
        sdhcTransferConfig.dataBlockCount = data->blockCount;
    }
    else
    {
        sdhcTransferConfig.dataBlockSize = 0U;
        sdhcTransferConfig.dataBlockCount = 0U;
    }

    sdhcTransferConfig.commandArgument = command->argument;
    sdhcTransferConfig.commandIndex = command->index;
    sdhcTransferConfig.flags = flags;
    SDHC_SetTransferConfig(base, &sdhcTransferConfig);
}

static void SDHC_ReceiveCommandResponse(SDHC_Type *base, sdhc_command_t *command)
{
    uint32_t i;

    if (command->responseType != kSDHC_ResponseTypeNone)
    {
        command->response[0U] = SDHC_GetCommandResponse(base, 0U);
        if (command->responseType == kSDHC_ResponseTypeR2)
        {
            command->response[1U] = SDHC_GetCommandResponse(base, 1U);
            command->response[2U] = SDHC_GetCommandResponse(base, 2U);
            command->response[3U] = SDHC_GetCommandResponse(base, 3U);

            i = 4U;
            /* R3-R2-R1-R0(lowest 8 bit is invalid bit) has the same format as R2 format in SD specification document
            after removed internal CRC7 and end bit. */
            do
            {
                command->response[i - 1U] <<= 8U;
                if (i > 1U)
                {
                    command->response[i - 1U] |= ((command->response[i - 2U] & 0xFF000000U) >> 24U);
                }
            } while (i--);
        }
    }
}

static uint32_t SDHC_ReadDataPort(SDHC_Type *base, sdhc_data_t *data, uint32_t transferredWords)
{
    uint32_t i;
    uint32_t totalWords;
    uint32_t wordsCanBeRead; /* The words can be read at this time. */
    uint32_t readWatermark = ((base->WML & SDHC_WML_RDWML_MASK) >> SDHC_WML_RDWML_SHIFT);

    /*
       * Add non aligned access support ,user need make sure your buffer size is big
       * enough to hold the data,in other words,user need make sure the buffer size
       * is 4 byte aligned
       */
    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

    /* If watermark level is equal or bigger than totalWords, transfers totalWords data. */
    if (readWatermark >= totalWords)
    {
        wordsCanBeRead = totalWords;
    }
    /* If watermark level is less than totalWords and left words to be sent is equal or bigger than readWatermark,
    transfers watermark level words. */
    else if ((readWatermark < totalWords) && ((totalWords - transferredWords) >= readWatermark))
    {
        wordsCanBeRead = readWatermark;
    }
    /* If watermark level is less than totalWords and left words to be sent is less than readWatermark, transfers left
    words. */
    else
    {
        wordsCanBeRead = (totalWords - transferredWords);
    }

    i = 0U;
    while (i < wordsCanBeRead)
    {
        data->rxData[transferredWords++] = SDHC_ReadData(base);
        i++;
    }

    return transferredWords;
}

static status_t SDHC_ReadByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data)
{
    uint32_t totalWords;
    uint32_t transferredWords = 0U;
    status_t error = kStatus_Success;

    /*
       * Add non aligned access support ,user need make sure your buffer size is big
       * enough to hold the data,in other words,user need make sure the buffer size
       * is 4 byte aligned
       */
    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

    while ((error == kStatus_Success) && (transferredWords < totalWords))
    {
        while (!(SDHC_GetInterruptStatusFlags(base) & (kSDHC_BufferReadReadyFlag | kSDHC_DataErrorFlag)))
        {
        }

        if (SDHC_GetInterruptStatusFlags(base) & kSDHC_DataErrorFlag)
        {
            if (!(data->enableIgnoreError))
            {
                error = kStatus_Fail;
            }
        }
        if (error == kStatus_Success)
        {
            transferredWords = SDHC_ReadDataPort(base, data, transferredWords);
        }

        /* Clear buffer enable flag to trigger transfer. Clear data error flag when SDHC encounter error */
        SDHC_ClearInterruptStatusFlags(base, (kSDHC_BufferReadReadyFlag | kSDHC_DataErrorFlag));
    }

    /* Clear data complete flag after the last read operation. */
    SDHC_ClearInterruptStatusFlags(base, kSDHC_DataCompleteFlag);

    return error;
}

static uint32_t SDHC_WriteDataPort(SDHC_Type *base, sdhc_data_t *data, uint32_t transferredWords)
{
    uint32_t i;
    uint32_t totalWords;
    uint32_t wordsCanBeWrote; /* Words can be wrote at this time. */
    uint32_t writeWatermark = ((base->WML & SDHC_WML_WRWML_MASK) >> SDHC_WML_WRWML_SHIFT);

    /*
       * Add non aligned access support ,user need make sure your buffer size is big
       * enough to hold the data,in other words,user need make sure the buffer size
       * is 4 byte aligned
       */
    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

    /* If watermark level is equal or bigger than totalWords, transfers totalWords data.*/
    if (writeWatermark >= totalWords)
    {
        wordsCanBeWrote = totalWords;
    }
    /* If watermark level is less than totalWords and left words to be sent is equal or bigger than watermark,
    transfers watermark level words. */
    else if ((writeWatermark < totalWords) && ((totalWords - transferredWords) >= writeWatermark))
    {
        wordsCanBeWrote = writeWatermark;
    }
    /* If watermark level is less than totalWords and left words to be sent is less than watermark, transfers left
    words. */
    else
    {
        wordsCanBeWrote = (totalWords - transferredWords);
    }

    i = 0U;
    while (i < wordsCanBeWrote)
    {
        SDHC_WriteData(base, data->txData[transferredWords++]);
        i++;
    }

    return transferredWords;
}

static status_t SDHC_WriteByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data)
{
    uint32_t totalWords;
    uint32_t transferredWords = 0U;
    status_t error = kStatus_Success;

    /*
       * Add non aligned access support ,user need make sure your buffer size is big
       * enough to hold the data,in other words,user need make sure the buffer size
       * is 4 byte aligned
       */
    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = (data->blockCount * data->blockSize) / sizeof(uint32_t);

    while ((error == kStatus_Success) && (transferredWords < totalWords))
    {
        while (!(SDHC_GetInterruptStatusFlags(base) & (kSDHC_BufferWriteReadyFlag | kSDHC_DataErrorFlag)))
        {
        }

        if (SDHC_GetInterruptStatusFlags(base) & kSDHC_DataErrorFlag)
        {
            if (!(data->enableIgnoreError))
            {
                error = kStatus_Fail;
            }
        }
        if (error == kStatus_Success)
        {
            transferredWords = SDHC_WriteDataPort(base, data, transferredWords);
        }

        /* Clear buffer enable flag to trigger transfer. Clear error flag when SDHC encounter error. */
        SDHC_ClearInterruptStatusFlags(base, (kSDHC_BufferWriteReadyFlag | kSDHC_DataErrorFlag));
    }

    /* Wait write data complete or data transfer error after the last writing operation. */
    while (!(SDHC_GetInterruptStatusFlags(base) & (kSDHC_DataCompleteFlag | kSDHC_DataErrorFlag)))
    {
    }
    if (SDHC_GetInterruptStatusFlags(base) & kSDHC_DataErrorFlag)
    {
        if (!(data->enableIgnoreError))
        {
            error = kStatus_Fail;
        }
    }
    SDHC_ClearInterruptStatusFlags(base, (kSDHC_DataCompleteFlag | kSDHC_DataErrorFlag));

    return error;
}

static status_t SDHC_SendCommandBlocking(SDHC_Type *base, sdhc_command_t *command)
{
    status_t error = kStatus_Success;

    /* Wait command complete or SDHC encounters error. */
    while (!(SDHC_GetInterruptStatusFlags(base) & (kSDHC_CommandCompleteFlag | kSDHC_CommandErrorFlag)))
    {
    }

    if (SDHC_GetInterruptStatusFlags(base) & kSDHC_CommandErrorFlag)
    {
        error = kStatus_Fail;
    }
    /* Receive response when command completes successfully. */
    if (error == kStatus_Success)
    {
        SDHC_ReceiveCommandResponse(base, command);
    }

    SDHC_ClearInterruptStatusFlags(base, (kSDHC_CommandCompleteFlag | kSDHC_CommandErrorFlag));

    return error;
}

static status_t SDHC_TransferByDataPortBlocking(SDHC_Type *base, sdhc_data_t *data)
{
    status_t error = kStatus_Success;

    if (data->rxData)
    {
        error = SDHC_ReadByDataPortBlocking(base, data);
    }
    else
    {
        error = SDHC_WriteByDataPortBlocking(base, data);
    }

    return error;
}

static status_t SDHC_TransferByAdma2Blocking(SDHC_Type *base, sdhc_data_t *data)
{
    status_t error = kStatus_Success;

    /* Wait data complete or SDHC encounters error. */
    while (!(SDHC_GetInterruptStatusFlags(base) & (kSDHC_DataCompleteFlag | kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag)))
    {
    }
    if (SDHC_GetInterruptStatusFlags(base) & (kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag))
    {
        if (!(data->enableIgnoreError))
        {
            error = kStatus_Fail;
        }
    }
    SDHC_ClearInterruptStatusFlags(
        base, (kSDHC_DataCompleteFlag | kSDHC_DmaCompleteFlag | kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag));
    return error;
}

#if defined FSL_SDHC_ENABLE_ADMA1
#define SDHC_TransferByAdma1Blocking(base, data) SDHC_TransferByAdma2Blocking(base, data)
#endif /* FSL_SDHC_ENABLE_ADMA1 */

static status_t SDHC_TransferDataBlocking(sdhc_dma_mode_t dmaMode, SDHC_Type *base, sdhc_data_t *data)
{
    status_t error = kStatus_Success;

    switch (dmaMode)
    {
        case kSDHC_DmaModeNo:
            error = SDHC_TransferByDataPortBlocking(base, data);
            break;
#if defined FSL_SDHC_ENABLE_ADMA1
        case kSDHC_DmaModeAdma1:
            error = SDHC_TransferByAdma1Blocking(base, data);
            break;
#endif /* FSL_SDHC_ENABLE_ADMA1 */
        case kSDHC_DmaModeAdma2:
            error = SDHC_TransferByAdma2Blocking(base, data);
            break;
        default:
            error = kStatus_InvalidArgument;
            break;
    }

    return error;
}

static void SDHC_TransferHandleCardDetect(sdhc_handle_t *handle, uint32_t interruptFlags)
{
    if (interruptFlags & kSDHC_CardInsertionFlag)
    {
        if (handle->callback.CardInserted)
        {
            handle->callback.CardInserted();
        }
    }
    else
    {
        if (handle->callback.CardRemoved)
        {
            handle->callback.CardRemoved();
        }
    }
}

static void SDHC_TransferHandleCommand(SDHC_Type *base, sdhc_handle_t *handle, uint32_t interruptFlags)
{
    assert(handle->command);

    if ((interruptFlags & kSDHC_CommandErrorFlag) && (!(handle->data)) && (handle->callback.TransferComplete))
    {
        handle->callback.TransferComplete(base, handle, kStatus_SDHC_SendCommandFailed, handle->userData);
    }
    else
    {
        /* Receive response */
        SDHC_ReceiveCommandResponse(base, handle->command);
        if ((!(handle->data)) && (handle->callback.TransferComplete))
        {
            handle->callback.TransferComplete(base, handle, kStatus_Success, handle->userData);
        }
    }
}

static void SDHC_TransferHandleData(SDHC_Type *base, sdhc_handle_t *handle, uint32_t interruptFlags)
{
    assert(handle->data);

    if ((!(handle->data->enableIgnoreError)) && (interruptFlags & (kSDHC_DataErrorFlag | kSDHC_DmaErrorFlag)) &&
        (handle->callback.TransferComplete))
    {
        handle->callback.TransferComplete(base, handle, kStatus_SDHC_TransferDataFailed, handle->userData);
    }
    else
    {
        if (interruptFlags & kSDHC_BufferReadReadyFlag)
        {
            handle->transferredWords = SDHC_ReadDataPort(base, handle->data, handle->transferredWords);
        }
        else if (interruptFlags & kSDHC_BufferWriteReadyFlag)
        {
            handle->transferredWords = SDHC_WriteDataPort(base, handle->data, handle->transferredWords);
        }
        else if ((interruptFlags & kSDHC_DataCompleteFlag) && (handle->callback.TransferComplete))
        {
            handle->callback.TransferComplete(base, handle, kStatus_Success, handle->userData);
        }
        else
        {
            /* Do nothing when DMA complete flag is set. Wait until data complete flag is set. */
        }
    }
}

static void SDHC_TransferHandleSdioInterrupt(sdhc_handle_t *handle)
{
    if (handle->callback.SdioInterrupt)
    {
        handle->callback.SdioInterrupt();
    }
}

static void SDHC_TransferHandleSdioBlockGap(sdhc_handle_t *handle)
{
    if (handle->callback.SdioBlockGap)
    {
        handle->callback.SdioBlockGap();
    }
}

void SDHC_Init(SDHC_Type *base, const sdhc_config_t *config)
{
    assert(config);
#if !defined FSL_SDHC_ENABLE_ADMA1
    assert(config->dmaMode != kSDHC_DmaModeAdma1);
#endif /* FSL_SDHC_ENABLE_ADMA1 */
    assert((config->writeWatermarkLevel >= 1U) && (config->writeWatermarkLevel <= 128U));
    assert((config->readWatermarkLevel >= 1U) && (config->readWatermarkLevel <= 128U));

    uint32_t proctl;
    uint32_t wml;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable SDHC clock. */
    CLOCK_EnableClock(s_sdhcClock[SDHC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset SDHC. */
    SDHC_Reset(base, kSDHC_ResetAll, 100);

    proctl = base->PROCTL;
    wml = base->WML;

    proctl &= ~(SDHC_PROCTL_D3CD_MASK | SDHC_PROCTL_EMODE_MASK | SDHC_PROCTL_DMAS_MASK);
    /* Set DAT3 as card detection pin */
    if (config->cardDetectDat3)
    {
        proctl |= SDHC_PROCTL_D3CD_MASK;
    }
    /* Endian mode and DMA mode */
    proctl |= (SDHC_PROCTL_EMODE(config->endianMode) | SDHC_PROCTL_DMAS(config->dmaMode));

    /* Watermark level */
    wml &= ~(SDHC_WML_RDWML_MASK | SDHC_WML_WRWML_MASK);
    wml |= (SDHC_WML_RDWML(config->readWatermarkLevel) | SDHC_WML_WRWML(config->writeWatermarkLevel));

    base->WML = wml;
    base->PROCTL = proctl;

    /* Disable all clock auto gated off feature because of DAT0 line logic(card buffer full status) can't be updated
    correctly when clock auto gated off is enabled. */
    base->SYSCTL |= (SDHC_SYSCTL_PEREN_MASK | SDHC_SYSCTL_HCKEN_MASK | SDHC_SYSCTL_IPGEN_MASK);

    /* Enable interrupt status but doesn't enable interrupt signal. */
    SDHC_SetTransferInterrupt(base, false);
}

void SDHC_Deinit(SDHC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable clock. */
    CLOCK_DisableClock(s_sdhcClock[SDHC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

bool SDHC_Reset(SDHC_Type *base, uint32_t mask, uint32_t timeout)
{
    base->SYSCTL |= (mask & (SDHC_SYSCTL_RSTA_MASK | SDHC_SYSCTL_RSTC_MASK | SDHC_SYSCTL_RSTD_MASK));
    /* Delay some time to wait reset success. */
    while ((base->SYSCTL & mask))
    {
        if (!timeout)
        {
            break;
        }
        timeout--;
    }

    return ((!timeout) ? false : true);
}

void SDHC_GetCapability(SDHC_Type *base, sdhc_capability_t *capability)
{
    assert(capability);

    uint32_t htCapability;
    uint32_t hostVer;
    uint32_t maxBlockLength;

    hostVer = base->HOSTVER;
    htCapability = base->HTCAPBLT;

    /* Get the capability of SDHC. */
    capability->specVersion = ((hostVer & SDHC_HOSTVER_SVN_MASK) >> SDHC_HOSTVER_SVN_SHIFT);
    capability->vendorVersion = ((hostVer & SDHC_HOSTVER_VVN_MASK) >> SDHC_HOSTVER_VVN_SHIFT);
    maxBlockLength = ((htCapability & SDHC_HTCAPBLT_MBL_MASK) >> SDHC_HTCAPBLT_MBL_SHIFT);
    capability->maxBlockLength = (512U << maxBlockLength);
    /* Other attributes not in HTCAPBLT register. */
    capability->maxBlockCount = SDHC_MAX_BLOCK_COUNT;
    capability->flags = (htCapability & (kSDHC_SupportAdmaFlag | kSDHC_SupportHighSpeedFlag | kSDHC_SupportDmaFlag |
                                         kSDHC_SupportSuspendResumeFlag | kSDHC_SupportV330Flag));
#if defined FSL_FEATURE_SDHC_HAS_V300_SUPPORT && FSL_FEATURE_SDHC_HAS_V300_SUPPORT
    capability->flags |= (htCapability & kSDHC_SupportV300Flag);
#endif
#if defined FSL_FEATURE_SDHC_HAS_V180_SUPPORT && FSL_FEATURE_SDHC_HAS_V180_SUPPORT
    capability->flags |= (htCapability & kSDHC_SupportV180Flag);
#endif
    /* eSDHC on all kinetis boards will support 4/8 bit data bus width. */
    capability->flags |= (kSDHC_Support4BitFlag | kSDHC_Support8BitFlag);
}

uint32_t SDHC_SetSdClock(SDHC_Type *base, uint32_t srcClock_Hz, uint32_t busClock_Hz)
{
    assert(srcClock_Hz != 0U);
    assert((busClock_Hz != 0U) && (busClock_Hz <= srcClock_Hz));

    uint32_t divisor;
    uint32_t prescaler;
    uint32_t sysctl;
    uint32_t nearestFrequency = 0;

    divisor = SDHC_INITIAL_DVS;
    prescaler = SDHC_INITIAL_CLKFS;

    /* Disable SD clock. It should be disabled before changing the SD clock frequency.*/
    base->SYSCTL &= ~SDHC_SYSCTL_SDCLKEN_MASK;

    if (busClock_Hz > 0U)
    {
        while ((srcClock_Hz / prescaler / SDHC_MAX_DVS > busClock_Hz) && (prescaler < SDHC_MAX_CLKFS))
        {
            SDHC_NEXT_CLKFS(prescaler);
        }
        while ((srcClock_Hz / prescaler / divisor > busClock_Hz) && (divisor < SDHC_MAX_DVS))
        {
            SDHC_NEXT_DVS(divisor);
        }
        nearestFrequency = srcClock_Hz / prescaler / divisor;
        SDHC_PREV_CLKFS(prescaler);
        SDHC_PREV_DVS(divisor);

        /* Set the SD clock frequency divisor, SD clock frequency select, data timeout counter value. */
        sysctl = base->SYSCTL;
        sysctl &= ~(SDHC_SYSCTL_DVS_MASK | SDHC_SYSCTL_SDCLKFS_MASK | SDHC_SYSCTL_DTOCV_MASK);
        sysctl |= (SDHC_SYSCTL_DVS(divisor) | SDHC_SYSCTL_SDCLKFS(prescaler) | SDHC_SYSCTL_DTOCV(0xEU));
        base->SYSCTL = sysctl;

        /* Wait until the SD clock is stable. */
        while (!(base->PRSSTAT & SDHC_PRSSTAT_SDSTB_MASK))
        {
        }
        /* Enable the SD clock. */
        base->SYSCTL |= SDHC_SYSCTL_SDCLKEN_MASK;
    }

    return nearestFrequency;
}

bool SDHC_SetCardActive(SDHC_Type *base, uint32_t timeout)
{
    base->SYSCTL |= SDHC_SYSCTL_INITA_MASK;
    /* Delay some time to wait card become active state. */
    while ((base->SYSCTL & SDHC_SYSCTL_INITA_MASK))
    {
        if (!timeout)
        {
            break;
        }
        timeout--;
    }

    return ((!timeout) ? false : true);
}

void SDHC_SetTransferConfig(SDHC_Type *base, const sdhc_transfer_config_t *config)
{
    assert(config);
    assert(config->dataBlockSize <= (SDHC_BLKATTR_BLKSIZE_MASK >> SDHC_BLKATTR_BLKSIZE_SHIFT));
    assert(config->dataBlockCount <= (SDHC_BLKATTR_BLKCNT_MASK >> SDHC_BLKATTR_BLKCNT_SHIFT));

    base->BLKATTR = ((base->BLKATTR & ~(SDHC_BLKATTR_BLKSIZE_MASK | SDHC_BLKATTR_BLKCNT_MASK)) |
                     (SDHC_BLKATTR_BLKSIZE(config->dataBlockSize) | SDHC_BLKATTR_BLKCNT(config->dataBlockCount)));
    base->CMDARG = config->commandArgument;
    base->XFERTYP = (((config->commandIndex << SDHC_XFERTYP_CMDINX_SHIFT) & SDHC_XFERTYP_CMDINX_MASK) |
                     (config->flags & (SDHC_XFERTYP_DMAEN_MASK | SDHC_XFERTYP_MSBSEL_MASK | SDHC_XFERTYP_DPSEL_MASK |
                                       SDHC_XFERTYP_CMDTYP_MASK | SDHC_XFERTYP_BCEN_MASK | SDHC_XFERTYP_CICEN_MASK |
                                       SDHC_XFERTYP_CCCEN_MASK | SDHC_XFERTYP_RSPTYP_MASK | SDHC_XFERTYP_DTDSEL_MASK |
                                       SDHC_XFERTYP_AC12EN_MASK)));
}

void SDHC_EnableSdioControl(SDHC_Type *base, uint32_t mask, bool enable)
{
    uint32_t proctl = base->PROCTL;
    uint32_t vendor = base->VENDOR;

    if (enable)
    {
        if (mask & kSDHC_StopAtBlockGapFlag)
        {
            proctl |= SDHC_PROCTL_SABGREQ_MASK;
        }
        if (mask & kSDHC_ReadWaitControlFlag)
        {
            proctl |= SDHC_PROCTL_RWCTL_MASK;
        }
        if (mask & kSDHC_InterruptAtBlockGapFlag)
        {
            proctl |= SDHC_PROCTL_IABG_MASK;
        }
        if (mask & kSDHC_ExactBlockNumberReadFlag)
        {
            vendor |= SDHC_VENDOR_EXBLKNU_MASK;
        }
    }
    else
    {
        if (mask & kSDHC_StopAtBlockGapFlag)
        {
            proctl &= ~SDHC_PROCTL_SABGREQ_MASK;
        }
        if (mask & kSDHC_ReadWaitControlFlag)
        {
            proctl &= ~SDHC_PROCTL_RWCTL_MASK;
        }
        if (mask & kSDHC_InterruptAtBlockGapFlag)
        {
            proctl &= ~SDHC_PROCTL_IABG_MASK;
        }
        if (mask & kSDHC_ExactBlockNumberReadFlag)
        {
            vendor &= ~SDHC_VENDOR_EXBLKNU_MASK;
        }
    }

    base->PROCTL = proctl;
    base->VENDOR = vendor;
}

void SDHC_SetMmcBootConfig(SDHC_Type *base, const sdhc_boot_config_t *config)
{
    assert(config);
    assert(config->ackTimeoutCount <= (SDHC_MMCBOOT_DTOCVACK_MASK >> SDHC_MMCBOOT_DTOCVACK_SHIFT));
    assert(config->blockCount <= (SDHC_MMCBOOT_BOOTBLKCNT_MASK >> SDHC_MMCBOOT_BOOTBLKCNT_SHIFT));

    uint32_t mmcboot = 0U;

    mmcboot = (SDHC_MMCBOOT_DTOCVACK(config->ackTimeoutCount) | SDHC_MMCBOOT_BOOTMODE(config->bootMode) |
               SDHC_MMCBOOT_BOOTBLKCNT(config->blockCount));
    if (config->enableBootAck)
    {
        mmcboot |= SDHC_MMCBOOT_BOOTACK_MASK;
    }
    if (config->enableBoot)
    {
        mmcboot |= SDHC_MMCBOOT_BOOTEN_MASK;
    }
    if (config->enableAutoStopAtBlockGap)
    {
        mmcboot |= SDHC_MMCBOOT_AUTOSABGEN_MASK;
    }
    base->MMCBOOT = mmcboot;
}

status_t SDHC_SetAdmaTableConfig(SDHC_Type *base,
                                 sdhc_dma_mode_t dmaMode,
                                 uint32_t *table,
                                 uint32_t tableWords,
                                 const uint32_t *data,
                                 uint32_t dataBytes)
{
    status_t error = kStatus_Success;
    const uint32_t *startAddress;
    uint32_t entries;
    uint32_t i;
#if defined FSL_SDHC_ENABLE_ADMA1
    sdhc_adma1_descriptor_t *adma1EntryAddress;
#endif
    sdhc_adma2_descriptor_t *adma2EntryAddress;

    if ((((!table) || (!tableWords)) && ((dmaMode == kSDHC_DmaModeAdma1) || (dmaMode == kSDHC_DmaModeAdma2))) ||
        (!data) || (!dataBytes)
#if !defined FSL_SDHC_ENABLE_ADMA1
        || (dmaMode == kSDHC_DmaModeAdma1)
#else
        /* Buffer address configured in ADMA1 descriptor must be 4KB aligned. */
        || ((dmaMode == kSDHC_DmaModeAdma1) && (((uint32_t)data % SDHC_ADMA1_LENGTH_ALIGN) != 0U))
#endif /* FSL_SDHC_ENABLE_ADMA1 */
            )
    {
        error = kStatus_InvalidArgument;
    }
    else
    {
        switch (dmaMode)
        {
            case kSDHC_DmaModeNo:
                break;
#if defined FSL_SDHC_ENABLE_ADMA1
            case kSDHC_DmaModeAdma1:
                /*
                * Add non aligned access support ,user need make sure your buffer size is big
                * enough to hold the data,in other words,user need make sure the buffer size
                * is 4 byte aligned
                */
                if (dataBytes % sizeof(uint32_t) != 0U)
                {
                    dataBytes +=
                        sizeof(uint32_t) - (dataBytes % sizeof(uint32_t)); /* make the data length as word-aligned */
                }

                startAddress = data;
                /* Check if ADMA descriptor's number is enough. */
                entries = ((dataBytes / SDHC_ADMA1_DESCRIPTOR_MAX_LENGTH_PER_ENTRY) + 1U);
                /* ADMA1 needs two descriptors to finish a transfer */
                entries <<= 1U;
                if (entries > ((tableWords * sizeof(uint32_t)) / sizeof(sdhc_adma1_descriptor_t)))
                {
                    error = kStatus_OutOfRange;
                }
                else
                {
                    adma1EntryAddress = (sdhc_adma1_descriptor_t *)(table);
                    for (i = 0U; i < entries; i += 2U)
                    {
                        /* Each descriptor for ADMA1 is 32-bit in length */
                        if ((dataBytes - sizeof(uint32_t) * (startAddress - data)) <=
                            SDHC_ADMA1_DESCRIPTOR_MAX_LENGTH_PER_ENTRY)
                        {
                            /* The last piece of data, setting end flag in descriptor */
                            adma1EntryAddress[i] = ((uint32_t)(dataBytes - sizeof(uint32_t) * (startAddress - data))
                                                    << SDHC_ADMA1_DESCRIPTOR_LENGTH_SHIFT);
                            adma1EntryAddress[i] |= kSDHC_Adma1DescriptorTypeSetLength;
                            adma1EntryAddress[i + 1U] =
                                ((uint32_t)(startAddress) << SDHC_ADMA1_DESCRIPTOR_ADDRESS_SHIFT);
                            adma1EntryAddress[i + 1U] |=
                                (kSDHC_Adma1DescriptorTypeTransfer | kSDHC_Adma1DescriptorEndFlag);
                        }
                        else
                        {
                            adma1EntryAddress[i] = ((uint32_t)SDHC_ADMA1_DESCRIPTOR_MAX_LENGTH_PER_ENTRY
                                                    << SDHC_ADMA1_DESCRIPTOR_LENGTH_SHIFT);
                            adma1EntryAddress[i] |= kSDHC_Adma1DescriptorTypeSetLength;
                            adma1EntryAddress[i + 1U] =
                                ((uint32_t)(startAddress) << SDHC_ADMA1_DESCRIPTOR_ADDRESS_SHIFT);
                            adma1EntryAddress[i + 1U] |= kSDHC_Adma1DescriptorTypeTransfer;
                            startAddress += SDHC_ADMA1_DESCRIPTOR_MAX_LENGTH_PER_ENTRY / sizeof(uint32_t);
                        }
                    }

                    /* When use ADMA, disable simple DMA */
                    base->DSADDR = 0U;
                    base->ADSADDR = (uint32_t)table;
                }
                break;
#endif /* FSL_SDHC_ENABLE_ADMA1 */
            case kSDHC_DmaModeAdma2:
                /*
                * Add non aligned access support ,user need make sure your buffer size is big
                * enough to hold the data,in other words,user need make sure the buffer size
                * is 4 byte aligned
                */
                if (dataBytes % sizeof(uint32_t) != 0U)
                {
                    dataBytes +=
                        sizeof(uint32_t) - (dataBytes % sizeof(uint32_t)); /* make the data length as word-aligned */
                }

                startAddress = data;
                /* Check if ADMA descriptor's number is enough. */
                entries = ((dataBytes / SDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY) + 1U);
                if (entries > ((tableWords * sizeof(uint32_t)) / sizeof(sdhc_adma2_descriptor_t)))
                {
                    error = kStatus_OutOfRange;
                }
                else
                {
                    adma2EntryAddress = (sdhc_adma2_descriptor_t *)(table);
                    for (i = 0U; i < entries; i++)
                    {
                        /* Each descriptor for ADMA2 is 64-bit in length */
                        if ((dataBytes - sizeof(uint32_t) * (startAddress - data)) <=
                            SDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY)
                        {
                            /* The last piece of data, setting end flag in descriptor */
                            adma2EntryAddress[i].address = startAddress;
                            adma2EntryAddress[i].attribute = ((dataBytes - sizeof(uint32_t) * (startAddress - data))
                                                              << SDHC_ADMA2_DESCRIPTOR_LENGTH_SHIFT);
                            adma2EntryAddress[i].attribute |=
                                (kSDHC_Adma2DescriptorTypeTransfer | kSDHC_Adma2DescriptorEndFlag);
                        }
                        else
                        {
                            adma2EntryAddress[i].address = startAddress;
                            adma2EntryAddress[i].attribute =
                                (((SDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY / sizeof(uint32_t)) * sizeof(uint32_t))
                                 << SDHC_ADMA2_DESCRIPTOR_LENGTH_SHIFT);
                            adma2EntryAddress[i].attribute |= kSDHC_Adma2DescriptorTypeTransfer;
                            startAddress += (SDHC_ADMA2_DESCRIPTOR_MAX_LENGTH_PER_ENTRY / sizeof(uint32_t));
                        }
                    }

                    /* When use ADMA, disable simple DMA */
                    base->DSADDR = 0U;
                    base->ADSADDR = (uint32_t)table;
                }
                break;
            default:
                break;
        }
    }

    return error;
}

status_t SDHC_TransferBlocking(SDHC_Type *base, uint32_t *admaTable, uint32_t admaTableWords, sdhc_transfer_t *transfer)
{
    assert(transfer);

    status_t error = kStatus_Success;
    sdhc_dma_mode_t dmaMode = (sdhc_dma_mode_t)((base->PROCTL & SDHC_PROCTL_DMAS_MASK) >> SDHC_PROCTL_DMAS_SHIFT);
    sdhc_command_t *command = transfer->command;
    sdhc_data_t *data = transfer->data;

    /* make sure the cmd/block count is valid */
    if ((!command) || (data && (data->blockCount > SDHC_MAX_BLOCK_COUNT)))
    {
        error = kStatus_InvalidArgument;
    }
    else
    {
        /* Wait until command/data bus out of busy status. */
        while (SDHC_GetPresentStatusFlags(base) & kSDHC_CommandInhibitFlag)
        {
        }
        while (data && (SDHC_GetPresentStatusFlags(base) & kSDHC_DataInhibitFlag))
        {
        }

        /* Update ADMA descriptor table according to different DMA mode(no DMA, ADMA1, ADMA2).*/
        if (data && (kStatus_Success != SDHC_SetAdmaTableConfig(base, dmaMode, admaTable, admaTableWords,
                                                                (data->rxData ? data->rxData : data->txData),
                                                                (data->blockCount * data->blockSize))))
        {
            error = kStatus_SDHC_PrepareAdmaDescriptorFailed;
        }
        else
        {
            /* Send command and receive data. */
            SDHC_StartTransfer(base, command, data);
            if (kStatus_Success != SDHC_SendCommandBlocking(base, command))
            {
                error = kStatus_SDHC_SendCommandFailed;
            }
            else if (data && (kStatus_Success != SDHC_TransferDataBlocking(dmaMode, base, data)))
            {
                error = kStatus_SDHC_TransferDataFailed;
            }
            else
            {
            }
        }
    }

    return error;
}

void SDHC_TransferCreateHandle(SDHC_Type *base,
                               sdhc_handle_t *handle,
                               const sdhc_transfer_callback_t *callback,
                               void *userData)
{
    assert(handle);
    assert(callback);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set the callback. */
    handle->callback.CardInserted = callback->CardInserted;
    handle->callback.CardRemoved = callback->CardRemoved;
    handle->callback.SdioInterrupt = callback->SdioInterrupt;
    handle->callback.SdioBlockGap = callback->SdioBlockGap;
    handle->callback.TransferComplete = callback->TransferComplete;
    handle->userData = userData;

    /* Save the handle in global variables to support the double weak mechanism. */
    s_sdhcHandle[SDHC_GetInstance(base)] = handle;

    /* Enable interrupt in NVIC. */
    SDHC_SetTransferInterrupt(base, true);

    /* save IRQ handler */
    s_sdhcIsr = SDHC_TransferHandleIRQ;

    EnableIRQ(s_sdhcIRQ[SDHC_GetInstance(base)]);
}

status_t SDHC_TransferNonBlocking(
    SDHC_Type *base, sdhc_handle_t *handle, uint32_t *admaTable, uint32_t admaTableWords, sdhc_transfer_t *transfer)
{
    assert(transfer);

    sdhc_dma_mode_t dmaMode = (sdhc_dma_mode_t)((base->PROCTL & SDHC_PROCTL_DMAS_MASK) >> SDHC_PROCTL_DMAS_SHIFT);
    status_t error = kStatus_Success;
    sdhc_command_t *command = transfer->command;
    sdhc_data_t *data = transfer->data;

    /* make sure cmd/block count is valid */
    if ((!command) || (data && (data->blockCount > SDHC_MAX_BLOCK_COUNT)))
    {
        error = kStatus_InvalidArgument;
    }
    else
    {
        /* Wait until command/data bus out of busy status. */
        if ((SDHC_GetPresentStatusFlags(base) & kSDHC_CommandInhibitFlag) ||
            (data && (SDHC_GetPresentStatusFlags(base) & kSDHC_DataInhibitFlag)))
        {
            error = kStatus_SDHC_BusyTransferring;
        }
        else
        {
            /* Update ADMA descriptor table according to different DMA mode(no DMA, ADMA1, ADMA2).*/
            if (data && (kStatus_Success != SDHC_SetAdmaTableConfig(base, dmaMode, admaTable, admaTableWords,
                                                                    (data->rxData ? data->rxData : data->txData),
                                                                    (data->blockCount * data->blockSize))))
            {
                error = kStatus_SDHC_PrepareAdmaDescriptorFailed;
            }
            else
            {
                /* Save command and data into handle before transferring. */
                handle->command = command;
                handle->data = data;
                handle->interruptFlags = 0U;
                /* transferredWords will only be updated in ISR when transfer way is DATAPORT. */
                handle->transferredWords = 0U;

                SDHC_StartTransfer(base, command, data);
            }
        }
    }

    return error;
}

void SDHC_TransferHandleIRQ(SDHC_Type *base, sdhc_handle_t *handle)
{
    assert(handle);

    uint32_t interruptFlags;

    interruptFlags = SDHC_GetInterruptStatusFlags(base);
    handle->interruptFlags = interruptFlags;

    if (interruptFlags & kSDHC_CardDetectFlag)
    {
        SDHC_TransferHandleCardDetect(handle, (interruptFlags & kSDHC_CardDetectFlag));
    }
    if (interruptFlags & kSDHC_CommandFlag)
    {
        SDHC_TransferHandleCommand(base, handle, (interruptFlags & kSDHC_CommandFlag));
    }
    if (interruptFlags & kSDHC_DataFlag)
    {
        SDHC_TransferHandleData(base, handle, (interruptFlags & kSDHC_DataFlag));
    }
    if (interruptFlags & kSDHC_CardInterruptFlag)
    {
        SDHC_TransferHandleSdioInterrupt(handle);
    }
    if (interruptFlags & kSDHC_BlockGapEventFlag)
    {
        SDHC_TransferHandleSdioBlockGap(handle);
    }

    SDHC_ClearInterruptStatusFlags(base, interruptFlags);
}

#if defined(SDHC)
void SDHC_DriverIRQHandler(void)
{
    assert(s_sdhcHandle[0]);

    s_sdhcIsr(SDHC, s_sdhcHandle[0]);
}
#endif
