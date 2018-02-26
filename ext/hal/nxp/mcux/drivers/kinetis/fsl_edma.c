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

#include "fsl_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define EDMA_TRANSFER_ENABLED_MASK 0x80U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for EDMA.
 *
 * @param base EDMA peripheral base address.
 */
static uint32_t EDMA_GetInstance(DMA_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Array to map EDMA instance number to base pointer. */
static DMA_Type *const s_edmaBases[] = DMA_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map EDMA instance number to clock name. */
static const clock_ip_name_t s_edmaClockName[] = EDMA_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Array to map EDMA instance number to IRQ number. */
static const IRQn_Type s_edmaIRQNumber[][FSL_FEATURE_EDMA_MODULE_CHANNEL] = DMA_CHN_IRQS;

/*! @brief Pointers to transfer handle for each EDMA channel. */
static edma_handle_t *s_EDMAHandle[FSL_FEATURE_EDMA_MODULE_CHANNEL * FSL_FEATURE_SOC_EDMA_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t EDMA_GetInstance(DMA_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_edmaBases); instance++)
    {
        if (s_edmaBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_edmaBases));

    return instance;
}

void EDMA_InstallTCD(DMA_Type *base, uint32_t channel, edma_tcd_t *tcd)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    /* Push tcd into hardware TCD register */
    base->TCD[channel].SADDR = tcd->SADDR;
    base->TCD[channel].SOFF = tcd->SOFF;
    base->TCD[channel].ATTR = tcd->ATTR;
    base->TCD[channel].NBYTES_MLNO = tcd->NBYTES;
    base->TCD[channel].SLAST = tcd->SLAST;
    base->TCD[channel].DADDR = tcd->DADDR;
    base->TCD[channel].DOFF = tcd->DOFF;
    base->TCD[channel].CITER_ELINKNO = tcd->CITER;
    base->TCD[channel].DLAST_SGA = tcd->DLAST_SGA;
    /* Clear DONE bit first, otherwise ESG cannot be set */
    base->TCD[channel].CSR = 0;
    base->TCD[channel].CSR = tcd->CSR;
    base->TCD[channel].BITER_ELINKNO = tcd->BITER;
}

void EDMA_Init(DMA_Type *base, const edma_config_t *config)
{
    assert(config != NULL);

    uint32_t tmpreg;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate EDMA periphral clock */
    CLOCK_EnableClock(s_edmaClockName[EDMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* Configure EDMA peripheral according to the configuration structure. */
    tmpreg = base->CR;
    tmpreg &= ~(DMA_CR_ERCA_MASK | DMA_CR_HOE_MASK | DMA_CR_CLM_MASK | DMA_CR_EDBG_MASK);
    tmpreg |= (DMA_CR_ERCA(config->enableRoundRobinArbitration) | DMA_CR_HOE(config->enableHaltOnError) |
               DMA_CR_CLM(config->enableContinuousLinkMode) | DMA_CR_EDBG(config->enableDebugMode) | DMA_CR_EMLM(true));
    base->CR = tmpreg;
}

void EDMA_Deinit(DMA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate EDMA periphral clock */
    CLOCK_DisableClock(s_edmaClockName[EDMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void EDMA_GetDefaultConfig(edma_config_t *config)
{
    assert(config != NULL);

    config->enableRoundRobinArbitration = false;
    config->enableHaltOnError = true;
    config->enableContinuousLinkMode = false;
    config->enableDebugMode = false;
}

void EDMA_ResetChannel(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    EDMA_TcdReset((edma_tcd_t *)&base->TCD[channel]);
}

void EDMA_SetTransferConfig(DMA_Type *base, uint32_t channel, const edma_transfer_config_t *config, edma_tcd_t *nextTcd)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);
    assert(((uint32_t)nextTcd & 0x1FU) == 0);

    EDMA_TcdSetTransferConfig((edma_tcd_t *)&base->TCD[channel], config, nextTcd);
}

void EDMA_SetMinorOffsetConfig(DMA_Type *base, uint32_t channel, const edma_minor_offset_config_t *config)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);

    uint32_t tmpreg;

    tmpreg = base->TCD[channel].NBYTES_MLOFFYES;
    tmpreg &= ~(DMA_NBYTES_MLOFFYES_SMLOE_MASK | DMA_NBYTES_MLOFFYES_DMLOE_MASK | DMA_NBYTES_MLOFFYES_MLOFF_MASK);
    tmpreg |=
        (DMA_NBYTES_MLOFFYES_SMLOE(config->enableSrcMinorOffset) |
         DMA_NBYTES_MLOFFYES_DMLOE(config->enableDestMinorOffset) | DMA_NBYTES_MLOFFYES_MLOFF(config->minorOffset));
    base->TCD[channel].NBYTES_MLOFFYES = tmpreg;
}

void EDMA_SetChannelLink(DMA_Type *base, uint32_t channel, edma_channel_link_type_t type, uint32_t linkedChannel)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(linkedChannel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    EDMA_TcdSetChannelLink((edma_tcd_t *)&base->TCD[channel], type, linkedChannel);
}

void EDMA_SetBandWidth(DMA_Type *base, uint32_t channel, edma_bandwidth_t bandWidth)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    base->TCD[channel].CSR = (base->TCD[channel].CSR & (~DMA_CSR_BWC_MASK)) | DMA_CSR_BWC(bandWidth);
}

void EDMA_SetModulo(DMA_Type *base, uint32_t channel, edma_modulo_t srcModulo, edma_modulo_t destModulo)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t tmpreg;

    tmpreg = base->TCD[channel].ATTR & (~(DMA_ATTR_SMOD_MASK | DMA_ATTR_DMOD_MASK));
    base->TCD[channel].ATTR = tmpreg | DMA_ATTR_DMOD(destModulo) | DMA_ATTR_SMOD(srcModulo);
}

void EDMA_EnableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Enable error interrupt */
    if (mask & kEDMA_ErrorInterruptEnable)
    {
        base->EEI |= (0x1U << channel);
    }

    /* Enable Major interrupt */
    if (mask & kEDMA_MajorInterruptEnable)
    {
        base->TCD[channel].CSR |= DMA_CSR_INTMAJOR_MASK;
    }

    /* Enable Half major interrupt */
    if (mask & kEDMA_HalfInterruptEnable)
    {
        base->TCD[channel].CSR |= DMA_CSR_INTHALF_MASK;
    }
}

void EDMA_DisableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Disable error interrupt */
    if (mask & kEDMA_ErrorInterruptEnable)
    {
        base->EEI &= ~(0x1U << channel);
    }

    /* Disable Major interrupt */
    if (mask & kEDMA_MajorInterruptEnable)
    {
        base->TCD[channel].CSR &= ~DMA_CSR_INTMAJOR_MASK;
    }

    /* Disable Half major interrupt */
    if (mask & kEDMA_HalfInterruptEnable)
    {
        base->TCD[channel].CSR &= ~DMA_CSR_INTHALF_MASK;
    }
}

void EDMA_TcdReset(edma_tcd_t *tcd)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    /* Reset channel TCD */
    tcd->SADDR = 0U;
    tcd->SOFF = 0U;
    tcd->ATTR = 0U;
    tcd->NBYTES = 0U;
    tcd->SLAST = 0U;
    tcd->DADDR = 0U;
    tcd->DOFF = 0U;
    tcd->CITER = 0U;
    tcd->DLAST_SGA = 0U;
    /* Enable auto disable request feature */
    tcd->CSR = DMA_CSR_DREQ(true);
    tcd->BITER = 0U;
}

void EDMA_TcdSetTransferConfig(edma_tcd_t *tcd, const edma_transfer_config_t *config, edma_tcd_t *nextTcd)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);
    assert(config != NULL);
    assert(((uint32_t)nextTcd & 0x1FU) == 0);

    /* source address */
    tcd->SADDR = config->srcAddr;
    /* destination address */
    tcd->DADDR = config->destAddr;
    /* Source data and destination data transfer size */
    tcd->ATTR = DMA_ATTR_SSIZE(config->srcTransferSize) | DMA_ATTR_DSIZE(config->destTransferSize);
    /* Source address signed offset */
    tcd->SOFF = config->srcOffset;
    /* Destination address signed offset */
    tcd->DOFF = config->destOffset;
    /* Minor byte transfer count */
    tcd->NBYTES = config->minorLoopBytes;
    /* Current major iteration count */
    tcd->CITER = config->majorLoopCounts;
    /* Starting major iteration count */
    tcd->BITER = config->majorLoopCounts;
    /* Enable scatter/gather processing */
    if (nextTcd != NULL)
    {
        tcd->DLAST_SGA = (uint32_t)nextTcd;
        /*
            Before call EDMA_TcdSetTransferConfig or EDMA_SetTransferConfig,
            user must call EDMA_TcdReset or EDMA_ResetChannel which will set
            DREQ, so must use "|" or "&" rather than "=".

            Clear the DREQ bit because scatter gather has been enabled, so the
            previous transfer is not the last transfer, and channel request should
            be enabled at the next transfer(the next TCD).
        */
        tcd->CSR = (tcd->CSR | DMA_CSR_ESG_MASK) & ~DMA_CSR_DREQ_MASK;
    }
}

void EDMA_TcdSetMinorOffsetConfig(edma_tcd_t *tcd, const edma_minor_offset_config_t *config)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    uint32_t tmpreg;

    tmpreg = tcd->NBYTES &
             ~(DMA_NBYTES_MLOFFYES_SMLOE_MASK | DMA_NBYTES_MLOFFYES_DMLOE_MASK | DMA_NBYTES_MLOFFYES_MLOFF_MASK);
    tmpreg |=
        (DMA_NBYTES_MLOFFYES_SMLOE(config->enableSrcMinorOffset) |
         DMA_NBYTES_MLOFFYES_DMLOE(config->enableDestMinorOffset) | DMA_NBYTES_MLOFFYES_MLOFF(config->minorOffset));
    tcd->NBYTES = tmpreg;
}

void EDMA_TcdSetChannelLink(edma_tcd_t *tcd, edma_channel_link_type_t type, uint32_t linkedChannel)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);
    assert(linkedChannel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    if (type == kEDMA_MinorLink) /* Minor link config */
    {
        uint32_t tmpreg;

        /* Enable minor link */
        tcd->CITER |= DMA_CITER_ELINKYES_ELINK_MASK;
        tcd->BITER |= DMA_BITER_ELINKYES_ELINK_MASK;
        /* Set likned channel */
        tmpreg = tcd->CITER & (~DMA_CITER_ELINKYES_LINKCH_MASK);
        tmpreg |= DMA_CITER_ELINKYES_LINKCH(linkedChannel);
        tcd->CITER = tmpreg;
        tmpreg = tcd->BITER & (~DMA_BITER_ELINKYES_LINKCH_MASK);
        tmpreg |= DMA_BITER_ELINKYES_LINKCH(linkedChannel);
        tcd->BITER = tmpreg;
    }
    else if (type == kEDMA_MajorLink) /* Major link config */
    {
        uint32_t tmpreg;

        /* Enable major link */
        tcd->CSR |= DMA_CSR_MAJORELINK_MASK;
        /* Set major linked channel */
        tmpreg = tcd->CSR & (~DMA_CSR_MAJORLINKCH_MASK);
        tcd->CSR = tmpreg | DMA_CSR_MAJORLINKCH(linkedChannel);
    }
    else /* Link none */
    {
        tcd->CITER &= ~DMA_CITER_ELINKYES_ELINK_MASK;
        tcd->BITER &= ~DMA_BITER_ELINKYES_ELINK_MASK;
        tcd->CSR &= ~DMA_CSR_MAJORELINK_MASK;
    }
}

void EDMA_TcdSetModulo(edma_tcd_t *tcd, edma_modulo_t srcModulo, edma_modulo_t destModulo)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0);

    uint32_t tmpreg;

    tmpreg = tcd->ATTR & (~(DMA_ATTR_SMOD_MASK | DMA_ATTR_DMOD_MASK));
    tcd->ATTR = tmpreg | DMA_ATTR_DMOD(destModulo) | DMA_ATTR_SMOD(srcModulo);
}

void EDMA_TcdEnableInterrupts(edma_tcd_t *tcd, uint32_t mask)
{
    assert(tcd != NULL);

    /* Enable Major interrupt */
    if (mask & kEDMA_MajorInterruptEnable)
    {
        tcd->CSR |= DMA_CSR_INTMAJOR_MASK;
    }

    /* Enable Half major interrupt */
    if (mask & kEDMA_HalfInterruptEnable)
    {
        tcd->CSR |= DMA_CSR_INTHALF_MASK;
    }
}

void EDMA_TcdDisableInterrupts(edma_tcd_t *tcd, uint32_t mask)
{
    assert(tcd != NULL);

    /* Disable Major interrupt */
    if (mask & kEDMA_MajorInterruptEnable)
    {
        tcd->CSR &= ~DMA_CSR_INTMAJOR_MASK;
    }

    /* Disable Half major interrupt */
    if (mask & kEDMA_HalfInterruptEnable)
    {
        tcd->CSR &= ~DMA_CSR_INTHALF_MASK;
    }
}

uint32_t EDMA_GetRemainingMajorLoopCount(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t remainingCount = 0;

    if (DMA_CSR_DONE_MASK & base->TCD[channel].CSR)
    {
        remainingCount = 0;
    }
    else
    {
        /* Calculate the unfinished bytes */
        if (base->TCD[channel].CITER_ELINKNO & DMA_CITER_ELINKNO_ELINK_MASK)
        {
            remainingCount =
                (base->TCD[channel].CITER_ELINKYES & DMA_CITER_ELINKYES_CITER_MASK) >> DMA_CITER_ELINKYES_CITER_SHIFT;
        }
        else
        {
            remainingCount =
                (base->TCD[channel].CITER_ELINKNO & DMA_CITER_ELINKNO_CITER_MASK) >> DMA_CITER_ELINKNO_CITER_SHIFT;
        }
    }

    return remainingCount;
}

uint32_t EDMA_GetChannelStatusFlags(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t retval = 0;

    /* Get DONE bit flag */
    retval |= ((base->TCD[channel].CSR & DMA_CSR_DONE_MASK) >> DMA_CSR_DONE_SHIFT);
    /* Get ERROR bit flag */
    retval |= (((base->ERR >> channel) & 0x1U) << 1U);
    /* Get INT bit flag */
    retval |= (((base->INT >> channel) & 0x1U) << 2U);

    return retval;
}

void EDMA_ClearChannelStatusFlags(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Clear DONE bit flag */
    if (mask & kEDMA_DoneFlag)
    {
        base->CDNE = channel;
    }
    /* Clear ERROR bit flag */
    if (mask & kEDMA_ErrorFlag)
    {
        base->CERR = channel;
    }
    /* Clear INT bit flag */
    if (mask & kEDMA_InterruptFlag)
    {
        base->CINT = channel;
    }
}

static uint8_t Get_StartInstance(void)
{
    static uint8_t StartInstanceNum;

#if defined(DMA0)
    StartInstanceNum = EDMA_GetInstance(DMA0);
#elif defined(DMA1)
    StartInstanceNum = EDMA_GetInstance(DMA1);
#elif defined(DMA2)
    StartInstanceNum = EDMA_GetInstance(DMA2);
#elif defined(DMA3)
    StartInstanceNum = EDMA_GetInstance(DMA3);
#endif

    return StartInstanceNum;
}

void EDMA_CreateHandle(edma_handle_t *handle, DMA_Type *base, uint32_t channel)
{
    assert(handle != NULL);
    assert(channel < FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t edmaInstance;
    uint32_t channelIndex;
    uint8_t StartInstance;
    edma_tcd_t *tcdRegs;

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    handle->base = base;
    handle->channel = channel;
    /* Get the DMA instance number */
    edmaInstance = EDMA_GetInstance(base);
    StartInstance = Get_StartInstance();
    channelIndex = ((edmaInstance - StartInstance) * FSL_FEATURE_EDMA_MODULE_CHANNEL) + channel;
    s_EDMAHandle[channelIndex] = handle;

    /* Enable NVIC interrupt */
    EnableIRQ(s_edmaIRQNumber[edmaInstance][channel]);

    /*
       Reset TCD registers to zero. Unlike the EDMA_TcdReset(DREQ will be set),
       CSR will be 0. Because in order to suit EDMA busy check mechanism in
       EDMA_SubmitTransfer, CSR must be set 0.
    */
    tcdRegs = (edma_tcd_t *)&handle->base->TCD[handle->channel];
    tcdRegs->SADDR = 0;
    tcdRegs->SOFF = 0;
    tcdRegs->ATTR = 0;
    tcdRegs->NBYTES = 0;
    tcdRegs->SLAST = 0;
    tcdRegs->DADDR = 0;
    tcdRegs->DOFF = 0;
    tcdRegs->CITER = 0;
    tcdRegs->DLAST_SGA = 0;
    tcdRegs->CSR = 0;
    tcdRegs->BITER = 0;
}

void EDMA_InstallTCDMemory(edma_handle_t *handle, edma_tcd_t *tcdPool, uint32_t tcdSize)
{
    assert(handle != NULL);
    assert(((uint32_t)tcdPool & 0x1FU) == 0);

    /* Initialize tcd queue attibute. */
    handle->header = 0;
    handle->tail = 0;
    handle->tcdUsed = 0;
    handle->tcdSize = tcdSize;
    handle->flags = 0;
    handle->tcdPool = tcdPool;
}

void EDMA_SetCallback(edma_handle_t *handle, edma_callback callback, void *userData)
{
    assert(handle != NULL);

    handle->callback = callback;
    handle->userData = userData;
}

void EDMA_PrepareTransfer(edma_transfer_config_t *config,
                          void *srcAddr,
                          uint32_t srcWidth,
                          void *destAddr,
                          uint32_t destWidth,
                          uint32_t bytesEachRequest,
                          uint32_t transferBytes,
                          edma_transfer_type_t type)
{
    assert(config != NULL);
    assert(srcAddr != NULL);
    assert(destAddr != NULL);
    assert((srcWidth == 1U) || (srcWidth == 2U) || (srcWidth == 4U) || (srcWidth == 16U) || (srcWidth == 32U));
    assert((destWidth == 1U) || (destWidth == 2U) || (destWidth == 4U) || (destWidth == 16U) || (destWidth == 32U));
    assert(transferBytes % bytesEachRequest == 0);

    config->destAddr = (uint32_t)destAddr;
    config->srcAddr = (uint32_t)srcAddr;
    config->minorLoopBytes = bytesEachRequest;
    config->majorLoopCounts = transferBytes / bytesEachRequest;
    switch (srcWidth)
    {
        case 1U:
            config->srcTransferSize = kEDMA_TransferSize1Bytes;
            break;
        case 2U:
            config->srcTransferSize = kEDMA_TransferSize2Bytes;
            break;
        case 4U:
            config->srcTransferSize = kEDMA_TransferSize4Bytes;
            break;
        case 16U:
            config->srcTransferSize = kEDMA_TransferSize16Bytes;
            break;
        case 32U:
            config->srcTransferSize = kEDMA_TransferSize32Bytes;
            break;
        default:
            break;
    }
    switch (destWidth)
    {
        case 1U:
            config->destTransferSize = kEDMA_TransferSize1Bytes;
            break;
        case 2U:
            config->destTransferSize = kEDMA_TransferSize2Bytes;
            break;
        case 4U:
            config->destTransferSize = kEDMA_TransferSize4Bytes;
            break;
        case 16U:
            config->destTransferSize = kEDMA_TransferSize16Bytes;
            break;
        case 32U:
            config->destTransferSize = kEDMA_TransferSize32Bytes;
            break;
        default:
            break;
    }
    switch (type)
    {
        case kEDMA_MemoryToMemory:
            config->destOffset = destWidth;
            config->srcOffset = srcWidth;
            break;
        case kEDMA_MemoryToPeripheral:
            config->destOffset = 0U;
            config->srcOffset = srcWidth;
            break;
        case kEDMA_PeripheralToMemory:
            config->destOffset = destWidth;
            config->srcOffset = 0U;
            break;
        default:
            break;
    }
}

status_t EDMA_SubmitTransfer(edma_handle_t *handle, const edma_transfer_config_t *config)
{
    assert(handle != NULL);
    assert(config != NULL);

    edma_tcd_t *tcdRegs = (edma_tcd_t *)&handle->base->TCD[handle->channel];

    if (handle->tcdPool == NULL)
    {
        /*
            Check if EDMA is busy: if the given channel started transfer, CSR will be not zero. Because
            if it is the last transfer, DREQ will be set. If not, ESG will be set. So in order to suit
            this check mechanism, EDMA_CreatHandle will clear CSR register.
        */
        if ((tcdRegs->CSR != 0) && ((tcdRegs->CSR & DMA_CSR_DONE_MASK) == 0))
        {
            return kStatus_EDMA_Busy;
        }
        else
        {
            EDMA_SetTransferConfig(handle->base, handle->channel, config, NULL);
            /* Enable auto disable request feature */
            handle->base->TCD[handle->channel].CSR |= DMA_CSR_DREQ_MASK;
            /* Enable major interrupt */
            handle->base->TCD[handle->channel].CSR |= DMA_CSR_INTMAJOR_MASK;

            return kStatus_Success;
        }
    }
    else /* Use the TCD queue. */
    {
        uint32_t primask;
        uint32_t csr;
        int8_t currentTcd;
        int8_t previousTcd;
        int8_t nextTcd;

        /* Check if tcd pool is full. */
        primask = DisableGlobalIRQ();
        if (handle->tcdUsed >= handle->tcdSize)
        {
            EnableGlobalIRQ(primask);

            return kStatus_EDMA_QueueFull;
        }
        currentTcd = handle->tail;
        handle->tcdUsed++;
        /* Calculate index of next TCD */
        nextTcd = currentTcd + 1U;
        if (nextTcd == handle->tcdSize)
        {
            nextTcd = 0U;
        }
        /* Advance queue tail index */
        handle->tail = nextTcd;
        EnableGlobalIRQ(primask);
        /* Calculate index of previous TCD */
        previousTcd = currentTcd ? currentTcd - 1U : handle->tcdSize - 1U;
        /* Configure current TCD block. */
        EDMA_TcdReset(&handle->tcdPool[currentTcd]);
        EDMA_TcdSetTransferConfig(&handle->tcdPool[currentTcd], config, NULL);
        /* Enable major interrupt */
        handle->tcdPool[currentTcd].CSR |= DMA_CSR_INTMAJOR_MASK;
        /* Link current TCD with next TCD for identification of current TCD */
        handle->tcdPool[currentTcd].DLAST_SGA = (uint32_t)&handle->tcdPool[nextTcd];
        /* Chain from previous descriptor unless tcd pool size is 1(this descriptor is its own predecessor). */
        if (currentTcd != previousTcd)
        {
            /* Enable scatter/gather feature in the previous TCD block. */
            csr = (handle->tcdPool[previousTcd].CSR | DMA_CSR_ESG_MASK) & ~DMA_CSR_DREQ_MASK;
            handle->tcdPool[previousTcd].CSR = csr;
            /*
                Check if the TCD blcok in the registers is the previous one (points to current TCD block). It
                is used to check if the previous TCD linked has been loaded in TCD register. If so, it need to
                link the TCD register in case link the current TCD with the dead chain when TCD loading occurs
                before link the previous TCD block.
            */
            if (tcdRegs->DLAST_SGA == (uint32_t)&handle->tcdPool[currentTcd])
            {
                /* Enable scatter/gather also in the TCD registers. */
                csr = (tcdRegs->CSR | DMA_CSR_ESG_MASK) & ~DMA_CSR_DREQ_MASK;
                /* Must write the CSR register one-time, because the transfer maybe finished anytime. */
                tcdRegs->CSR = csr;
                /*
                    It is very important to check the ESG bit!
                    Because this hardware design: if DONE bit is set, the ESG bit can not be set. So it can
                    be used to check if the dynamic TCD link operation is successful. If ESG bit is not set
                    and the DLAST_SGA is not the next TCD address(it means the dynamic TCD link succeed and
                    the current TCD block has been loaded into TCD registers), it means transfer finished
                    and TCD link operation fail, so must install TCD content into TCD registers and enable
                    transfer again. And if ESG is set, it means transfer has notfinished, so TCD dynamic
                    link succeed.
                */
                if (tcdRegs->CSR & DMA_CSR_ESG_MASK)
                {
                    return kStatus_Success;
                }
                /*
                    Check whether the current TCD block is already loaded in the TCD registers. It is another
                    condition when ESG bit is not set: it means the dynamic TCD link succeed and the current
                    TCD block has been loaded into TCD registers.
                */
                if (tcdRegs->DLAST_SGA == (uint32_t)&handle->tcdPool[nextTcd])
                {
                    return kStatus_Success;
                }
                /*
                    If go to this, means the previous transfer finished, and the DONE bit is set.
                    So shall configure TCD registers.
                */
            }
            else if (tcdRegs->DLAST_SGA != 0)
            {
                /* The current TCD block has been linked successfully. */
                return kStatus_Success;
            }
            else
            {
                /*
                    DLAST_SGA is 0 and it means the first submit transfer, so shall configure
                    TCD registers.
                */
            }
        }
        /* There is no live chain, TCD block need to be installed in TCD registers. */
        EDMA_InstallTCD(handle->base, handle->channel, &handle->tcdPool[currentTcd]);
        /* Enable channel request again. */
        if (handle->flags & EDMA_TRANSFER_ENABLED_MASK)
        {
            handle->base->SERQ = DMA_SERQ_SERQ(handle->channel);
        }

        return kStatus_Success;
    }
}

void EDMA_StartTransfer(edma_handle_t *handle)
{
    assert(handle != NULL);

    if (handle->tcdPool == NULL)
    {
        handle->base->SERQ = DMA_SERQ_SERQ(handle->channel);
    }
    else /* Use the TCD queue. */
    {
        uint32_t primask;
        edma_tcd_t *tcdRegs = (edma_tcd_t *)&handle->base->TCD[handle->channel];

        handle->flags |= EDMA_TRANSFER_ENABLED_MASK;

        /* Check if there was at least one descriptor submitted since reset (TCD in registers is valid) */
        if (tcdRegs->DLAST_SGA != 0U)
        {
            primask = DisableGlobalIRQ();
            /* Check if channel request is actually disable. */
            if ((handle->base->ERQ & (1U << handle->channel)) == 0U)
            {
                /* Check if transfer is paused. */
                if ((!(tcdRegs->CSR & DMA_CSR_DONE_MASK)) || (tcdRegs->CSR & DMA_CSR_ESG_MASK))
                {
                    /*
                        Re-enable channel request must be as soon as possible, so must put it into
                        critical section to avoid task switching or interrupt service routine.
                    */
                    handle->base->SERQ = DMA_SERQ_SERQ(handle->channel);
                }
            }
            EnableGlobalIRQ(primask);
        }
    }
}

void EDMA_StopTransfer(edma_handle_t *handle)
{
    assert(handle != NULL);

    handle->flags &= (~EDMA_TRANSFER_ENABLED_MASK);
    handle->base->CERQ = DMA_CERQ_CERQ(handle->channel);
}

void EDMA_AbortTransfer(edma_handle_t *handle)
{
    handle->base->CERQ = DMA_CERQ_CERQ(handle->channel);
    /*
        Clear CSR to release channel. Because if the given channel started transfer,
        CSR will be not zero. Because if it is the last transfer, DREQ will be set.
        If not, ESG will be set.
    */
    handle->base->TCD[handle->channel].CSR = 0;
    /* Cancel all next TCD transfer. */
    handle->base->TCD[handle->channel].DLAST_SGA = 0;

    /* Handle the tcd */
    if (handle->tcdPool != NULL)
    {
        handle->header = 0;
        handle->tail = 0;
        handle->tcdUsed = 0;
    }
}

void EDMA_HandleIRQ(edma_handle_t *handle)
{
    assert(handle != NULL);

    /* Clear EDMA interrupt flag */
    handle->base->CINT = handle->channel;
    if ((handle->tcdPool == NULL) && (handle->callback != NULL))
    {
        (handle->callback)(handle, handle->userData, true, 0);
    }
    else /* Use the TCD queue. Please refer to the API descriptions in the eDMA header file for detailed information. */
    {
        uint32_t sga = handle->base->TCD[handle->channel].DLAST_SGA;
        uint32_t sga_index;
        int32_t tcds_done;
        uint8_t new_header;
        bool transfer_done;

        /* Check if transfer is already finished. */
        transfer_done = ((handle->base->TCD[handle->channel].CSR & DMA_CSR_DONE_MASK) != 0);
        /* Get the offset of the next transfer TCD blcoks to be loaded into the eDMA engine. */
        sga -= (uint32_t)handle->tcdPool;
        /* Get the index of the next transfer TCD blcoks to be loaded into the eDMA engine. */
        sga_index = sga / sizeof(edma_tcd_t);
        /* Adjust header positions. */
        if (transfer_done)
        {
            /* New header shall point to the next TCD to be loaded (current one is already finished) */
            new_header = sga_index;
        }
        else
        {
            /* New header shall point to this descriptor currently loaded (not finished yet) */
            new_header = sga_index ? sga_index - 1U : handle->tcdSize - 1U;
        }
        /* Calculate the number of finished TCDs */
        if (new_header == handle->header)
        {
            if (handle->tcdUsed == handle->tcdSize)
            {
                tcds_done = handle->tcdUsed;
            }
            else
            {
                /* No TCD in the memory are going to be loaded or internal error occurs. */
                tcds_done = 0;
            }
        }
        else
        {
            tcds_done = new_header - handle->header;
            if (tcds_done < 0)
            {
                tcds_done += handle->tcdSize;
            }
        }
        /* Advance header which points to the TCD to be loaded into the eDMA engine from memory. */
        handle->header = new_header;
        /* Release TCD blocks. tcdUsed is the TCD number which can be used/loaded in the memory pool. */
        handle->tcdUsed -= tcds_done;
        /* Invoke callback function. */
        if (handle->callback)
        {
            (handle->callback)(handle, handle->userData, transfer_done, tcds_done);
        }
    }
}

/* 8 channels (Shared): kl28 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL == 8U

#if defined(DMA0)
void DMA0_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(DMA1)

#if defined(DMA0)
void DMA1_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

#else
void DMA1_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
#endif
#endif /* 8 channels (Shared) */

/* 16 channels (Shared): K32H844P */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL == 16U

void DMA0_08_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_210_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_311_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_412_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_513_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_614_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_715_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

#if defined(DMA1)
void DMA1_08_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 8U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 9U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_210_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 10U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_311_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 11U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_412_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 12U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_513_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 13U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_614_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 14U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_715_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 15U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
#endif /* 16 channels (Shared) */

/* 32 channels (Shared): k80 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL == 32U

void DMA0_DMA16_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 16U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_DMA17_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 17U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA2_DMA18_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 18U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA3_DMA19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 19U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA4_DMA20_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 20U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA5_DMA21_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 21U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA6_DMA22_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 22U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA7_DMA23_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 23U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA8_DMA24_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 24U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA9_DMA25_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 25U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA10_DMA26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 26U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA11_DMA27_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 27U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA12_DMA28_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 28U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA13_DMA29_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 29U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA14_DMA30_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 30U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA15_DMA31_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 31U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* 32 channels (Shared) */

/* 32 channels (Shared): MCIMX7U5_M4 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL == 32U

void DMA0_0_4_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_1_5_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_2_6_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_3_7_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_8_12_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_9_13_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_10_14_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_11_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_16_20_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 16U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 20U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_17_21_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 17U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 21U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_18_22_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 18U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 22U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_19_23_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 19U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 23U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_24_28_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 24U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 28U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_25_29_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 25U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 29U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_26_30_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 26U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 30U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA0_27_31_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 27U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 31U) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* 32 channels (Shared): MCIMX7U5 */

/* 4 channels (No Shared): kv10  */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL > 0

void DMA0_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[0]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA1_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[1]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA2_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[2]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA3_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[3]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/* 8 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL > 4U

void DMA4_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[4]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA5_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[5]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA6_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[6]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA7_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[7]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 8 */

/* 16 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL > 8U

void DMA8_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[8]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA9_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[9]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA10_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[10]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA11_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[11]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA12_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[12]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA13_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[13]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA14_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[14]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA15_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[15]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 16 */

/* 32 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL > 16U

void DMA16_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[16]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA17_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[17]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA18_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[18]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA19_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[19]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA20_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[20]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA21_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[21]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA22_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[22]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA23_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[23]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA24_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[24]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA25_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[25]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA26_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[26]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA27_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[27]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA28_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[28]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA29_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[29]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA30_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[30]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void DMA31_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[31]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 32 */

#endif /* 4/8/16/32 channels (No Shared)  */
