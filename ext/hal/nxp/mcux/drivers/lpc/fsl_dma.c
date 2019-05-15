/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_dma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpc_dma"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for DMA.
 *
 * @param base DMA peripheral base address.
 */
static uint32_t DMA_GetInstance(DMA_Type *base);

/*!
 * @brief Get virtual channel number.
 *
 * @param base DMA peripheral base address.
 */
static uint32_t DMA_GetVirtualStartChannel(DMA_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Array to map DMA instance number to base pointer. */
static DMA_Type *const s_dmaBases[] = DMA_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map DMA instance number to clock name. */
static const clock_ip_name_t s_dmaClockName[] = DMA_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_DMA_HAS_NO_RESET) && FSL_FEATURE_DMA_HAS_NO_RESET)
/*! @brief Pointers to DMA resets for each instance. */
static const reset_ip_name_t s_dmaResets[] = DMA_RSTS_N;
#endif /*! @brief Array to map DMA instance number to IRQ number. */
static const IRQn_Type s_dmaIRQNumber[] = DMA_IRQS;

/*! @brief Pointers to transfer handle for each DMA channel. */
static dma_handle_t *s_DMAHandle[FSL_FEATURE_DMA_ALL_CHANNELS];

SDK_ALIGN(dma_descriptor_t s_dma_descriptor_table0[FSL_FEATURE_DMA_MAX_CHANNELS],
          FSL_FEATURE_DMA_DESCRIPTOR_ALIGN_SIZE);
#if defined(DMA1)
SDK_ALIGN(dma_descriptor_t s_dma_descriptor_table1[FSL_FEATURE_DMA_MAX_CHANNELS],
          FSL_FEATURE_DMA_DESCRIPTOR_ALIGN_SIZE);
static dma_descriptor_t *s_dma_descriptor_table[] = {s_dma_descriptor_table0, s_dma_descriptor_table1};
#else
static dma_descriptor_t *s_dma_descriptor_table[] = {s_dma_descriptor_table0};
#endif

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

static uint32_t DMA_GetVirtualStartChannel(DMA_Type *base)
{
    uint32_t startChannel = 0, instance = 0;
    uint32_t i = 0;

    instance = DMA_GetInstance(base);

    /* Compute start channel */
    for (i = 0; i < instance; i++)
    {
        startChannel += FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(s_dmaBases[i]);
    }

    return startChannel;
}

/*!
 * brief Initializes DMA peripheral.
 *
 * This function enable the DMA clock, set descriptor table and
 * enable DMA peripheral.
 *
 * param base DMA peripheral base address.
 */
void DMA_Init(DMA_Type *base)
{
    uint32_t instance = DMA_GetInstance(base);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* enable dma clock gate */
    CLOCK_EnableClock(s_dmaClockName[DMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_DMA_HAS_NO_RESET) && FSL_FEATURE_DMA_HAS_NO_RESET)
    /* Reset the DMA module */
    RESET_PeripheralReset(s_dmaResets[DMA_GetInstance(base)]);
#endif
    /* set descriptor table */
    base->SRAMBASE = (uint32_t)s_dma_descriptor_table[instance];
    /* enable dma peripheral */
    base->CTRL |= DMA_CTRL_ENABLE_MASK;
}

/*!
 * brief Deinitializes DMA peripheral.
 *
 * This function gates the DMA clock.
 *
 * param base DMA peripheral base address.
 */
void DMA_Deinit(DMA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_dmaClockName[DMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* Disable DMA peripheral */
    base->CTRL &= ~(DMA_CTRL_ENABLE_MASK);
}

/*!
 * brief Set trigger settings of DMA channel.
 * deprecated Do not use this function.  It has been superceded by @ref DMA_SetChannelConfig.
 *
 * param base DMA peripheral base address.
 * param channel DMA channel number.
 * param trigger trigger configuration.
 */
void DMA_ConfigureChannelTrigger(DMA_Type *base, uint32_t channel, dma_channel_trigger_t *trigger)
{
    assert((channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(base)) && (NULL != trigger));

    uint32_t tmp = (DMA_CHANNEL_CFG_HWTRIGEN_MASK | DMA_CHANNEL_CFG_TRIGPOL_MASK | DMA_CHANNEL_CFG_TRIGTYPE_MASK |
                    DMA_CHANNEL_CFG_TRIGBURST_MASK | DMA_CHANNEL_CFG_BURSTPOWER_MASK |
                    DMA_CHANNEL_CFG_SRCBURSTWRAP_MASK | DMA_CHANNEL_CFG_DSTBURSTWRAP_MASK);
    tmp = base->CHANNEL[channel].CFG & (~tmp);
    tmp |= (uint32_t)(trigger->type) | (uint32_t)(trigger->burst) | (uint32_t)(trigger->wrap);
    base->CHANNEL[channel].CFG = tmp;
}

/*!
 * brief Gets the remaining bytes of the current DMA descriptor transfer.
 *
 * param base DMA peripheral base address.
 * param channel DMA channel number.
 * return The number of bytes which have not been transferred yet.
 */
uint32_t DMA_GetRemainingBytes(DMA_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(base));

    /* NOTE: when descriptors are chained, ACTIVE bit is set for whole chain. It makes
     * impossible to distinguish between:
     * - transfer finishes (represented by value '0x3FF')
     * - and remaining 1024 bytes to transfer (value 0x3FF)
     * for all descriptor in chain, except the last one.
     * If you decide to use this function, please use 1023 transfers as maximal value */

    /* Channel not active (transfer finished) and value is 0x3FF - nothing to transfer */
    if ((!DMA_ChannelIsActive(base, channel)) &&
        (0x3FF == ((base->CHANNEL[channel].XFERCFG & DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK) >>
                   DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT)))
    {
        return 0;
    }

    return ((base->CHANNEL[channel].XFERCFG & DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK) >>
            DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT) +
           1;
}

/* Verify and convert dma_xfercfg_t to XFERCFG register */
static void DMA_SetupXferCFG(dma_xfercfg_t *xfercfg, uint32_t *xfercfg_addr)
{
    assert(xfercfg != NULL);
    /* check source increment */
    assert((xfercfg->srcInc <= kDMA_AddressInterleave4xWidth) && (xfercfg->dstInc <= kDMA_AddressInterleave4xWidth));
    /* check data width */
    assert(xfercfg->byteWidth <= kDMA_Transfer32BitWidth);
    /* check transfer count */
    assert(xfercfg->transferCount <= DMA_MAX_TRANSFER_COUNT);

    uint32_t xfer = 0;

    /* set valid flag - descriptor is ready now */
    xfer |= DMA_CHANNEL_XFERCFG_CFGVALID(xfercfg->valid);
    /* set reload - allow link to next descriptor */
    xfer |= DMA_CHANNEL_XFERCFG_RELOAD(xfercfg->reload);
    /* set swtrig flag - start transfer */
    xfer |= DMA_CHANNEL_XFERCFG_SWTRIG(xfercfg->swtrig);
    /* set transfer count */
    xfer |= DMA_CHANNEL_XFERCFG_CLRTRIG(xfercfg->clrtrig);
    /* set INTA */
    xfer |= DMA_CHANNEL_XFERCFG_SETINTA(xfercfg->intA);
    /* set INTB */
    xfer |= DMA_CHANNEL_XFERCFG_SETINTB(xfercfg->intB);
    /* set data width */
    xfer |= DMA_CHANNEL_XFERCFG_WIDTH(xfercfg->byteWidth == 4 ? 2 : xfercfg->byteWidth - 1);
    /* set source increment value */
    xfer |= DMA_CHANNEL_XFERCFG_SRCINC((xfercfg->srcInc == kDMA_AddressInterleave4xWidth) ? (xfercfg->srcInc - 1) :
                                                                                            xfercfg->srcInc);
    /* set destination increment value */
    xfer |= DMA_CHANNEL_XFERCFG_DSTINC((xfercfg->dstInc == kDMA_AddressInterleave4xWidth) ? (xfercfg->dstInc - 1) :
                                                                                            xfercfg->dstInc);
    /* set transfer count */
    xfer |= DMA_CHANNEL_XFERCFG_XFERCOUNT(xfercfg->transferCount - 1);

    /* store xferCFG */
    *xfercfg_addr = xfer;
}

/*!
 * brief setup dma descriptor
 *
 * param desc DMA descriptor address.
 * param xfercfg Transfer configuration for DMA descriptor.
 * param srcStartAddr Start address of source address.
 * param dstStartAddr Start address of destination address.
 * param nextDesc Address of next descriptor in chain.
 */
void DMA_SetupDescriptor(
    dma_descriptor_t *desc, uint32_t xfercfg, void *srcStartAddr, void *dstStartAddr, void *nextDesc)
{
    assert(((uint32_t)nextDesc & (DMA_LINK_DESCRIPTOR_ADDRESS_ALIGN - 1)) == 0U);

    uint32_t width = 0, srcInc = 0, dstInc = 0, transferCount = 0;

    width = (xfercfg & DMA_CHANNEL_XFERCFG_WIDTH_MASK) >> DMA_CHANNEL_XFERCFG_WIDTH_SHIFT;
    srcInc = (xfercfg & DMA_CHANNEL_XFERCFG_SRCINC_MASK) >> DMA_CHANNEL_XFERCFG_SRCINC_SHIFT;
    dstInc = (xfercfg & DMA_CHANNEL_XFERCFG_DSTINC_MASK) >> DMA_CHANNEL_XFERCFG_DSTINC_SHIFT;
    transferCount = ((xfercfg & DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK) >> DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT) + 1U;

    /* covert register value to actual value */
    if (width == 2U)
    {
        width = kDMA_Transfer32BitWidth;
    }
    else
    {
        width += 1U;
    }

    if (srcInc == 3U)
    {
        srcInc = kDMA_AddressInterleave4xWidth;
    }

    if (dstInc == 3U)
    {
        dstInc = kDMA_AddressInterleave4xWidth;
    }

    desc->xfercfg = xfercfg;
    desc->srcEndAddr = DMA_DESCRIPTOR_END_ADDRESS(srcStartAddr, srcInc, transferCount * width, width);
    desc->dstEndAddr = DMA_DESCRIPTOR_END_ADDRESS(dstStartAddr, dstInc, transferCount * width, width);
    ;
    desc->linkToNextDesc = nextDesc;
}

/*!
 * brief Create application specific DMA descriptor
 *        to be used in a chain in transfer
 * deprecated Do not use this function.  It has been superceded by @ref DMA_SetupDescriptor
 * param desc DMA descriptor address.
 * param xfercfg Transfer configuration for DMA descriptor.
 * param srcAddr Address of last item to transmit
 * param dstAddr Address of last item to receive.
 * param nextDesc Address of next descriptor in chain.
 */
void DMA_CreateDescriptor(dma_descriptor_t *desc, dma_xfercfg_t *xfercfg, void *srcAddr, void *dstAddr, void *nextDesc)
{
    assert(((uint32_t)nextDesc & (DMA_LINK_DESCRIPTOR_ADDRESS_ALIGN - 1)) == 0U);
    assert((NULL != srcAddr) && (0 == (uint32_t)srcAddr % xfercfg->byteWidth));
    assert((NULL != dstAddr) && (0 == (uint32_t)dstAddr % xfercfg->byteWidth));

    uint32_t xfercfg_reg = 0;

    DMA_SetupXferCFG(xfercfg, &xfercfg_reg);

    /* Set descriptor structure */
    DMA_SetupDescriptor(desc, xfercfg_reg, srcAddr, dstAddr, nextDesc);
}

/*!
 * brief Abort running transfer by handle.
 *
 * This function aborts DMA transfer specified by handle.
 *
 * param handle DMA handle pointer.
 */
void DMA_AbortTransfer(dma_handle_t *handle)
{
    assert(NULL != handle);

    DMA_DisableChannel(handle->base, handle->channel);
    while (DMA_COMMON_CONST_REG_GET(handle->base, handle->channel, BUSY) & (1U << DMA_CHANNEL_INDEX(handle->channel)))
    {
    }
    DMA_COMMON_REG_GET(handle->base, handle->channel, ABORT) |= 1U << DMA_CHANNEL_INDEX(handle->channel);
    DMA_EnableChannel(handle->base, handle->channel);
}

/*!
 * brief Creates the DMA handle.
 *
 * This function is called if using transaction API for DMA. This function
 * initializes the internal state of DMA handle.
 *
 * param handle DMA handle pointer. The DMA handle stores callback function and
 *               parameters.
 * param base DMA peripheral base address.
 * param channel DMA channel number.
 */
void DMA_CreateHandle(dma_handle_t *handle, DMA_Type *base, uint32_t channel)
{
    assert((NULL != handle) && (channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(base)));

    int32_t dmaInstance;
    uint32_t startChannel = 0;
    /* base address is invalid DMA instance */
    dmaInstance = DMA_GetInstance(base);
    startChannel = DMA_GetVirtualStartChannel(base);

    memset(handle, 0, sizeof(*handle));
    handle->base = base;
    handle->channel = channel;
    s_DMAHandle[startChannel + channel] = handle;
    /* Enable NVIC interrupt */
    EnableIRQ(s_dmaIRQNumber[dmaInstance]);
    /* Enable channel interrupt */
    DMA_EnableChannelInterrupts(handle->base, channel);
}

/*!
 * brief Installs a callback function for the DMA transfer.
 *
 * This callback is called in DMA IRQ handler. Use the callback to do something after
 * the current major loop transfer completes.
 *
 * param handle DMA handle pointer.
 * param callback DMA callback function pointer.
 * param userData Parameter for callback function.
 */
void DMA_SetCallback(dma_handle_t *handle, dma_callback callback, void *userData)
{
    assert(handle != NULL);

    handle->callback = callback;
    handle->userData = userData;
}

/*!
 * brief Prepares the DMA transfer structure.
 * deprecated Do not use this function.  It has been superceded by @ref DMA_PrepareChannelTransfer and
 * DMA_PrepareChannelXfer.
 * This function prepares the transfer configuration structure according to the user input.
 *
 * param config The user configuration structure of type dma_transfer_t.
 * param srcAddr DMA transfer source address.
 * param dstAddr DMA transfer destination address.
 * param byteWidth DMA transfer destination address width(bytes).
 * param transferBytes DMA transfer bytes to be transferred.
 * param type DMA transfer type.
 * param nextDesc Chain custom descriptor to transfer.
 * note The data address and the data width must be consistent. For example, if the SRC
 *       is 4 bytes, so the source address must be 4 bytes aligned, or it shall result in
 *       source address error(SAE).
 */
void DMA_PrepareTransfer(dma_transfer_config_t *config,
                         void *srcAddr,
                         void *dstAddr,
                         uint32_t byteWidth,
                         uint32_t transferBytes,
                         dma_transfer_type_t type,
                         void *nextDesc)
{
    uint32_t xfer_count;
    assert((NULL != config) && (NULL != srcAddr) && (NULL != dstAddr));
    assert((byteWidth == 1) || (byteWidth == 2) || (byteWidth == 4));
    assert(((uint32_t)nextDesc & (DMA_LINK_DESCRIPTOR_ADDRESS_ALIGN - 1)) == 0U);

    /* check max */
    xfer_count = transferBytes / byteWidth;
    assert((xfer_count <= DMA_MAX_TRANSFER_COUNT) && (0 == transferBytes % byteWidth));

    memset(config, 0, sizeof(*config));
    switch (type)
    {
        case kDMA_MemoryToMemory:
            config->xfercfg.srcInc = 1;
            config->xfercfg.dstInc = 1;
            config->isPeriph = false;
            break;
        case kDMA_PeripheralToMemory:
            /* Peripheral register - source doesn't increment */
            config->xfercfg.srcInc = 0;
            config->xfercfg.dstInc = 1;
            config->isPeriph = true;
            break;
        case kDMA_MemoryToPeripheral:
            /* Peripheral register - destination doesn't increment */
            config->xfercfg.srcInc = 1;
            config->xfercfg.dstInc = 0;
            config->isPeriph = true;
            break;
        case kDMA_StaticToStatic:
            config->xfercfg.srcInc = 0;
            config->xfercfg.dstInc = 0;
            config->isPeriph = true;
            break;
        default:
            return;
    }

    config->dstAddr = (uint8_t *)dstAddr;
    config->srcAddr = (uint8_t *)srcAddr;
    config->nextDesc = (uint8_t *)nextDesc;
    config->xfercfg.transferCount = xfer_count;
    config->xfercfg.byteWidth = byteWidth;
    config->xfercfg.intA = true;
    config->xfercfg.reload = nextDesc != NULL;
    config->xfercfg.valid = true;
}

/*!
 * brief set channel config.
 *
 * This function provide a interface to configure channel configuration reisters.
 *
 * param base DMA base address.
 * param channel DMA channel number.
 * param config channel configurations structure.
 */
void DMA_SetChannelConfig(DMA_Type *base, uint32_t channel, dma_channel_trigger_t *trigger, bool isPeriph)
{
    assert(channel <= FSL_FEATURE_DMA_MAX_CHANNELS);

    uint32_t tmp = DMA_CHANNEL_CFG_PERIPHREQEN_MASK;

    if (trigger != NULL)
    {
        tmp |= DMA_CHANNEL_CFG_HWTRIGEN_MASK | DMA_CHANNEL_CFG_TRIGPOL_MASK | DMA_CHANNEL_CFG_TRIGTYPE_MASK |
               DMA_CHANNEL_CFG_TRIGBURST_MASK | DMA_CHANNEL_CFG_BURSTPOWER_MASK | DMA_CHANNEL_CFG_SRCBURSTWRAP_MASK |
               DMA_CHANNEL_CFG_DSTBURSTWRAP_MASK;
    }

    tmp = base->CHANNEL[channel].CFG & (~tmp);

    if (trigger != NULL)
    {
        tmp |= (uint32_t)(trigger->type) | (uint32_t)(trigger->burst) | (uint32_t)(trigger->wrap);
    }

    tmp |= DMA_CHANNEL_CFG_PERIPHREQEN(isPeriph);

    base->CHANNEL[channel].CFG = tmp;
}

/*!
 * brief Prepare channel transfer configurations.
 *
 * This function used to prepare channel transfer configurations.
 *
 * param config Pointer to DMA channel transfer configuration structure.
 * param srcStartAddr source start address.
 * param dstStartAddr destination start address.
 * param xferCfg xfer configuration, user can reference DMA_CHANNEL_XFER about to how to get xferCfg value.
 * param type transfer type.
 * param trigger DMA channel trigger configurations.
 * param nextDesc address of next descriptor.
 */
void DMA_PrepareChannelTransfer(dma_channel_config_t *config,
                                void *srcStartAddr,
                                void *dstStartAddr,
                                uint32_t xferCfg,
                                dma_transfer_type_t type,
                                dma_channel_trigger_t *trigger,
                                void *nextDesc)
{
    assert((NULL != config) && (NULL != srcStartAddr) && (NULL != dstStartAddr));
    assert(((uint32_t)nextDesc & (DMA_LINK_DESCRIPTOR_ADDRESS_ALIGN - 1)) == 0U);

    /* check max */
    memset(config, 0, sizeof(*config));

    switch (type)
    {
        case kDMA_MemoryToMemory:
            config->isPeriph = false;
            break;
        case kDMA_PeripheralToMemory:
            config->isPeriph = true;
            break;
        case kDMA_MemoryToPeripheral:
            config->isPeriph = true;
            break;
        case kDMA_StaticToStatic:
            config->isPeriph = true;
            break;
        default:
            return;
    }

    config->dstStartAddr = (uint8_t *)dstStartAddr;
    config->srcStartAddr = (uint8_t *)srcStartAddr;
    config->nextDesc = (uint8_t *)nextDesc;
    config->trigger = trigger;
    config->xferCfg = xferCfg;
}

/*!
 * brief Install DMA descriptor memory.
 *
 * This function used to register DMA descriptor memory for linked transfer, a typical case is ping pong
 * transfer which will request more than one DMA descriptor memory space, althrough current DMA driver has
 * a default DMA descriptor buffer, but it support one DMA descriptor for one channel only.
 * User should be take care about the address of DMA descriptor pool which required align with 512BYTE.
 *
 * param handle Pointer to DMA channel transfer handle.
 * param addr DMA descriptor address
 * param num DMA descriptor number.
 */
void DMA_InstallDescriptorMemory(DMA_Type *base, void *addr)
{
    assert(addr != NULL);
    assert(((uint32_t)addr & (FSL_FEATURE_DMA_DESCRIPTOR_ALIGN_SIZE - 1U)) == 0U);

    /* reconfigure the DMA descriptor base address */
    base->SRAMBASE = (uint32_t)addr;
}

/*!
 * brief Submits the DMA channel transfer request.
 *
 * This function submits the DMA transfer request according to the transfer configuration structure.
 * If the user submits the transfer request repeatedly, this function packs an unprocessed request as
 * a TCD and enables scatter/gather feature to process it in the next time.
 *
 * param handle DMA handle pointer.
 * param config Pointer to DMA transfer configuration structure.
 * retval kStatus_DMA_Success It means submit transfer request succeed.
 * retval kStatus_DMA_QueueFull It means TCD queue is full. Submit transfer request is not allowed.
 * retval kStatus_DMA_Busy It means the given channel is busy, need to submit request later.
 */
status_t DMA_SubmitChannelTransfer(dma_handle_t *handle, dma_channel_config_t *config)
{
    assert((NULL != handle) && (NULL != config));
    assert(handle->channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(handle->base));

    uint32_t instance = DMA_GetInstance(handle->base);
    dma_descriptor_t *descriptor = (dma_descriptor_t *)(&s_dma_descriptor_table[instance][handle->channel]);

    /* Previous transfer has not finished */
    if (DMA_ChannelIsActive(handle->base, handle->channel))
    {
        return kStatus_DMA_Busy;
    }

    DMA_SetupDescriptor(descriptor, config->xferCfg, config->srcStartAddr, config->dstStartAddr, config->nextDesc);
    DMA_SetChannelConfig(handle->base, handle->channel, config->trigger, config->isPeriph);
    /* Set channel XFERCFG register according first channel descriptor. */
    handle->base->CHANNEL[handle->channel].XFERCFG = descriptor->xfercfg;

    return kStatus_Success;
}

/*!
 * brief Submits the DMA transfer request.
 * deprecated Do not use this function.  It has been superceded by @ref DMA_SubmitChannelTransfer.
 *
 * This function submits the DMA transfer request according to the transfer configuration structure.
 * If the user submits the transfer request repeatedly, this function packs an unprocessed request as
 * a TCD and enables scatter/gather feature to process it in the next time.
 *
 * param handle DMA handle pointer.
 * param config Pointer to DMA transfer configuration structure.
 * retval kStatus_DMA_Success It means submit transfer request succeed.
 * retval kStatus_DMA_QueueFull It means TCD queue is full. Submit transfer request is not allowed.
 * retval kStatus_DMA_Busy It means the given channel is busy, need to submit request later.
 */
status_t DMA_SubmitTransfer(dma_handle_t *handle, dma_transfer_config_t *config)
{
    assert((NULL != handle) && (NULL != config));
    assert(handle->channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(handle->base));

    uint32_t instance = DMA_GetInstance(handle->base);
    dma_descriptor_t *descriptor = (dma_descriptor_t *)(&s_dma_descriptor_table[instance][handle->channel]);

    /* Previous transfer has not finished */
    if (DMA_ChannelIsActive(handle->base, handle->channel))
    {
        return kStatus_DMA_Busy;
    }

    /* enable/disable peripheral request */
    if (config->isPeriph)
    {
        DMA_EnableChannelPeriphRq(handle->base, handle->channel);
    }
    else
    {
        DMA_DisableChannelPeriphRq(handle->base, handle->channel);
    }

    DMA_CreateDescriptor(descriptor, &config->xfercfg, config->srcAddr, config->dstAddr, config->nextDesc);
    /* Set channel XFERCFG register according first channel descriptor. */
    handle->base->CHANNEL[handle->channel].XFERCFG = descriptor->xfercfg;

    return kStatus_Success;
}

/*!
 * brief DMA start transfer.
 *
 * This function enables the channel request. User can call this function after submitting the transfer request
 * or before submitting the transfer request.
 *
 * param handle DMA handle pointer.
 */
void DMA_StartTransfer(dma_handle_t *handle)
{
    assert(NULL != handle);

    uint32_t channel = handle->channel;
    assert(channel < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(handle->base));

    /* enable channel */
    DMA_EnableChannel(handle->base, channel);

    /* user software trigger if Peripheral request not enabled */
    if (((handle->base->CHANNEL[handle->channel].CFG & DMA_CHANNEL_CFG_TRIGBURST_MASK) != 0U) ||
        ((handle->base->CHANNEL[handle->channel].CFG & DMA_CHANNEL_CFG_TRIGTYPE_MASK) != DMA_CHANNEL_CFG_TRIGTYPE(1U)))
    {
        handle->base->CHANNEL[channel].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
    }
}

void DMA_IRQHandle(DMA_Type *base)
{
    dma_handle_t *handle;
    int32_t channel_index;
    uint32_t startChannel = DMA_GetVirtualStartChannel(base);
    uint32_t i = 0;

    /* Find channels that have completed transfer */
    for (i = 0; i < FSL_FEATURE_DMA_NUMBER_OF_CHANNELSn(base); i++)
    {
        handle = s_DMAHandle[i + startChannel];
        /* Handle is not present */
        if (NULL == handle)
        {
            continue;
        }
        channel_index = DMA_CHANNEL_INDEX(handle->channel);
        /* Channel uses INTA flag */
        if (DMA_COMMON_REG_GET(handle->base, handle->channel, INTA) & (1U << channel_index))
        {
            /* Clear INTA flag */
            DMA_COMMON_REG_SET(handle->base, handle->channel, INTA, (1U << channel_index));
            if (handle->callback)
            {
                (handle->callback)(handle, handle->userData, true, kDMA_IntA);
            }
        }
        /* Channel uses INTB flag */
        if (DMA_COMMON_REG_GET(handle->base, handle->channel, INTB) & (1U << channel_index))
        {
            /* Clear INTB flag */
            DMA_COMMON_REG_SET(handle->base, handle->channel, INTB, (1U << channel_index));
            if (handle->callback)
            {
                (handle->callback)(handle, handle->userData, true, kDMA_IntB);
            }
        }
        /* Error flag */
        if (DMA_COMMON_REG_GET(handle->base, handle->channel, ERRINT) & (1U << channel_index))
        {
            /* Clear error flag */
            DMA_COMMON_REG_SET(handle->base, handle->channel, ERRINT, (1U << channel_index));
            if (handle->callback)
            {
                (handle->callback)(handle, handle->userData, false, kDMA_IntError);
            }
        }
    }
}

void DMA0_DriverIRQHandler(void)
{
    DMA_IRQHandle(DMA0);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

#if defined(DMA1)
void DMA1_DriverIRQHandler(void)
{
    DMA_IRQHandle(DMA1);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
