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

#include "fsl_dma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for DMA.
 *
 * @param base DMA peripheral base address.
 */
static uint32_t DMA_GetInstance(DMA_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Array to map DMA instance number to base pointer. */
static DMA_Type *const s_dmaBases[] = DMA_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map DMA instance number to clock name. */
static const clock_ip_name_t s_dmaClockName[] = DMA_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Array to map DMA instance number to IRQ number. */
static const IRQn_Type s_dmaIRQNumber[][FSL_FEATURE_DMA_MODULE_CHANNEL] = DMA_CHN_IRQS;

/*! @brief Pointers to transfer handle for each DMA channel. */
static dma_handle_t *s_DMAHandle[FSL_FEATURE_DMA_MODULE_CHANNEL * FSL_FEATURE_SOC_DMA_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t DMA_GetInstance(DMA_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dmaBases); instance++)
    {
        if (s_dmaBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dmaBases));

    return instance;
}

void DMA_Init(DMA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(s_dmaClockName[DMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void DMA_Deinit(DMA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_dmaClockName[DMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void DMA_ResetChannel(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMA_MODULE_CHANNEL);

    /* clear all status bit */
    base->DMA[channel].DSR_BCR |= DMA_DSR_BCR_DONE(true);
    /* clear all registers */
    base->DMA[channel].SAR = 0;
    base->DMA[channel].DAR = 0;
    base->DMA[channel].DSR_BCR = 0;
    /* enable cycle steal and enable auto disable channel request */
    base->DMA[channel].DCR = DMA_DCR_D_REQ(true) | DMA_DCR_CS(true);
}

void DMA_SetTransferConfig(DMA_Type *base, uint32_t channel, const dma_transfer_config_t *config)
{
    assert(channel < FSL_FEATURE_DMA_MODULE_CHANNEL);
    assert(config != NULL);

    uint32_t tmpreg;

    /* Set source address */
    base->DMA[channel].SAR = config->srcAddr;
    /* Set destination address */
    base->DMA[channel].DAR = config->destAddr;
    /* Set transfer bytes */
    base->DMA[channel].DSR_BCR = DMA_DSR_BCR_BCR(config->transferSize);
    /* Set DMA Control Register */
    tmpreg = base->DMA[channel].DCR;
    tmpreg &= ~(DMA_DCR_DSIZE_MASK | DMA_DCR_DINC_MASK | DMA_DCR_SSIZE_MASK | DMA_DCR_SINC_MASK);
    tmpreg |= (DMA_DCR_DSIZE(config->destSize) | DMA_DCR_DINC(config->enableDestIncrement) |
               DMA_DCR_SSIZE(config->srcSize) | DMA_DCR_SINC(config->enableSrcIncrement));
    base->DMA[channel].DCR = tmpreg;
}

void DMA_SetChannelLinkConfig(DMA_Type *base, uint32_t channel, const dma_channel_link_config_t *config)
{
    assert(channel < FSL_FEATURE_DMA_MODULE_CHANNEL);
    assert(config != NULL);

    uint32_t tmpreg;

    tmpreg = base->DMA[channel].DCR;
    tmpreg &= ~(DMA_DCR_LINKCC_MASK | DMA_DCR_LCH1_MASK | DMA_DCR_LCH2_MASK);
    tmpreg |= (DMA_DCR_LINKCC(config->linkType) | DMA_DCR_LCH1(config->channel1) | DMA_DCR_LCH2(config->channel2));
    base->DMA[channel].DCR = tmpreg;
}

void DMA_SetModulo(DMA_Type *base, uint32_t channel, dma_modulo_t srcModulo, dma_modulo_t destModulo)
{
    assert(channel < FSL_FEATURE_DMA_MODULE_CHANNEL);

    uint32_t tmpreg;

    tmpreg = base->DMA[channel].DCR;
    tmpreg &= ~(DMA_DCR_SMOD_MASK | DMA_DCR_DMOD_MASK);
    tmpreg |= (DMA_DCR_SMOD(srcModulo) | DMA_DCR_DMOD(destModulo));
    base->DMA[channel].DCR = tmpreg;
}

void DMA_CreateHandle(dma_handle_t *handle, DMA_Type *base, uint32_t channel)
{
    assert(handle != NULL);
    assert(channel < FSL_FEATURE_DMA_MODULE_CHANNEL);

    uint32_t dmaInstance;
    uint32_t channelIndex;

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    handle->base = base;
    handle->channel = channel;
    /* Get the DMA instance number */
    dmaInstance = DMA_GetInstance(base);
    channelIndex = (dmaInstance * FSL_FEATURE_DMA_MODULE_CHANNEL) + channel;
    /* Store handle */
    s_DMAHandle[channelIndex] = handle;
    /* Enable NVIC interrupt. */
    EnableIRQ(s_dmaIRQNumber[dmaInstance][channelIndex]);
}

void DMA_PrepareTransfer(dma_transfer_config_t *config,
                         void *srcAddr,
                         uint32_t srcWidth,
                         void *destAddr,
                         uint32_t destWidth,
                         uint32_t transferBytes,
                         dma_transfer_type_t type)
{
    assert(config != NULL);
    assert(srcAddr != NULL);
    assert(destAddr != NULL);
    assert((srcWidth == 1U) || (srcWidth == 2U) || (srcWidth == 4U));
    assert((destWidth == 1U) || (destWidth == 2U) || (destWidth == 4U));

    config->srcAddr = (uint32_t)srcAddr;
    config->destAddr = (uint32_t)destAddr;
    config->transferSize = transferBytes;
    switch (srcWidth)
    {
        case 1U:
            config->srcSize = kDMA_Transfersize8bits;
            break;
        case 2U:
            config->srcSize = kDMA_Transfersize16bits;
            break;
        case 4U:
            config->srcSize = kDMA_Transfersize32bits;
            break;
        default:
            break;
    }
    switch (destWidth)
    {
        case 1U:
            config->destSize = kDMA_Transfersize8bits;
            break;
        case 2U:
            config->destSize = kDMA_Transfersize16bits;
            break;
        case 4U:
            config->destSize = kDMA_Transfersize32bits;
            break;
        default:
            break;
    }
    switch (type)
    {
        case kDMA_MemoryToMemory:
            config->enableSrcIncrement = true;
            config->enableDestIncrement = true;
            break;
        case kDMA_PeripheralToMemory:
            config->enableSrcIncrement = false;
            config->enableDestIncrement = true;
            break;
        case kDMA_MemoryToPeripheral:
            config->enableSrcIncrement = true;
            config->enableDestIncrement = false;
            break;
        default:
            break;
    }
}

void DMA_SetCallback(dma_handle_t *handle, dma_callback callback, void *userData)
{
    assert(handle != NULL);

    handle->callback = callback;
    handle->userData = userData;
}

status_t DMA_SubmitTransfer(dma_handle_t *handle, const dma_transfer_config_t *config, uint32_t options)
{
    assert(handle != NULL);
    assert(config != NULL);

    /* Check if DMA is busy */
    if (handle->base->DMA[handle->channel].DSR_BCR & DMA_DSR_BCR_BSY_MASK)
    {
        return kStatus_DMA_Busy;
    }
    DMA_ResetChannel(handle->base, handle->channel);
    DMA_SetTransferConfig(handle->base, handle->channel, config);
    if (options & kDMA_EnableInterrupt)
    {
        DMA_EnableInterrupts(handle->base, handle->channel);
    }
    return kStatus_Success;
}

void DMA_AbortTransfer(dma_handle_t *handle)
{
    assert(handle != NULL);

    handle->base->DMA[handle->channel].DCR &= ~DMA_DCR_ERQ_MASK;
    /* clear all status bit */
    handle->base->DMA[handle->channel].DSR_BCR |= DMA_DSR_BCR_DONE(true);
}

void DMA_HandleIRQ(dma_handle_t *handle)
{
    assert(handle != NULL);

    /* Clear interrupt pending bit */
    DMA_ClearChannelStatusFlags(handle->base, handle->channel, kDMA_TransactionsDoneFlag);
    if (handle->callback)
    {
        (handle->callback)(handle, handle->userData);
    }
}

#if defined(FSL_FEATURE_DMA_MODULE_CHANNEL) && (FSL_FEATURE_DMA_MODULE_CHANNEL == 4U)
void DMA0_DriverIRQHandler(void)
{
    DMA_HandleIRQ(s_DMAHandle[0]);
}

void DMA1_DriverIRQHandler(void)
{
    DMA_HandleIRQ(s_DMAHandle[1]);
}

void DMA2_DriverIRQHandler(void)
{
    DMA_HandleIRQ(s_DMAHandle[2]);
}

void DMA3_DriverIRQHandler(void)
{
    DMA_HandleIRQ(s_DMAHandle[3]);
}
#endif /* FSL_FEATURE_DMA_MODULE_CHANNEL */
