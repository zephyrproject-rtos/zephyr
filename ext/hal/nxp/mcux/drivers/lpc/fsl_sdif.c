/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_sdif.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.sdif"
#endif

/* Typedef for interrupt handler. */
typedef void (*sdif_isr_t)(SDIF_Type *base, sdif_handle_t *handle);

/*! @brief convert the name here, due to RM use SDIO */
#define SDIF_DriverIRQHandler SDIO_DriverIRQHandler
/*! @brief define the controller support sd/sdio card version 2.0 */
#define SDIF_SUPPORT_SD_VERSION (0x20)
/*! @brief define the controller support mmc card version 4.4 */
#define SDIF_SUPPORT_MMC_VERSION (0x44)
/*! @brief define the timeout counter */
#define SDIF_TIMEOUT_VALUE (~0U)
/*! @brief this value can be any value */
#define SDIF_POLL_DEMAND_VALUE (0xFFU)
/*! @brief DMA descriptor buffer1 size */
#define SDIF_DMA_DESCRIPTOR_BUFFER1_SIZE(x) ((x)&0x1FFFU)
/*! @brief DMA descriptor buffer2 size */
#define SDIF_DMA_DESCRIPTOR_BUFFER2_SIZE(x) (((x)&0x1FFFU) << 13U)
/*! @brief RX water mark value */
#define SDIF_RX_WATERMARK (15U)
/*! @brief TX water mark value */
#define SDIF_TX_WATERMARK (16U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance.
 *
 * @param base SDIF peripheral base address.
 * @return Instance number.
 */
static uint32_t SDIF_GetInstance(SDIF_Type *base);

/*
* @brief config the SDIF interface before transfer between the card and host
* @param SDIF base address
* @param transfer config structure
* @param enDMA DMA enable flag
*/
static status_t SDIF_TransferConfig(SDIF_Type *base, sdif_transfer_t *transfer, bool enDMA);

/*
* @brief wait the command done function and check error status
* @param SDIF base address
* @param command config structure
*/
static status_t SDIF_WaitCommandDone(SDIF_Type *base, sdif_command_t *command);

/*
* @brief transfer data in a blocking way
* @param SDIF base address
* @param data config structure
* @param indicate current transfer mode:DMA or polling
*/
static status_t SDIF_TransferDataBlocking(SDIF_Type *base, sdif_data_t *data, bool isDMA);

/*
* @brief read the command response
* @param SDIF base address
* @param sdif command pointer
*/
static status_t SDIF_ReadCommandResponse(SDIF_Type *base, sdif_command_t *command);

/*
* @brief handle transfer command interrupt
* @param SDIF base address
* @param sdif handle
* @param interrupt mask flags
*/
static void SDIF_TransferHandleCommand(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags);

/*
* @brief handle transfer data interrupt
* @param SDIF base address
* @param sdif handle
* @param interrupt mask flags
*/
static void SDIF_TransferHandleData(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags);

/*
* @brief handle DMA transfer
* @param SDIF base address
* @param sdif handle
* @param interrupt mask flag
*/
static void SDIF_TransferHandleDMA(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags);

/*
* @brief driver IRQ handler
* @param SDIF base address
* @param sdif handle
*/
static void SDIF_TransferHandleIRQ(SDIF_Type *base, sdif_handle_t *handle);

/*
* @brief read data port
* @param SDIF base address
* @param sdif data
* @param the number of data been transferred
*/
static uint32_t SDIF_ReadDataPort(SDIF_Type *base, sdif_data_t *data, uint32_t transferredWords);

/*
* @brief write data port
* @param SDIF base address
* @param sdif data
* @param the number of data been transferred
*/
static uint32_t SDIF_WriteDataPort(SDIF_Type *base, sdif_data_t *data, uint32_t transferredWords);

/*
* @brief read data by blocking way
* @param SDIF base address
* @param sdif data
*/
static status_t SDIF_ReadDataPortBlocking(SDIF_Type *base, sdif_data_t *data);

/*
* @brief write data by blocking way
* @param SDIF base address
* @param sdif data
*/
static status_t SDIF_WriteDataPortBlocking(SDIF_Type *base, sdif_data_t *data);

/*
* @brief handle sdio interrupt
* This function will call the SDIO interrupt callback
* @param SDIF base address
* @param SDIF handle
*/
static void SDIF_TransferHandleSDIOInterrupt(SDIF_Type *base, sdif_handle_t *handle);

/*
* @brief handle card detect
* This function will call the cardInserted callback
* @param SDIF base addres
* @param SDIF handle
*/
static void SDIF_TransferHandleCardDetect(SDIF_Type *base, sdif_handle_t *handle);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief SDIF internal handle pointer array */
static sdif_handle_t *s_sdifHandle[FSL_FEATURE_SOC_SDIF_COUNT];

/*! @brief SDIF base pointer array */
static SDIF_Type *const s_sdifBase[] = SDIF_BASE_PTRS;

/*! @brief SDIF IRQ name array */
static const IRQn_Type s_sdifIRQ[] = SDIF_IRQS;

/* SDIF ISR for transactional APIs. */
static sdif_isr_t s_sdifIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t SDIF_GetInstance(SDIF_Type *base)
{
    uint8_t instance = 0U;

    while ((instance < ARRAY_SIZE(s_sdifBase)) && (s_sdifBase[instance] != base))
    {
        instance++;
    }

    assert(instance < ARRAY_SIZE(s_sdifBase));

    return instance;
}

static status_t SDIF_TransferConfig(SDIF_Type *base, sdif_transfer_t *transfer, bool enDMA)
{
    sdif_command_t *command = transfer->command;
    sdif_data_t *data = transfer->data;

    if ((command == NULL) || (data && (data->blockSize > SDIF_BLKSIZ_BLOCK_SIZE_MASK)))
    {
        return kStatus_SDIF_InvalidArgument;
    }

    if (data != NULL)
    {
        /* config the block size register ,the block size maybe smaller than FIFO
         depth, will test on the board */
        base->BLKSIZ = SDIF_BLKSIZ_BLOCK_SIZE(data->blockSize);
        /* config the byte count register */
        base->BYTCNT = SDIF_BYTCNT_BYTE_COUNT(data->blockSize * data->blockCount);

        command->flags |= kSDIF_DataExpect; /* need transfer data flag */

        if (data->txData != NULL)
        {
            command->flags |= kSDIF_DataWriteToCard; /* data transfer direction */
        }
        else
        {
            /* config the card read threshold,enable the card read threshold */
            if (data->blockSize <= (SDIF_FIFO_COUNT * sizeof(uint32_t)))
            {
                base->CARDTHRCTL = SDIF_CARDTHRCTL_CARDRDTHREN_MASK | SDIF_CARDTHRCTL_CARDTHRESHOLD(data->blockSize);
            }
            else
            {
                base->CARDTHRCTL &= ~SDIF_CARDTHRCTL_CARDRDTHREN_MASK;
            }
        }

        if (data->streamTransfer)
        {
            command->flags |= kSDIF_DataStreamTransfer; /* indicate if use stream transfer or block transfer  */
        }

        if ((data->enableAutoCommand12) &&
            (data->blockCount > 1U)) /* indicate if auto stop will send after the data transfer done */
        {
            command->flags |= kSDIF_DataTransferAutoStop;
        }

        if (enDMA)
        {
            base->INTMASK &= ~(kSDIF_DataTransferOver | kSDIF_WriteFIFORequest | kSDIF_ReadFIFORequest);
        }
        else
        {
            base->INTMASK |= (kSDIF_DataTransferOver | kSDIF_WriteFIFORequest | kSDIF_ReadFIFORequest);
        }
    }
    /* R2 response length long */
    if (command->responseType == kCARD_ResponseTypeR2)
    {
        command->flags |= (kSDIF_CmdCheckResponseCRC | kSDIF_CmdResponseLengthLong | kSDIF_CmdResponseExpect);
    }
    else if ((command->responseType == kCARD_ResponseTypeR3) || (command->responseType == kCARD_ResponseTypeR4))
    {
        command->flags |= kSDIF_CmdResponseExpect; /* response R3 do not check Response CRC */
    }
    else
    {
        if (command->responseType != kCARD_ResponseTypeNone)
        {
            command->flags |= (kSDIF_CmdCheckResponseCRC | kSDIF_CmdResponseExpect);
        }
    }

    if (command->type == kCARD_CommandTypeAbort)
    {
        command->flags |= kSDIF_TransferStopAbort;
    }

    /* wait pre-transfer complete */
    command->flags |= kSDIF_WaitPreTransferComplete | kSDIF_CmdDataUseHoldReg;

    return kStatus_Success;
}

static status_t SDIF_ReadCommandResponse(SDIF_Type *base, sdif_command_t *command)
{
    /* check if command exist,if not, do not read the response */
    if (NULL != command)
    {
        /* read response */
        command->response[0U] = base->RESP[0U];
        if (command->responseType == kCARD_ResponseTypeR2)
        {
            command->response[1U] = base->RESP[1U];
            command->response[2U] = base->RESP[2U];
            command->response[3U] = base->RESP[3U];
        }

        if ((command->responseErrorFlags != 0U) &&
            ((command->responseType == kCARD_ResponseTypeR1) || (command->responseType == kCARD_ResponseTypeR1b) ||
             (command->responseType == kCARD_ResponseTypeR6) || (command->responseType == kCARD_ResponseTypeR5)))
        {
            if (((command->responseErrorFlags) & (command->response[0U])) != 0U)
            {
                return kStatus_SDIF_ResponseError;
            }
        }
    }

    return kStatus_Success;
}

static status_t SDIF_WaitCommandDone(SDIF_Type *base, sdif_command_t *command)
{
    uint32_t status = 0U;

    do
    {
        status = SDIF_GetInterruptStatus(base);
    } while ((status & kSDIF_CommandDone) != kSDIF_CommandDone);
    /* clear interrupt status flag first */
    SDIF_ClearInterruptStatus(base, status & kSDIF_CommandTransferStatus);
    if ((status & (kSDIF_ResponseError | kSDIF_ResponseCRCError | kSDIF_ResponseTimeout | kSDIF_HardwareLockError)) !=
        0u)
    {
        return kStatus_SDIF_SendCmdFail;
    }
    else
    {
        return SDIF_ReadCommandResponse(base, command);
    }
}

/*!
 * brief SDIF release the DMA descriptor to DMA engine
 * this function should be called when DMA descriptor unavailable status occurs
 * param base SDIF peripheral base address.
 * param sdif DMA config pointer
 */
status_t SDIF_ReleaseDMADescriptor(SDIF_Type *base, sdif_dma_config_t *dmaConfig)
{
    assert(NULL != dmaConfig);
    assert(NULL != dmaConfig->dmaDesBufferStartAddr);

    sdif_dma_descriptor_t *dmaDesAddr;
    uint32_t *tempDMADesBuffer = dmaConfig->dmaDesBufferStartAddr;
    uint32_t dmaDesBufferSize = 0U;

    dmaDesAddr = (sdif_dma_descriptor_t *)tempDMADesBuffer;

    /* chain descriptor mode */
    if (dmaConfig->mode == kSDIF_ChainDMAMode)
    {
        while (((dmaDesAddr->dmaDesAttribute & (1U << kSDIF_DMADescriptorDataBufferEnd)) !=
                (1U << kSDIF_DMADescriptorDataBufferEnd)) &&
               (dmaDesBufferSize < dmaConfig->dmaDesBufferLen * sizeof(uint32_t)))
        {
            /* set the OWN bit */
            dmaDesAddr->dmaDesAttribute |= 1U << kSDIF_DMADescriptorOwnByDMA;
            dmaDesAddr++;
            dmaDesBufferSize += sizeof(sdif_dma_descriptor_t);
        }
        /* if access dma des address overflow, return fail */
        if (dmaDesBufferSize > dmaConfig->dmaDesBufferLen * sizeof(uint32_t))
        {
            return kStatus_Fail;
        }
        dmaDesAddr->dmaDesAttribute |= 1U << kSDIF_DMADescriptorOwnByDMA;
    }
    /* dual descriptor mode */
    else
    {
        while (((dmaDesAddr->dmaDesAttribute & (1U << kSDIF_DMADescriptorEnd)) != (1U << kSDIF_DMADescriptorEnd)) &&
               (dmaDesBufferSize < dmaConfig->dmaDesBufferLen * sizeof(uint32_t)))
        {
            dmaDesAddr = (sdif_dma_descriptor_t *)tempDMADesBuffer;
            dmaDesAddr->dmaDesAttribute |= 1U << kSDIF_DMADescriptorOwnByDMA;
            tempDMADesBuffer += dmaConfig->dmaDesSkipLen;
        }
        /* if access dma des address overflow, return fail */
        if (dmaDesBufferSize > dmaConfig->dmaDesBufferLen * sizeof(uint32_t))
        {
            return kStatus_Fail;
        }
        dmaDesAddr->dmaDesAttribute |= 1U << kSDIF_DMADescriptorOwnByDMA;
    }
    /* reload DMA descriptor */
    base->PLDMND = SDIF_POLL_DEMAND_VALUE;

    return kStatus_Success;
}

static uint32_t SDIF_ReadDataPort(SDIF_Type *base, sdif_data_t *data, uint32_t transferredWords)
{
    uint32_t i;
    uint32_t totalWords;
    uint32_t wordsCanBeRead; /* The words can be read at this time. */
    uint32_t readWatermark = ((base->FIFOTH & SDIF_FIFOTH_RX_WMARK_MASK) >> SDIF_FIFOTH_RX_WMARK_SHIFT);

    if ((base->CTRL & SDIF_CTRL_USE_INTERNAL_DMAC_MASK) == 0U)
    {
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
        /* If watermark level is less than totalWords and left words to be sent is less than readWatermark, transfers
        left
        words. */
        else
        {
            wordsCanBeRead = (totalWords - transferredWords);
        }

        i = 0U;
        while (i < wordsCanBeRead)
        {
            data->rxData[transferredWords++] = base->FIFO[i];
            i++;
        }
    }

    return transferredWords;
}

static uint32_t SDIF_WriteDataPort(SDIF_Type *base, sdif_data_t *data, uint32_t transferredWords)
{
    uint32_t i;
    uint32_t totalWords;
    uint32_t wordsCanBeWrite; /* The words can be read at this time. */
    uint32_t writeWatermark = ((base->FIFOTH & SDIF_FIFOTH_TX_WMARK_MASK) >> SDIF_FIFOTH_TX_WMARK_SHIFT);

    if ((base->CTRL & SDIF_CTRL_USE_INTERNAL_DMAC_MASK) == 0U)
    {
        if (data->blockSize % sizeof(uint32_t) != 0U)
        {
            data->blockSize +=
                sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
        }

        totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

        /* If watermark level is equal or bigger than totalWords, transfers totalWords data. */
        if (writeWatermark >= totalWords)
        {
            wordsCanBeWrite = totalWords;
        }
        /* If watermark level is less than totalWords and left words to be sent is equal or bigger than writeWatermark,
        transfers watermark level words. */
        else if ((writeWatermark < totalWords) && ((totalWords - transferredWords) >= writeWatermark))
        {
            wordsCanBeWrite = writeWatermark;
        }
        /* If watermark level is less than totalWords and left words to be sent is less than writeWatermark, transfers
        left
        words. */
        else
        {
            wordsCanBeWrite = (totalWords - transferredWords);
        }

        i = 0U;
        while (i < wordsCanBeWrite)
        {
            base->FIFO[i] = data->txData[transferredWords++];
            i++;
        }
    }

    return transferredWords;
}

static status_t SDIF_ReadDataPortBlocking(SDIF_Type *base, sdif_data_t *data)
{
    uint32_t totalWords;
    uint32_t transferredWords = 0U;
    status_t error = kStatus_Success;
    uint32_t status;
    bool transferOver = false;

    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

    while ((transferredWords < totalWords) && (error == kStatus_Success))
    {
        /* wait data transfer complete or reach RX watermark */
        do
        {
            status = SDIF_GetInterruptStatus(base);
            if (status & kSDIF_DataTransferError)
            {
                if (!(data->enableIgnoreError))
                {
                    error = kStatus_Fail;
                    break;
                }
            }
        } while (((status & (kSDIF_DataTransferOver | kSDIF_ReadFIFORequest)) == 0U) && (!transferOver));

        if ((status & kSDIF_DataTransferOver) == kSDIF_DataTransferOver)
        {
            transferOver = true;
        }

        if (error == kStatus_Success)
        {
            transferredWords = SDIF_ReadDataPort(base, data, transferredWords);
        }

        /* clear interrupt status */
        SDIF_ClearInterruptStatus(base, status);
    }

    return error;
}

static status_t SDIF_WriteDataPortBlocking(SDIF_Type *base, sdif_data_t *data)
{
    uint32_t totalWords;
    uint32_t transferredWords = 0U;
    status_t error = kStatus_Success;
    uint32_t status;

    if (data->blockSize % sizeof(uint32_t) != 0U)
    {
        data->blockSize +=
            sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t)); /* make the block size as word-aligned */
    }

    totalWords = ((data->blockCount * data->blockSize) / sizeof(uint32_t));

    while ((transferredWords < totalWords) && (error == kStatus_Success))
    {
        /* wait data transfer complete or reach RX watermark */
        do
        {
            status = SDIF_GetInterruptStatus(base);
            if (status & kSDIF_DataTransferError)
            {
                if (!(data->enableIgnoreError))
                {
                    error = kStatus_Fail;
                }
            }
        } while ((status & kSDIF_WriteFIFORequest) == 0U);

        if (error == kStatus_Success)
        {
            transferredWords = SDIF_WriteDataPort(base, data, transferredWords);
        }

        /* clear interrupt status */
        SDIF_ClearInterruptStatus(base, status);
    }

    while ((SDIF_GetInterruptStatus(base) & kSDIF_DataTransferOver) != kSDIF_DataTransferOver)
    {
    }

    if (SDIF_GetInterruptStatus(base) & kSDIF_DataTransferError)
    {
        if (!(data->enableIgnoreError))
        {
            error = kStatus_Fail;
        }
    }
    SDIF_ClearInterruptStatus(base, (kSDIF_DataTransferOver | kSDIF_DataTransferError));

    return error;
}

/*!
 * brief reset the different block of the interface.
 * param base SDIF peripheral base address.
 * param mask indicate which block to reset.
 * param timeout value,set to wait the bit self clear
 * return reset result.
 */
bool SDIF_Reset(SDIF_Type *base, uint32_t mask, uint32_t timeout)
{
    /* reset through CTRL */
    base->CTRL |= mask;
    /* DMA software reset */
    if (mask & kSDIF_ResetDMAInterface)
    {
        /* disable DMA first then do DMA software reset */
        base->BMOD = (base->BMOD & (~SDIF_BMOD_DE_MASK)) | SDIF_BMOD_SWR_MASK;
    }

    /* check software DMA reset here for DMA reset also need to check this bit */
    while ((base->CTRL & mask) != 0U)
    {
        if (!timeout)
        {
            break;
        }
        timeout--;
    }

    return timeout ? true : false;
}

static status_t SDIF_TransferDataBlocking(SDIF_Type *base, sdif_data_t *data, bool isDMA)
{
    assert(NULL != data);

    uint32_t dmaStatus = 0U;
    status_t error = kStatus_Success;

    /* in DMA mode, only need to wait the complete flag and check error */
    if (isDMA)
    {
        do
        {
            dmaStatus = SDIF_GetInternalDMAStatus(base);
            if ((dmaStatus & kSDIF_DMAFatalBusError) == kSDIF_DMAFatalBusError)
            {
                SDIF_ClearInternalDMAStatus(base, kSDIF_DMAFatalBusError | kSDIF_AbnormalInterruptSummary);
                error = kStatus_SDIF_DMATransferFailWithFBE; /* in this condition,need reset */
            }
            /* Card error summary, include EBE,SBE,Data CRC,RTO,DRTO,Response error */
            if ((dmaStatus & kSDIF_DMACardErrorSummary) == kSDIF_DMACardErrorSummary)
            {
                SDIF_ClearInternalDMAStatus(base, kSDIF_DMACardErrorSummary | kSDIF_AbnormalInterruptSummary);
                if (!(data->enableIgnoreError))
                {
                    error = kStatus_SDIF_DataTransferFail;
                }

                /* if error occur, then return */
                break;
            }
        } while ((dmaStatus & (kSDIF_DMATransFinishOneDescriptor | kSDIF_DMARecvFinishOneDescriptor)) == 0U);

        /* clear the corresponding status bit */
        SDIF_ClearInternalDMAStatus(base, (kSDIF_DMATransFinishOneDescriptor | kSDIF_DMARecvFinishOneDescriptor |
                                           kSDIF_NormalInterruptSummary));

        SDIF_ClearInterruptStatus(base, SDIF_GetInterruptStatus(base));
    }
    else
    {
        if (data->rxData != NULL)
        {
            error = SDIF_ReadDataPortBlocking(base, data);
        }
        else
        {
            error = SDIF_WriteDataPortBlocking(base, data);
        }
    }

    return error;
}

/*!
 * brief send command to the card
 * param base SDIF peripheral base address.
 * param command configuration collection
 * param timeout value
 * return command excute status
 */
status_t SDIF_SendCommand(SDIF_Type *base, sdif_command_t *cmd, uint32_t timeout)
{
    assert(NULL != cmd);

    base->CMDARG = cmd->argument;
    base->CMD = SDIF_CMD_CMD_INDEX(cmd->index) | SDIF_CMD_START_CMD_MASK | (cmd->flags & (~SDIF_CMD_CMD_INDEX_MASK));

    /* wait start_cmd bit auto clear within timeout */
    while ((base->CMD & SDIF_CMD_START_CMD_MASK) == SDIF_CMD_START_CMD_MASK)
    {
        if (!timeout)
        {
            break;
        }

        --timeout;
    }

    return timeout ? kStatus_Success : kStatus_Fail;
}

/*!
 * brief SDIF send initialize 80 clocks for SD card after initial
 * param base SDIF peripheral base address.
 * param timeout value
 */
bool SDIF_SendCardActive(SDIF_Type *base, uint32_t timeout)
{
    bool enINT = false;
    sdif_command_t command = {.index = 0U, .argument = 0U};

    /* add for conflict with interrupt mode,close the interrupt temporary */
    if ((base->CTRL & SDIF_CTRL_INT_ENABLE_MASK) == SDIF_CTRL_INT_ENABLE_MASK)
    {
        enINT = true;
        base->CTRL &= ~SDIF_CTRL_INT_ENABLE_MASK;
    }

    command.flags = SDIF_CMD_SEND_INITIALIZATION_MASK;

    if (SDIF_SendCommand(base, &command, timeout) == kStatus_Fail)
    {
        return false;
    }

    /* wait command done */
    while ((SDIF_GetInterruptStatus(base) & kSDIF_CommandDone) != kSDIF_CommandDone)
    {
    }

    /* clear status */
    SDIF_ClearInterruptStatus(base, kSDIF_CommandDone);

    /* add for conflict with interrupt mode */
    if (enINT)
    {
        base->CTRL |= SDIF_CTRL_INT_ENABLE_MASK;
    }

    return true;
}

/*!
 * brief SDIF config the clock delay
 * This function is used to config the cclk_in delay to
 * sample and driver the data ,should meet the min setup
 * time and hold time, and user need to config this parameter
 * according to your board setting
 * param target freq work mode
 * param clock divider which is used to decide if use phase shift for delay
 */
void SDIF_ConfigClockDelay(uint32_t target_HZ, uint32_t divider)
{
    uint32_t sdioClkCtrl = SYSCON->SDIOCLKCTRL;

    if (target_HZ >= SDIF_CLOCK_RANGE_NEED_DELAY)
    {
        if (divider == 1U)
        {
#if defined(SDIF_HIGHSPEED_SAMPLE_PHASE_SHIFT) && (SDIF_HIGHSPEED_SAMPLE_PHASE_SHIFT != 0U)
            sdioClkCtrl |= SYSCON_SDIOCLKCTRL_PHASE_ACTIVE_MASK |
                           SYSCON_SDIOCLKCTRL_CCLK_SAMPLE_PHASE(SDIF_HIGHSPEED_SAMPLE_PHASE_SHIFT);
#endif
#if defined(SDIF_HIGHSPEED_DRV_PHASE_SHIFT) && (SDIF_HIGHSPEED_DRV_PHASE_SHIFT != 0U)
            sdioClkCtrl |= SYSCON_SDIOCLKCTRL_PHASE_ACTIVE_MASK |
                           SYSCON_SDIOCLKCTRL_CCLK_DRV_PHASE(SDIF_HIGHSPEED_DRV_PHASE_SHIFT);
#endif
        }
        else
        {
#ifdef SDIF_HIGHSPEED_SAMPLE_DELAY
            sdioClkCtrl |= SYSCON_SDIOCLKCTRL_CCLK_SAMPLE_DELAY_ACTIVE_MASK |
                           SYSCON_SDIOCLKCTRL_CCLK_SAMPLE_DELAY(SDIF_HIGHSPEED_SAMPLE_DELAY);
#endif
#ifdef SDIF_HIGHSPEED_DRV_DELAY
            sdioClkCtrl |= SYSCON_SDIOCLKCTRL_CCLK_DRV_DELAY_ACTIVE_MASK |
                           SYSCON_SDIOCLKCTRL_CCLK_DRV_DELAY(SDIF_HIGHSPEED_DRV_DELAY);
#endif
        }
    }

    SYSCON->SDIOCLKCTRL = sdioClkCtrl;
}

/*!
 * brief Sets the card bus clock frequency.
 *
 * param base SDIF peripheral base address.
 * param srcClock_Hz SDIF source clock frequency united in Hz.
 * param target_HZ card bus clock frequency united in Hz.
 * return The nearest frequency of busClock_Hz configured to SD bus.
 */
uint32_t SDIF_SetCardClock(SDIF_Type *base, uint32_t srcClock_Hz, uint32_t target_HZ)
{
    sdif_command_t cmd = {.index = 0U, .argument = 0U};
    uint32_t divider = 0U, targetFreq = target_HZ;

    /* if target freq bigger than the source clk, set the target_HZ to
     src clk, this interface can run up to 52MHZ with card */
    if (srcClock_Hz < targetFreq)
    {
        targetFreq = srcClock_Hz;
    }

    /* disable the clock first,need sync to CIU*/
    SDIF_EnableCardClock(base, false);
    /* update the clock register and wait the pre-transfer complete */
    cmd.flags = kSDIF_CmdUpdateClockRegisterOnly | kSDIF_WaitPreTransferComplete;
    SDIF_SendCommand(base, &cmd, SDIF_TIMEOUT_VALUE);

    /*calculate the divider*/
    if (targetFreq != srcClock_Hz)
    {
        divider = (srcClock_Hz / targetFreq + 1U) / 2U;
    }
    /* load the clock divider */
    base->CLKDIV = SDIF_CLKDIV_CLK_DIVIDER0(divider);
    /* update the divider to CIU */
    SDIF_SendCommand(base, &cmd, SDIF_TIMEOUT_VALUE);

    /* enable the card clock and sync to CIU */
    SDIF_EnableCardClock(base, true);
    SDIF_SendCommand(base, &cmd, SDIF_TIMEOUT_VALUE);

    /* config the clock delay to meet the hold time and setup time */
    SDIF_ConfigClockDelay(target_HZ, divider);

    /* return the actual card clock freq */

    return (divider != 0U) ? (srcClock_Hz / (divider * 2U)) : srcClock_Hz;
}

/*!
 * brief SDIF abort the read data when SDIF card is in suspend state
 * Once assert this bit,data state machine will be reset which is waiting for the
 * next blocking data,used in SDIO card suspend sequence,should call after suspend
 * cmd send
 * param base SDIF peripheral base address.
 * param timeout value to wait this bit self clear which indicate the data machine
 * reset to idle
 */
bool SDIF_AbortReadData(SDIF_Type *base, uint32_t timeout)
{
    /* assert this bit to reset the data machine to abort the read data */
    base->CTRL |= SDIF_CTRL_ABORT_READ_DATA_MASK;
    /* polling the bit self clear */
    while ((base->CTRL & SDIF_CTRL_ABORT_READ_DATA_MASK) == SDIF_CTRL_ABORT_READ_DATA_MASK)
    {
        if (!timeout)
        {
            break;
        }
        timeout--;
    }

    return base->CTRL & SDIF_CTRL_ABORT_READ_DATA_MASK ? false : true;
}

/*!
 * brief SDIF internal DMA config function
 * param base SDIF peripheral base address.
 * param internal DMA configuration collection
 * param data buffer pointer
 * param data buffer size
 */
status_t SDIF_InternalDMAConfig(SDIF_Type *base, sdif_dma_config_t *config, const uint32_t *data, uint32_t dataSize)
{
    assert(NULL != config);
    assert(NULL != data);

    uint32_t dmaEntry = 0U, i, dmaBufferSize = 0U, dmaBuffer1Size = 0U;
    uint32_t *tempDMADesBuffer = config->dmaDesBufferStartAddr;
    const uint32_t *dataBuffer = data;
    sdif_dma_descriptor_t *descriptorPoniter = NULL;
    uint32_t maxDMABuffer = FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE * (config->mode);

    if ((((uint32_t)data % SDIF_INTERNAL_DMA_ADDR_ALIGN) != 0U) ||
        (((uint32_t)tempDMADesBuffer % SDIF_INTERNAL_DMA_ADDR_ALIGN) != 0U))
    {
        return kStatus_SDIF_DMAAddrNotAlign;
    }

    /* check the read/write data size,must be a multiple of 4 */
    if (dataSize % sizeof(uint32_t) != 0U)
    {
        dataSize += sizeof(uint32_t) - (dataSize % sizeof(uint32_t));
    }

    /*config the bus mode*/
    if (config->enableFixBurstLen)
    {
        base->BMOD |= SDIF_BMOD_FB_MASK;
    }

    /* calculate the dma descriptor entry due to DMA buffer size limit */
    /* if data size smaller than one descriptor buffer size */
    if (dataSize > maxDMABuffer)
    {
        dmaEntry = dataSize / maxDMABuffer + (dataSize % maxDMABuffer ? 1U : 0U);
    }
    else /* need one dma descriptor */
    {
        dmaEntry = 1U;
    }

    /* check the DMA descriptor buffer len one more time,it is user's responsibility to make sure the DMA descriptor
    table
    size is bigger enough to hold the transfer descriptor */
    if (config->dmaDesBufferLen * sizeof(uint32_t) < (dmaEntry * sizeof(sdif_dma_descriptor_t) + config->dmaDesSkipLen))
    {
        return kStatus_SDIF_DescriptorBufferLenError;
    }

    switch (config->mode)
    {
        case kSDIF_DualDMAMode:
            base->BMOD |= SDIF_BMOD_DSL(config->dmaDesSkipLen); /* config the distance between the DMA descriptor */
            for (i = 0U; i < dmaEntry; i++)
            {
                if (dataSize > FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE)
                {
                    dmaBufferSize = FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE;
                    dataSize -= dmaBufferSize;
                    dmaBuffer1Size = dataSize > FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE ?
                                         FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE :
                                         dataSize;
                    dataSize -= dmaBuffer1Size;
                }
                else
                {
                    dmaBufferSize = dataSize;
                    dmaBuffer1Size = 0U;
                }

                descriptorPoniter = (sdif_dma_descriptor_t *)tempDMADesBuffer;
                if (i == 0U)
                {
                    descriptorPoniter->dmaDesAttribute = 1U << kSDIF_DMADescriptorDataBufferStart;
                }
                descriptorPoniter->dmaDesAttribute |=
                    (1U << kSDIF_DMADescriptorOwnByDMA) | (1U << kSDIF_DisableCompleteInterrupt);
                descriptorPoniter->dmaDataBufferSize =
                    SDIF_DMA_DESCRIPTOR_BUFFER1_SIZE(dmaBufferSize) | SDIF_DMA_DESCRIPTOR_BUFFER2_SIZE(dmaBuffer1Size);

                descriptorPoniter->dmaDataBufferAddr0 = dataBuffer;
                descriptorPoniter->dmaDataBufferAddr1 = dataBuffer + dmaBufferSize / sizeof(uint32_t);
                dataBuffer += (dmaBufferSize + dmaBuffer1Size) / sizeof(uint32_t);

                /* descriptor skip length */
                tempDMADesBuffer += config->dmaDesSkipLen + sizeof(sdif_dma_descriptor_t) / sizeof(uint32_t);
            }
            /* enable the completion interrupt when reach the last descriptor */
            descriptorPoniter->dmaDesAttribute &= ~(1U << kSDIF_DisableCompleteInterrupt);
            descriptorPoniter->dmaDesAttribute |=
                (1U << kSDIF_DMADescriptorDataBufferEnd) | (1U << kSDIF_DMADescriptorEnd);
            break;

        case kSDIF_ChainDMAMode:
            for (i = 0U; i < dmaEntry; i++)
            {
                if (dataSize > FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE)
                {
                    dmaBufferSize = FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE;
                    dataSize -= FSL_FEATURE_SDIF_INTERNAL_DMA_MAX_BUFFER_SIZE;
                }
                else
                {
                    dmaBufferSize = dataSize;
                }

                descriptorPoniter = (sdif_dma_descriptor_t *)tempDMADesBuffer;
                if (i == 0U)
                {
                    descriptorPoniter->dmaDesAttribute = 1U << kSDIF_DMADescriptorDataBufferStart;
                }
                descriptorPoniter->dmaDesAttribute |= (1U << kSDIF_DMADescriptorOwnByDMA) |
                                                      (1U << kSDIF_DMASecondAddrChained) |
                                                      (1U << kSDIF_DisableCompleteInterrupt);
                descriptorPoniter->dmaDataBufferSize =
                    SDIF_DMA_DESCRIPTOR_BUFFER1_SIZE(dmaBufferSize); /* use only buffer 1 for data buffer*/
                descriptorPoniter->dmaDataBufferAddr0 = dataBuffer;
                dataBuffer += dmaBufferSize / sizeof(uint32_t);
                tempDMADesBuffer +=
                    sizeof(sdif_dma_descriptor_t) / sizeof(uint32_t); /* calculate the next descriptor address */
                /* this descriptor buffer2 pointer to the next descriptor address */
                descriptorPoniter->dmaDataBufferAddr1 = tempDMADesBuffer;
            }
            /* enable the completion interrupt when reach the last descriptor */
            descriptorPoniter->dmaDesAttribute &= ~(1U << kSDIF_DisableCompleteInterrupt);
            descriptorPoniter->dmaDesAttribute |= (1U << kSDIF_DMADescriptorDataBufferEnd);
            break;

        default:
            break;
    }

    /* use internal DMA interface */
    base->CTRL |= SDIF_CTRL_USE_INTERNAL_DMAC_MASK;
    /* enable the internal SD/MMC DMA */
    base->BMOD |= SDIF_BMOD_DE_MASK;
    /* enable DMA status check */
    base->IDINTEN |= kSDIF_DMAAllStatus;
    /* load DMA descriptor buffer address */
    base->DBADDR = (uint32_t)config->dmaDesBufferStartAddr;

    return kStatus_Success;
}

#if defined(FSL_FEATURE_SDIF_ONE_INSTANCE_SUPPORT_TWO_CARD) && FSL_FEATURE_SDIF_ONE_INSTANCE_SUPPORT_TWO_CARD
/*!
 * brief set card data bus width
 * param base SDIF peripheral base address.
 * param data bus width type
 */
void SDIF_SetCardBusWidth(SDIF_Type *base, sdif_bus_width_t type)
{
    switch (type)
    {
        case kSDIF_Bus1BitWidth:
            base->CTYPE &= ~(SDIF_CTYPE_CARD0_WIDTH0_MASK | SDIF_CTYPE_CARD0_WIDTH1_MASK);
            break;
        case kSDIF_Bus4BitWidth:
            base->CTYPE = (base->CTYPE & (~SDIF_CTYPE_CARD0_WIDTH1_MASK)) | SDIF_CTYPE_CARD0_WIDTH0_MASK;
            break;
        case kSDIF_Bus8BitWidth:
            base->CTYPE |= SDIF_CTYPE_CARD0_WIDTH1_MASK;
            break;
        default:
            break;
    }
}

/*!
 * brief set card1 data bus width
 * param base SDIF peripheral base address.
 * param data bus width type
 */
void SDIF_SetCard1BusWidth(SDIF_Type *base, sdif_bus_width_t type)
{
    switch (type)
    {
        case kSDIF_Bus1BitWidth:
            base->CTYPE &= ~(SDIF_CTYPE_CARD1_WIDTH0_MASK | SDIF_CTYPE_CARD1_WIDTH1_MASK);
            break;
        case kSDIF_Bus4BitWidth:
            base->CTYPE = (base->CTYPE & (~SDIF_CTYPE_CARD1_WIDTH1_MASK)) | SDIF_CTYPE_CARD1_WIDTH0_MASK;
            break;
        case kSDIF_Bus8BitWidth:
            base->CTYPE |= SDIF_CTYPE_CARD1_WIDTH1_MASK;
            break;
        default:
            break;
    }
}
#else
/*!
 * brief set card data bus width
 * param base SDIF peripheral base address.
 * param data bus width type
 */
void SDIF_SetCardBusWidth(SDIF_Type *base, sdif_bus_width_t type)
{
    switch (type)
    {
        case kSDIF_Bus1BitWidth:
            base->CTYPE &= ~(SDIF_CTYPE_CARD_WIDTH0_MASK | SDIF_CTYPE_CARD_WIDTH1_MASK);
            break;
        case kSDIF_Bus4BitWidth:
            base->CTYPE = (base->CTYPE & (~SDIF_CTYPE_CARD_WIDTH1_MASK)) | SDIF_CTYPE_CARD_WIDTH0_MASK;
            break;
        case kSDIF_Bus8BitWidth:
            base->CTYPE |= SDIF_CTYPE_CARD_WIDTH1_MASK;
            break;
        default:
            break;
    }
}
#endif

/*!
 * brief SDIF module initialization function.
 *
 * Configures the SDIF according to the user configuration.
 * param base SDIF peripheral base address.
 * param config SDIF configuration information.
 */
void SDIF_Init(SDIF_Type *base, sdif_config_t *config)
{
    assert(NULL != config);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(kCLOCK_Sdio);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(kSDIO_RST_SHIFT_RSTn);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

    /*config timeout register */
    base->TMOUT = ((base->TMOUT) & ~(SDIF_TMOUT_RESPONSE_TIMEOUT_MASK | SDIF_TMOUT_DATA_TIMEOUT_MASK)) |
                  SDIF_TMOUT_RESPONSE_TIMEOUT(config->responseTimeout) | SDIF_TMOUT_DATA_TIMEOUT(config->dataTimeout);

    /* config the card detect debounce clock count */
    base->DEBNCE = SDIF_DEBNCE_DEBOUNCE_COUNT(config->cardDetDebounce_Clock);

    /*config the watermark/burst transfer value */
    base->FIFOTH =
        SDIF_FIFOTH_TX_WMARK(SDIF_TX_WATERMARK) | SDIF_FIFOTH_RX_WMARK(SDIF_RX_WATERMARK) | SDIF_FIFOTH_DMA_MTS(1U);

    /* enable the interrupt status  */
    SDIF_EnableInterrupt(base, kSDIF_AllInterruptStatus);

    /* clear all interrupt/DMA status */
    SDIF_ClearInterruptStatus(base, kSDIF_AllInterruptStatus);
    SDIF_ClearInternalDMAStatus(base, kSDIF_DMAAllStatus);
}

/*!
 * brief SDIF transfer function data/cmd in a blocking way
 * param base SDIF peripheral base address.
 * param DMA config structure
 *       1. NULL
 *           In this condition, polling transfer mode is selected
 *       2. avaliable DMA config
 *           In this condition, DMA transfer mode is selected
 * param sdif transfer configuration collection
 */
status_t SDIF_TransferBlocking(SDIF_Type *base, sdif_dma_config_t *dmaConfig, sdif_transfer_t *transfer)
{
    assert(NULL != transfer);

    bool enDMA = true;
    sdif_data_t *data = transfer->data;
    status_t error = kStatus_Fail;

    /* if need transfer data in dma mode, config the DMA descriptor first */
    if ((data != NULL) && (dmaConfig != NULL))
    {
        if ((error = SDIF_InternalDMAConfig(base, dmaConfig, data->rxData ? data->rxData : data->txData,
                                            data->blockSize * data->blockCount)) ==
            kStatus_SDIF_DescriptorBufferLenError)
        {
            return kStatus_SDIF_DescriptorBufferLenError;
        }
        /* if DMA descriptor address or data buffer address not align with SDIF_INTERNAL_DMA_ADDR_ALIGN, switch to
        polling transfer mode, disable the internal DMA */
        if (error == kStatus_SDIF_DMAAddrNotAlign)
        {
            enDMA = false;
        }
    }
    else
    {
        enDMA = false;
    }

    if (!enDMA)
    {
        SDIF_EnableInternalDMA(base, false);
        /* reset FIFO and clear RAW status for host transfer */
        SDIF_Reset(base, kSDIF_ResetFIFO, SDIF_TIMEOUT_VALUE);
        SDIF_ClearInterruptStatus(base, kSDIF_AllInterruptStatus);
    }

    /* config the transfer parameter */
    if (SDIF_TransferConfig(base, transfer, enDMA) != kStatus_Success)
    {
        return kStatus_SDIF_InvalidArgument;
    }

    /* send command first */
    if (SDIF_SendCommand(base, transfer->command, SDIF_TIMEOUT_VALUE) != kStatus_Success)
    {
        return kStatus_SDIF_SyncCmdTimeout;
    }

    /* wait the command transfer done and check if error occurs */
    if (SDIF_WaitCommandDone(base, transfer->command) != kStatus_Success)
    {
        return kStatus_SDIF_SendCmdFail;
    }

    /* if use DMA transfer mode ,check the corresponding status bit */
    if (data != NULL)
    {
        /* handle data transfer */
        if (SDIF_TransferDataBlocking(base, data, enDMA) != kStatus_Success)
        {
            return kStatus_SDIF_DataTransferFail;
        }
    }

    return kStatus_Success;
}

/*!
 * brief SDIF transfer function data/cmd in a non-blocking way
 * this API should be use in interrupt mode, when use this API user
 * must call SDIF_TransferCreateHandle first, all status check through
 * interrupt
 * param base SDIF peripheral base address.
 * param sdif handle
 * param DMA config structure
 *  This parameter can be config as:
 *      1. NULL
            In this condition, polling transfer mode is selected
        2. avaliable DMA config
            In this condition, DMA transfer mode is selected
 * param sdif transfer configuration collection
 */
status_t SDIF_TransferNonBlocking(SDIF_Type *base,
                                  sdif_handle_t *handle,
                                  sdif_dma_config_t *dmaConfig,
                                  sdif_transfer_t *transfer)
{
    assert(NULL != transfer);

    sdif_data_t *data = transfer->data;
    status_t error = kStatus_Fail;
    bool enDMA = true;

    /* save the data and command before transfer */
    handle->data = transfer->data;
    handle->command = transfer->command;
    handle->transferredWords = 0U;
    handle->interruptFlags = 0U;
    handle->dmaInterruptFlags = 0U;

    if ((data != NULL) && (dmaConfig != NULL))
    {
        /* use internal DMA mode to transfer between the card and host*/
        if ((error = SDIF_InternalDMAConfig(base, dmaConfig, data->rxData ? data->rxData : data->txData,
                                            data->blockSize * data->blockCount)) ==
            kStatus_SDIF_DescriptorBufferLenError)
        {
            return kStatus_SDIF_DescriptorBufferLenError;
        }
        /* if DMA descriptor address or data buffer address not align with SDIF_INTERNAL_DMA_ADDR_ALIGN, switch to
        polling transfer mode, disable the internal DMA */
        if (error == kStatus_SDIF_DMAAddrNotAlign)
        {
            enDMA = false;
        }
    }
    else
    {
        enDMA = false;
    }

    if (!enDMA)
    {
        SDIF_EnableInternalDMA(base, false);
        /* reset FIFO and clear RAW status for host transfer */
        SDIF_Reset(base, kSDIF_ResetFIFO, SDIF_TIMEOUT_VALUE);
        SDIF_ClearInterruptStatus(base, kSDIF_AllInterruptStatus);
    }

    /* config the transfer parameter */
    if (SDIF_TransferConfig(base, transfer, enDMA) != kStatus_Success)
    {
        return kStatus_SDIF_InvalidArgument;
    }

    /* send command first */
    if (SDIF_SendCommand(base, transfer->command, SDIF_TIMEOUT_VALUE) != kStatus_Success)
    {
        return kStatus_SDIF_SyncCmdTimeout;
    }

    return kStatus_Success;
}

/*!
 * brief Creates the SDIF handle.
 * register call back function for interrupt and enable the interrupt
 * param base SDIF peripheral base address.
 * param handle SDIF handle pointer.
 * param callback Structure pointer to contain all callback functions.
 * param userData Callback function parameter.
 */
void SDIF_TransferCreateHandle(SDIF_Type *base,
                               sdif_handle_t *handle,
                               sdif_transfer_callback_t *callback,
                               void *userData)
{
    assert(handle);
    assert(callback);

    /* reset the handle. */
    memset(handle, 0U, sizeof(*handle));

    /* Set the callback. */
    handle->callback.SDIOInterrupt = callback->SDIOInterrupt;
    handle->callback.DMADesUnavailable = callback->DMADesUnavailable;
    handle->callback.CommandReload = callback->CommandReload;
    handle->callback.TransferComplete = callback->TransferComplete;
    handle->callback.cardInserted = callback->cardInserted;
    handle->userData = userData;

    /* Save the handle in global variables to support the double weak mechanism. */
    s_sdifHandle[SDIF_GetInstance(base)] = handle;

    /* save IRQ handler */
    s_sdifIsr = SDIF_TransferHandleIRQ;

    /* enable the global interrupt */
    SDIF_EnableGlobalInterrupt(base, true);

    EnableIRQ(s_sdifIRQ[SDIF_GetInstance(base)]);
}

/*!
 * brief SDIF return the controller capability
 * param base SDIF peripheral base address.
 * param sdif capability pointer
 */
void SDIF_GetCapability(SDIF_Type *base, sdif_capability_t *capability)
{
    assert(NULL != capability);

    /* Initializes the configure structure to zero. */
    memset(capability, 0, sizeof(*capability));

    capability->sdVersion = SDIF_SUPPORT_SD_VERSION;
    capability->mmcVersion = SDIF_SUPPORT_MMC_VERSION;
    capability->maxBlockLength = SDIF_BLKSIZ_BLOCK_SIZE_MASK;
    /* set the max block count = max byte count / max block size */
    capability->maxBlockCount = SDIF_BYTCNT_BYTE_COUNT_MASK / SDIF_BLKSIZ_BLOCK_SIZE_MASK;
    capability->flags = kSDIF_SupportHighSpeedFlag | kSDIF_SupportDmaFlag | kSDIF_SupportSuspendResumeFlag |
                        kSDIF_SupportV330Flag | kSDIF_Support4BitFlag | kSDIF_Support8BitFlag;
}

static void SDIF_TransferHandleCommand(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags)
{
    assert(handle->command);

    /* cmd buffer full, in this condition user need re-send the command */
    if (interruptFlags & kSDIF_HardwareLockError)
    {
        if (handle->callback.CommandReload)
        {
            handle->callback.CommandReload(base, handle->userData);
        }
    }
    /* transfer command done */
    else
    {
        if ((kSDIF_CommandDone & interruptFlags) != 0U)
        {
            /* transfer error */
            if (interruptFlags & (kSDIF_ResponseError | kSDIF_ResponseCRCError | kSDIF_ResponseTimeout))
            {
                handle->callback.TransferComplete(base, handle, kStatus_SDIF_SendCmdFail, handle->userData);
            }
            else
            {
                SDIF_ReadCommandResponse(base, handle->command);
                if (((handle->data) == NULL) && (handle->callback.TransferComplete))
                {
                    handle->callback.TransferComplete(base, handle, kStatus_Success, handle->userData);
                }
            }
        }
    }
}

static void SDIF_TransferHandleData(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags)
{
    assert(handle->data);

    /* data starvation by host time out, software should read/write FIFO*/
    if (interruptFlags & kSDIF_DataStarvationByHostTimeout)
    {
        if (handle->data->rxData != NULL)
        {
            handle->transferredWords = SDIF_ReadDataPort(base, handle->data, handle->transferredWords);
        }
        else if (handle->data->txData != NULL)
        {
            handle->transferredWords = SDIF_WriteDataPort(base, handle->data, handle->transferredWords);
        }
        else
        {
            handle->callback.TransferComplete(base, handle, kStatus_SDIF_DataTransferFail, handle->userData);
        }
    }
    /* data transfer fail */
    else if (interruptFlags & kSDIF_DataTransferError)
    {
        if (!handle->data->enableIgnoreError)
        {
            handle->callback.TransferComplete(base, handle, kStatus_SDIF_DataTransferFail, handle->userData);
        }
    }
    /* need fill data to FIFO */
    else if (interruptFlags & kSDIF_WriteFIFORequest)
    {
        handle->transferredWords = SDIF_WriteDataPort(base, handle->data, handle->transferredWords);
    }
    /* need read data from FIFO */
    else if (interruptFlags & kSDIF_ReadFIFORequest)
    {
        handle->transferredWords = SDIF_ReadDataPort(base, handle->data, handle->transferredWords);
    }
    else
    {
    }

    /* data transfer over */
    if (interruptFlags & kSDIF_DataTransferOver)
    {
        while ((handle->data->rxData != NULL) && ((base->STATUS & SDIF_STATUS_FIFO_COUNT_MASK) != 0U))
        {
            handle->transferredWords = SDIF_ReadDataPort(base, handle->data, handle->transferredWords);
        }
        handle->callback.TransferComplete(base, handle, kStatus_Success, handle->userData);
    }
}

static void SDIF_TransferHandleDMA(SDIF_Type *base, sdif_handle_t *handle, uint32_t interruptFlags)
{
    if (interruptFlags & kSDIF_DMAFatalBusError)
    {
        handle->callback.TransferComplete(base, handle, kStatus_SDIF_DMATransferFailWithFBE, handle->userData);
    }
    else if (interruptFlags & kSDIF_DMADescriptorUnavailable)
    {
        if (handle->callback.DMADesUnavailable)
        {
            handle->callback.DMADesUnavailable(base, handle->userData);
        }
    }
    else if ((interruptFlags & (kSDIF_AbnormalInterruptSummary | kSDIF_DMACardErrorSummary)) &&
             (!handle->data->enableIgnoreError))
    {
        handle->callback.TransferComplete(base, handle, kStatus_SDIF_DataTransferFail, handle->userData);
    }
    /* card normal summary */
    else
    {
        handle->callback.TransferComplete(base, handle, kStatus_Success, handle->userData);
    }
}

static void SDIF_TransferHandleSDIOInterrupt(SDIF_Type *base, sdif_handle_t *handle)
{
    if (handle->callback.SDIOInterrupt != NULL)
    {
        handle->callback.SDIOInterrupt(base, handle->userData);
    }
}

static void SDIF_TransferHandleCardDetect(SDIF_Type *base, sdif_handle_t *handle)
{
    if (SDIF_DetectCardInsert(base, false))
    {
        if ((handle->callback.cardInserted) != NULL)
        {
            handle->callback.cardInserted(base, handle->userData);
        }
    }
    else
    {
        if ((handle->callback.cardRemoved) != NULL)
        {
            handle->callback.cardRemoved(base, handle->userData);
        }
    }
}

static void SDIF_TransferHandleIRQ(SDIF_Type *base, sdif_handle_t *handle)
{
    assert(handle);

    uint32_t interruptFlags, dmaInterruptFlags;

    interruptFlags = SDIF_GetInterruptStatus(base);
    dmaInterruptFlags = SDIF_GetInternalDMAStatus(base);

    handle->interruptFlags = interruptFlags;
    handle->dmaInterruptFlags = dmaInterruptFlags;

    if ((interruptFlags & kSDIF_CommandTransferStatus) != 0U)
    {
        SDIF_TransferHandleCommand(base, handle, (interruptFlags & kSDIF_CommandTransferStatus));
    }
    if ((interruptFlags & kSDIF_DataTransferStatus) != 0U)
    {
        SDIF_TransferHandleData(base, handle, (interruptFlags & kSDIF_DataTransferStatus));
    }
    if (interruptFlags & kSDIF_SDIOInterrupt)
    {
        SDIF_TransferHandleSDIOInterrupt(base, handle);
    }
    if (dmaInterruptFlags & kSDIF_DMAAllStatus)
    {
        SDIF_TransferHandleDMA(base, handle, dmaInterruptFlags);
    }
    if (interruptFlags & kSDIF_CardDetect)
    {
        SDIF_TransferHandleCardDetect(base, handle);
    }

    SDIF_ClearInterruptStatus(base, interruptFlags);
    SDIF_ClearInternalDMAStatus(base, dmaInterruptFlags);
}

/*!
 * brief SDIF module deinit function.
 * user should call this function follow with IP reset
 * param base SDIF peripheral base address.
 */
void SDIF_Deinit(SDIF_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(kCLOCK_Sdio);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* disable the SDIOCLKCTRL */
    SYSCON->SDIOCLKCTRL &= ~(SYSCON_SDIOCLKCTRL_CCLK_SAMPLE_DELAY_ACTIVE_MASK |
                             SYSCON_SDIOCLKCTRL_CCLK_DRV_DELAY_ACTIVE_MASK | SYSCON_SDIOCLKCTRL_PHASE_ACTIVE_MASK);

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(kSDIO_RST_SHIFT_RSTn);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */
}

#if defined(SDIF)
void SDIF_DriverIRQHandler(void)
{
    assert(s_sdifHandle[0]);

    s_sdifIsr(SDIF, s_sdifHandle[0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
