/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.edma"
#endif

#define EDMA_TRANSFER_ENABLED_MASK 0x80U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance offset.
 *
 * @param instance EDMA peripheral instance number.
 */
static uint32_t EDMA_GetInstanceOffset(uint32_t instance);

/*!
 * @brief Map transfer width.
 *
 * @param width transfer width.
 */
static edma_transfer_size_t EDMA_TransferWidthMapping(uint32_t width);
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

/*!
 * brief Push content of TCD structure into hardware TCD register.
 *
 * param base EDMA peripheral base address.
 * param channel EDMA channel number.
 * param tcd Point to TCD structure.
 */
void EDMA_InstallTCD(DMA_Type *base, uint32_t channel, edma_tcd_t *tcd)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);

    /* Push tcd into hardware TCD register */
    base->TCD[channel].SADDR         = tcd->SADDR;
    base->TCD[channel].SOFF          = tcd->SOFF;
    base->TCD[channel].ATTR          = tcd->ATTR;
    base->TCD[channel].NBYTES_MLNO   = tcd->NBYTES;
    base->TCD[channel].SLAST         = tcd->SLAST;
    base->TCD[channel].DADDR         = tcd->DADDR;
    base->TCD[channel].DOFF          = tcd->DOFF;
    base->TCD[channel].CITER_ELINKNO = tcd->CITER;
    base->TCD[channel].DLAST_SGA     = tcd->DLAST_SGA;
    /* Clear DONE bit first, otherwise ESG cannot be set */
    base->TCD[channel].CSR           = 0;
    base->TCD[channel].CSR           = tcd->CSR;
    base->TCD[channel].BITER_ELINKNO = tcd->BITER;
}

/*!
 * brief Initializes the eDMA peripheral.
 *
 * This function ungates the eDMA clock and configures the eDMA peripheral according
 * to the configuration structure.
 *
 * param base eDMA peripheral base address.
 * param config A pointer to the configuration structure, see "edma_config_t".
 * note This function enables the minor loop map feature.
 */
void EDMA_Init(DMA_Type *base, const edma_config_t *config)
{
    assert(config != NULL);

    uint32_t tmpreg;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate EDMA peripheral clock */
    CLOCK_EnableClock(s_edmaClockName[EDMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* clear all the enabled request, status to make sure EDMA status is in normal condition */
    base->ERQ = 0U;
    base->INT = 0xFFFFFFFFU;
    base->ERR = 0xFFFFFFFFU;
    /* Configure EDMA peripheral according to the configuration structure. */
    tmpreg = base->CR;
    tmpreg &= ~(DMA_CR_ERCA_MASK | DMA_CR_HOE_MASK | DMA_CR_CLM_MASK | DMA_CR_EDBG_MASK);
    tmpreg |= (DMA_CR_ERCA(config->enableRoundRobinArbitration) | DMA_CR_HOE(config->enableHaltOnError) |
               DMA_CR_CLM(config->enableContinuousLinkMode) | DMA_CR_EDBG(config->enableDebugMode) | DMA_CR_EMLM(1U));
    base->CR = tmpreg;
}

/*!
 * brief Deinitializes the eDMA peripheral.
 *
 * This function gates the eDMA clock.
 *
 * param base eDMA peripheral base address.
 */
void EDMA_Deinit(DMA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate EDMA peripheral clock */
    CLOCK_DisableClock(s_edmaClockName[EDMA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Gets the eDMA default configuration structure.
 *
 * This function sets the configuration structure to default values.
 * The default configuration is set to the following values.
 * code
 *   config.enableContinuousLinkMode = false;
 *   config.enableHaltOnError = true;
 *   config.enableRoundRobinArbitration = false;
 *   config.enableDebugMode = false;
 * endcode
 *
 * param config A pointer to the eDMA configuration structure.
 */
void EDMA_GetDefaultConfig(edma_config_t *config)
{
    assert(config != NULL);

    /* Initializes the configure structure to zero. */
    (void)memset(config, 0, sizeof(*config));

    config->enableRoundRobinArbitration = false;
    config->enableHaltOnError           = true;
    config->enableContinuousLinkMode    = false;
    config->enableDebugMode             = false;
}

/*!
 * brief Sets all TCD registers to default values.
 *
 * This function sets TCD registers for this channel to default values.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * note This function must not be called while the channel transfer is ongoing
 *       or it causes unpredictable results.
 * note This function enables the auto stop request feature.
 */
void EDMA_ResetChannel(DMA_Type *base, uint32_t channel)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    EDMA_TcdReset((edma_tcd_t *)(uint32_t)&base->TCD[channel]);
}

/*!
 * brief Configures the eDMA transfer attribute.
 *
 * This function configures the transfer attribute, including source address, destination address,
 * transfer size, address offset, and so on. It also configures the scatter gather feature if the
 * user supplies the TCD address.
 * Example:
 * code
 *  edma_transfer_t config;
 *  edma_tcd_t tcd;
 *  config.srcAddr = ..;
 *  config.destAddr = ..;
 *  ...
 *  EDMA_SetTransferConfig(DMA0, channel, &config, &stcd);
 * endcode
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param config Pointer to eDMA transfer configuration structure.
 * param nextTcd Point to TCD structure. It can be NULL if users
 *                do not want to enable scatter/gather feature.
 * note If nextTcd is not NULL, it means scatter gather feature is enabled
 *       and DREQ bit is cleared in the previous transfer configuration, which
 *       is set in the eDMA_ResetChannel.
 */
void EDMA_SetTransferConfig(DMA_Type *base, uint32_t channel, const edma_transfer_config_t *config, edma_tcd_t *nextTcd)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);
    assert(((uint32_t)nextTcd & 0x1FU) == 0U);

    EDMA_TcdSetTransferConfig((edma_tcd_t *)(uint32_t)&base->TCD[channel], config, nextTcd);
}

/*!
 * brief Configures the eDMA minor offset feature.
 *
 * The minor offset means that the signed-extended value is added to the source address or destination
 * address after each minor loop.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param config A pointer to the minor offset configuration structure.
 */
void EDMA_SetMinorOffsetConfig(DMA_Type *base, uint32_t channel, const edma_minor_offset_config_t *config)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);

    uint32_t tmpreg;

    tmpreg = base->TCD[channel].NBYTES_MLOFFYES;
    tmpreg &= ~(DMA_NBYTES_MLOFFYES_SMLOE_MASK | DMA_NBYTES_MLOFFYES_DMLOE_MASK | DMA_NBYTES_MLOFFYES_MLOFF_MASK);
    tmpreg |=
        (DMA_NBYTES_MLOFFYES_SMLOE(config->enableSrcMinorOffset) |
         DMA_NBYTES_MLOFFYES_DMLOE(config->enableDestMinorOffset) | DMA_NBYTES_MLOFFYES_MLOFF(config->minorOffset));
    base->TCD[channel].NBYTES_MLOFFYES = tmpreg;
}

/*!
 * brief Configures the eDMA channel TCD major offset feature.
 *
 * Adjustment value added to the source address at the completion of the major iteration count
 *
 * param base eDMA peripheral base address.
 * param channel edma channel number.
 * param sourceOffset source address offset.
 * param destOffset destination address offset.
 */
void EDMA_SetMajorOffsetConfig(DMA_Type *base, uint32_t channel, int32_t sourceOffset, int32_t destOffset)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    base->TCD[channel].SLAST     = (uint32_t)sourceOffset;
    base->TCD[channel].DLAST_SGA = (uint32_t)destOffset;
}

/*!
 * brief Configures the eDMA channel preemption feature.
 *
 * This function configures the channel preemption attribute and the priority of the channel.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number
 * param config A pointer to the channel preemption configuration structure.
 */
void EDMA_SetChannelPreemptionConfig(DMA_Type *base, uint32_t channel, const edma_channel_Preemption_config_t *config)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(config != NULL);

    bool tmpEnablePreemptAbility    = config->enablePreemptAbility;
    bool tmpEnableChannelPreemption = config->enableChannelPreemption;
    uint8_t tmpChannelPriority      = config->channelPriority;
    volatile uint8_t *tmpReg        = &base->DCHPRI3;

    ((volatile uint8_t *)tmpReg)[DMA_DCHPRI_INDEX(channel)] =
        (DMA_DCHPRI0_DPA((true == tmpEnablePreemptAbility ? 0U : 1U)) |
         DMA_DCHPRI0_ECP((true == tmpEnableChannelPreemption ? 1U : 0U)) | DMA_DCHPRI0_CHPRI(tmpChannelPriority));
}

/*!
 * brief Sets the channel link for the eDMA transfer.
 *
 * This function configures either the minor link or the major link mode. The minor link means that the channel link is
 * triggered every time CITER decreases by 1. The major link means that the channel link is triggered when the CITER is
 * exhausted.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param type A channel link type, which can be one of the following:
 *   arg kEDMA_LinkNone
 *   arg kEDMA_MinorLink
 *   arg kEDMA_MajorLink
 * param linkedChannel The linked channel number.
 * note Users should ensure that DONE flag is cleared before calling this interface, or the configuration is invalid.
 */
void EDMA_SetChannelLink(DMA_Type *base, uint32_t channel, edma_channel_link_type_t type, uint32_t linkedChannel)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);
    assert(linkedChannel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    EDMA_TcdSetChannelLink((edma_tcd_t *)(uint32_t)&base->TCD[channel], type, linkedChannel);
}

/*!
 * brief Sets the bandwidth for the eDMA transfer.
 *
 * Because the eDMA processes the minor loop, it continuously generates read/write sequences
 * until the minor count is exhausted. The bandwidth forces the eDMA to stall after the completion of
 * each read/write access to control the bus request bandwidth seen by the crossbar switch.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param bandWidth A bandwidth setting, which can be one of the following:
 *     arg kEDMABandwidthStallNone
 *     arg kEDMABandwidthStall4Cycle
 *     arg kEDMABandwidthStall8Cycle
 */
void EDMA_SetBandWidth(DMA_Type *base, uint32_t channel, edma_bandwidth_t bandWidth)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    base->TCD[channel].CSR = (uint16_t)((base->TCD[channel].CSR & (~DMA_CSR_BWC_MASK)) | DMA_CSR_BWC(bandWidth));
}

/*!
 * brief Sets the source modulo and the destination modulo for the eDMA transfer.
 *
 * This function defines a specific address range specified to be the value after (SADDR + SOFF)/(DADDR + DOFF)
 * calculation is performed or the original register value. It provides the ability to implement a circular data
 * queue easily.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param srcModulo A source modulo value.
 * param destModulo A destination modulo value.
 */
void EDMA_SetModulo(DMA_Type *base, uint32_t channel, edma_modulo_t srcModulo, edma_modulo_t destModulo)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint16_t tmpreg;

    tmpreg                  = base->TCD[channel].ATTR & (~(uint16_t)(DMA_ATTR_SMOD_MASK | DMA_ATTR_DMOD_MASK));
    base->TCD[channel].ATTR = tmpreg | DMA_ATTR_DMOD(destModulo) | DMA_ATTR_SMOD(srcModulo);
}

/*!
 * brief Enables the interrupt source for the eDMA transfer.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param mask The mask of interrupt source to be set. Users need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_EnableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Enable error interrupt */
    if (0U != (mask & (uint32_t)kEDMA_ErrorInterruptEnable))
    {
        base->EEI |= ((uint32_t)0x1U << channel);
    }

    /* Enable Major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_MajorInterruptEnable))
    {
        base->TCD[channel].CSR |= DMA_CSR_INTMAJOR_MASK;
    }

    /* Enable Half major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_HalfInterruptEnable))
    {
        base->TCD[channel].CSR |= DMA_CSR_INTHALF_MASK;
    }
}

/*!
 * brief Disables the interrupt source for the eDMA transfer.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param mask The mask of the interrupt source to be set. Use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_DisableChannelInterrupts(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Disable error interrupt */
    if (0U != (mask & (uint32_t)kEDMA_ErrorInterruptEnable))
    {
        base->EEI &= (~((uint32_t)0x1U << channel));
    }

    /* Disable Major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_MajorInterruptEnable))
    {
        base->TCD[channel].CSR &= ~(uint16_t)DMA_CSR_INTMAJOR_MASK;
    }

    /* Disable Half major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_HalfInterruptEnable))
    {
        base->TCD[channel].CSR &= ~(uint16_t)DMA_CSR_INTHALF_MASK;
    }
}

/*!
 * brief Sets all fields to default values for the TCD structure.
 *
 * This function sets all fields for this TCD structure to default value.
 *
 * param tcd Pointer to the TCD structure.
 * note This function enables the auto stop request feature.
 */
void EDMA_TcdReset(edma_tcd_t *tcd)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);

    /* Reset channel TCD */
    tcd->SADDR     = 0U;
    tcd->SOFF      = 0U;
    tcd->ATTR      = 0U;
    tcd->NBYTES    = 0U;
    tcd->SLAST     = 0U;
    tcd->DADDR     = 0U;
    tcd->DOFF      = 0U;
    tcd->CITER     = 0U;
    tcd->DLAST_SGA = 0U;
    /* Enable auto disable request feature */
    tcd->CSR   = DMA_CSR_DREQ(true);
    tcd->BITER = 0U;
}

/*!
 * brief Configures the eDMA TCD transfer attribute.
 *
 * The TCD is a transfer control descriptor. The content of the TCD is the same as the hardware TCD registers.
 * The STCD is used in the scatter-gather mode.
 * This function configures the TCD transfer attribute, including source address, destination address,
 * transfer size, address offset, and so on. It also configures the scatter gather feature if the
 * user supplies the next TCD address.
 * Example:
 * code
 *   edma_transfer_t config = {
 *   ...
 *   }
 *   edma_tcd_t tcd __aligned(32);
 *   edma_tcd_t nextTcd __aligned(32);
 *   EDMA_TcdSetTransferConfig(&tcd, &config, &nextTcd);
 * endcode
 *
 * param tcd Pointer to the TCD structure.
 * param config Pointer to eDMA transfer configuration structure.
 * param nextTcd Pointer to the next TCD structure. It can be NULL if users
 *                do not want to enable scatter/gather feature.
 * note TCD address should be 32 bytes aligned or it causes an eDMA error.
 * note If the nextTcd is not NULL, the scatter gather feature is enabled
 *       and DREQ bit is cleared in the previous transfer configuration, which
 *       is set in the EDMA_TcdReset.
 */
void EDMA_TcdSetTransferConfig(edma_tcd_t *tcd, const edma_transfer_config_t *config, edma_tcd_t *nextTcd)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);
    assert(config != NULL);
    assert(((uint32_t)nextTcd & 0x1FU) == 0U);
    assert((config->srcAddr % (1UL << (uint32_t)config->srcTransferSize)) == 0U);
    assert((config->destAddr % (1UL << (uint32_t)config->destTransferSize)) == 0U);

    /* source address */
    tcd->SADDR = config->srcAddr;
    /* destination address */
    tcd->DADDR = config->destAddr;
    /* Source data and destination data transfer size */
    tcd->ATTR = DMA_ATTR_SSIZE(config->srcTransferSize) | DMA_ATTR_DSIZE(config->destTransferSize);
    /* Source address signed offset */
    tcd->SOFF = (uint16_t)config->srcOffset;
    /* Destination address signed offset */
    tcd->DOFF = (uint16_t)config->destOffset;
    /* Minor byte transfer count */
    tcd->NBYTES = config->minorLoopBytes;
    /* Current major iteration count */
    tcd->CITER = (uint16_t)config->majorLoopCounts;
    /* Starting major iteration count */
    tcd->BITER = (uint16_t)config->majorLoopCounts;
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
        tcd->CSR = (tcd->CSR | (uint16_t)DMA_CSR_ESG_MASK) & ~(uint16_t)DMA_CSR_DREQ_MASK;
    }
}

/*!
 * brief Configures the eDMA TCD minor offset feature.
 *
 * A minor offset is a signed-extended value added to the source address or a destination
 * address after each minor loop.
 *
 * param tcd A point to the TCD structure.
 * param config A pointer to the minor offset configuration structure.
 */
void EDMA_TcdSetMinorOffsetConfig(edma_tcd_t *tcd, const edma_minor_offset_config_t *config)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);

    uint32_t tmpreg;

    tmpreg = tcd->NBYTES &
             ~(DMA_NBYTES_MLOFFYES_SMLOE_MASK | DMA_NBYTES_MLOFFYES_DMLOE_MASK | DMA_NBYTES_MLOFFYES_MLOFF_MASK);
    tmpreg |=
        (DMA_NBYTES_MLOFFYES_SMLOE(config->enableSrcMinorOffset) |
         DMA_NBYTES_MLOFFYES_DMLOE(config->enableDestMinorOffset) | DMA_NBYTES_MLOFFYES_MLOFF(config->minorOffset));
    tcd->NBYTES = tmpreg;
}

/*!
 * brief Configures the eDMA TCD major offset feature.
 *
 * Adjustment value added to the source address at the completion of the major iteration count
 *
 * param tcd A point to the TCD structure.
 * param sourceOffset source address offset.
 * param destOffset destination address offset.
 */
void EDMA_TcdSetMajorOffsetConfig(edma_tcd_t *tcd, int32_t sourceOffset, int32_t destOffset)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);

    tcd->SLAST     = (uint32_t)sourceOffset;
    tcd->DLAST_SGA = (uint32_t)destOffset;
}

/*!
 * brief Sets the channel link for the eDMA TCD.
 *
 * This function configures either a minor link or a major link. The minor link means the channel link is
 * triggered every time CITER decreases by 1. The major link means that the channel link  is triggered when the CITER is
 * exhausted.
 *
 * note Users should ensure that DONE flag is cleared before calling this interface, or the configuration is invalid.
 * param tcd Point to the TCD structure.
 * param type Channel link type, it can be one of:
 *   arg kEDMA_LinkNone
 *   arg kEDMA_MinorLink
 *   arg kEDMA_MajorLink
 * param linkedChannel The linked channel number.
 */
void EDMA_TcdSetChannelLink(edma_tcd_t *tcd, edma_channel_link_type_t type, uint32_t linkedChannel)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);
    assert(linkedChannel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    if (type == kEDMA_MinorLink) /* Minor link config */
    {
        uint16_t tmpreg;

        /* Enable minor link */
        tcd->CITER |= DMA_CITER_ELINKYES_ELINK_MASK;
        tcd->BITER |= DMA_BITER_ELINKYES_ELINK_MASK;
        /* Set linked channel */
        tmpreg = tcd->CITER & (~(uint16_t)DMA_CITER_ELINKYES_LINKCH_MASK);
        tmpreg |= DMA_CITER_ELINKYES_LINKCH(linkedChannel);
        tcd->CITER = tmpreg;
        tmpreg     = tcd->BITER & (~(uint16_t)DMA_BITER_ELINKYES_LINKCH_MASK);
        tmpreg |= DMA_BITER_ELINKYES_LINKCH(linkedChannel);
        tcd->BITER = tmpreg;
    }
    else if (type == kEDMA_MajorLink) /* Major link config */
    {
        uint16_t tmpreg;

        /* Enable major link */
        tcd->CSR |= DMA_CSR_MAJORELINK_MASK;
        /* Set major linked channel */
        tmpreg   = tcd->CSR & (~(uint16_t)DMA_CSR_MAJORLINKCH_MASK);
        tcd->CSR = tmpreg | DMA_CSR_MAJORLINKCH(linkedChannel);
    }
    else /* Link none */
    {
        tcd->CITER &= ~(uint16_t)DMA_CITER_ELINKYES_ELINK_MASK;
        tcd->BITER &= ~(uint16_t)DMA_BITER_ELINKYES_ELINK_MASK;
        tcd->CSR &= ~(uint16_t)DMA_CSR_MAJORELINK_MASK;
    }
}

/*!
 * brief Sets the source modulo and the destination modulo for the eDMA TCD.
 *
 * This function defines a specific address range specified to be the value after (SADDR + SOFF)/(DADDR + DOFF)
 * calculation is performed or the original register value. It provides the ability to implement a circular data
 * queue easily.
 *
 * param tcd A pointer to the TCD structure.
 * param srcModulo A source modulo value.
 * param destModulo A destination modulo value.
 */
void EDMA_TcdSetModulo(edma_tcd_t *tcd, edma_modulo_t srcModulo, edma_modulo_t destModulo)
{
    assert(tcd != NULL);
    assert(((uint32_t)tcd & 0x1FU) == 0U);

    uint16_t tmpreg;

    tmpreg    = tcd->ATTR & (~(uint16_t)(DMA_ATTR_SMOD_MASK | DMA_ATTR_DMOD_MASK));
    tcd->ATTR = tmpreg | DMA_ATTR_DMOD(destModulo) | DMA_ATTR_SMOD(srcModulo);
}

/*!
 * brief Enables the interrupt source for the eDMA TCD.
 *
 * param tcd Point to the TCD structure.
 * param mask The mask of interrupt source to be set. Users need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_TcdEnableInterrupts(edma_tcd_t *tcd, uint32_t mask)
{
    assert(tcd != NULL);

    /* Enable Major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_MajorInterruptEnable))
    {
        tcd->CSR |= DMA_CSR_INTMAJOR_MASK;
    }

    /* Enable Half major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_HalfInterruptEnable))
    {
        tcd->CSR |= DMA_CSR_INTHALF_MASK;
    }
}

/*!
 * brief Disables the interrupt source for the eDMA TCD.
 *
 * param tcd Point to the TCD structure.
 * param mask The mask of interrupt source to be set. Users need to use
 *             the defined edma_interrupt_enable_t type.
 */
void EDMA_TcdDisableInterrupts(edma_tcd_t *tcd, uint32_t mask)
{
    assert(tcd != NULL);

    /* Disable Major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_MajorInterruptEnable))
    {
        tcd->CSR &= ~(uint16_t)DMA_CSR_INTMAJOR_MASK;
    }

    /* Disable Half major interrupt */
    if (0U != (mask & (uint32_t)kEDMA_HalfInterruptEnable))
    {
        tcd->CSR &= ~(uint16_t)DMA_CSR_INTHALF_MASK;
    }
}

/*!
 * brief Gets the remaining major loop count from the eDMA current channel TCD.
 *
 * This function checks the TCD (Task Control Descriptor) status for a specified
 * eDMA channel and returns the number of major loop count that has not finished.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * return Major loop count which has not been transferred yet for the current TCD.
 * note 1. This function can only be used to get unfinished major loop count of transfer without
 *          the next TCD, or it might be inaccuracy.
 *       2. The unfinished/remaining transfer bytes cannot be obtained directly from registers while
 *          the channel is running.
 *          Because to calculate the remaining bytes, the initial NBYTES configured in DMA_TCDn_NBYTES_MLNO
 *          register is needed while the eDMA IP does not support getting it while a channel is active.
 *          In another word, the NBYTES value reading is always the actual (decrementing) NBYTES value the dma_engine
 *          is working with while a channel is running.
 *          Consequently, to get the remaining transfer bytes, a software-saved initial value of NBYTES (for example
 *          copied before enabling the channel) is needed. The formula to calculate it is shown below:
 *          RemainingBytes = RemainingMajorLoopCount * NBYTES(initially configured)
 */
uint32_t EDMA_GetRemainingMajorLoopCount(DMA_Type *base, uint32_t channel)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t remainingCount = 0;

    if (0U != (DMA_CSR_DONE_MASK & base->TCD[channel].CSR))
    {
        remainingCount = 0;
    }
    else
    {
        /* Calculate the unfinished bytes */
        if (0U != (base->TCD[channel].CITER_ELINKNO & DMA_CITER_ELINKNO_ELINK_MASK))
        {
            remainingCount = (((uint32_t)base->TCD[channel].CITER_ELINKYES & DMA_CITER_ELINKYES_CITER_MASK) >>
                              DMA_CITER_ELINKYES_CITER_SHIFT);
        }
        else
        {
            remainingCount = (((uint32_t)base->TCD[channel].CITER_ELINKNO & DMA_CITER_ELINKNO_CITER_MASK) >>
                              DMA_CITER_ELINKNO_CITER_SHIFT);
        }
    }

    return remainingCount;
}

/*!
 * brief Gets the eDMA channel status flags.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * return The mask of channel status flags. Users need to use the
 *         _edma_channel_status_flags type to decode the return variables.
 */
uint32_t EDMA_GetChannelStatusFlags(DMA_Type *base, uint32_t channel)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t retval = 0;

    /* Get DONE bit flag */
    retval |= (((uint32_t)base->TCD[channel].CSR & DMA_CSR_DONE_MASK) >> DMA_CSR_DONE_SHIFT);
    /* Get ERROR bit flag */
    retval |= ((((uint32_t)base->ERR >> channel) & 0x1U) << 1U);
    /* Get INT bit flag */
    retval |= ((((uint32_t)base->INT >> channel) & 0x1U) << 2U);

    return retval;
}

/*!
 * brief Clears the eDMA channel status flags.
 *
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 * param mask The mask of channel status to be cleared. Users need to use
 *             the defined _edma_channel_status_flags type.
 */
void EDMA_ClearChannelStatusFlags(DMA_Type *base, uint32_t channel, uint32_t mask)
{
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    /* Clear DONE bit flag */
    if (0U != (mask & (uint32_t)kEDMA_DoneFlag))
    {
        base->CDNE = (uint8_t)channel;
    }
    /* Clear ERROR bit flag */
    if (0U != (mask & (uint32_t)kEDMA_ErrorFlag))
    {
        base->CERR = (uint8_t)channel;
    }
    /* Clear INT bit flag */
    if (0U != (mask & (uint32_t)kEDMA_InterruptFlag))
    {
        base->CINT = (uint8_t)channel;
    }
}

static uint32_t EDMA_GetInstanceOffset(uint32_t instance)
{
    static uint8_t startInstanceNum;

#if defined(DMA0)
    startInstanceNum = (uint8_t)EDMA_GetInstance(DMA0);
#elif defined(DMA1)
    startInstanceNum = (uint8_t)EDMA_GetInstance(DMA1);
#elif defined(DMA2)
    startInstanceNum = (uint8_t)EDMA_GetInstance(DMA2);
#elif defined(DMA3)
    startInstanceNum = (uint8_t)EDMA_GetInstance(DMA3);
#endif

    assert(startInstanceNum <= instance);

    return instance - startInstanceNum;
}

/*!
 * brief Creates the eDMA handle.
 *
 * This function is called if using the transactional API for eDMA. This function
 * initializes the internal state of the eDMA handle.
 *
 * param handle eDMA handle pointer. The eDMA handle stores callback function and
 *               parameters.
 * param base eDMA peripheral base address.
 * param channel eDMA channel number.
 */
void EDMA_CreateHandle(edma_handle_t *handle, DMA_Type *base, uint32_t channel)
{
    assert(handle != NULL);
    assert(channel < (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL);

    uint32_t edmaInstance;
    uint32_t channelIndex;
    edma_tcd_t *tcdRegs;

    /* Zero the handle */
    (void)memset(handle, 0, sizeof(*handle));

    handle->base    = base;
    handle->channel = (uint8_t)channel;
    /* Get the DMA instance number */
    edmaInstance = EDMA_GetInstance(base);
    channelIndex = (EDMA_GetInstanceOffset(edmaInstance) * (uint32_t)FSL_FEATURE_EDMA_MODULE_CHANNEL) + channel;
    s_EDMAHandle[channelIndex] = handle;

    /* Enable NVIC interrupt */
    (void)EnableIRQ(s_edmaIRQNumber[edmaInstance][channel]);

    /*
       Reset TCD registers to zero. Unlike the EDMA_TcdReset(DREQ will be set),
       CSR will be 0. Because in order to suit EDMA busy check mechanism in
       EDMA_SubmitTransfer, CSR must be set 0.
    */
    tcdRegs            = (edma_tcd_t *)(uint32_t)&handle->base->TCD[handle->channel];
    tcdRegs->SADDR     = 0;
    tcdRegs->SOFF      = 0;
    tcdRegs->ATTR      = 0;
    tcdRegs->NBYTES    = 0;
    tcdRegs->SLAST     = 0;
    tcdRegs->DADDR     = 0;
    tcdRegs->DOFF      = 0;
    tcdRegs->CITER     = 0;
    tcdRegs->DLAST_SGA = 0;
    tcdRegs->CSR       = 0;
    tcdRegs->BITER     = 0;
}

/*!
 * brief Installs the TCDs memory pool into the eDMA handle.
 *
 * This function is called after the EDMA_CreateHandle to use scatter/gather feature. This function shall only be used
 * while users need to use scatter gather mode. Scatter gather mode enables EDMA to load a new transfer control block
 * (tcd) in hardware, and automatically reconfigure that DMA channel for a new transfer.
 * Users need to prepare tcd memory and also configure tcds using interface EDMA_SubmitTransfer.
 *
 * param handle eDMA handle pointer.
 * param tcdPool A memory pool to store TCDs. It must be 32 bytes aligned.
 * param tcdSize The number of TCD slots.
 */
void EDMA_InstallTCDMemory(edma_handle_t *handle, edma_tcd_t *tcdPool, uint32_t tcdSize)
{
    assert(handle != NULL);
    assert(((uint32_t)tcdPool & 0x1FU) == 0U);

    /* Initialize tcd queue attribute. */
    handle->header  = 0;
    handle->tail    = 0;
    handle->tcdUsed = 0;
    handle->tcdSize = (int8_t)tcdSize;
    handle->flags   = 0;
    handle->tcdPool = tcdPool;
}

/*!
 * brief Installs a callback function for the eDMA transfer.
 *
 * This callback is called in the eDMA IRQ handler. Use the callback to do something after
 * the current major loop transfer completes. This function will be called every time one tcd finished transfer.
 *
 * param handle eDMA handle pointer.
 * param callback eDMA callback function pointer.
 * param userData A parameter for the callback function.
 */
void EDMA_SetCallback(edma_handle_t *handle, edma_callback callback, void *userData)
{
    assert(handle != NULL);

    handle->callback = callback;
    handle->userData = userData;
}

static edma_transfer_size_t EDMA_TransferWidthMapping(uint32_t width)
{
    edma_transfer_size_t transferSize = kEDMA_TransferSize1Bytes;

    /* map width to register value */
    switch (width)
    {
        /* width 8bit */
        case 1U:
            transferSize = kEDMA_TransferSize1Bytes;
            break;
        /* width 16bit */
        case 2U:
            transferSize = kEDMA_TransferSize2Bytes;
            break;
        /* width 32bit */
        case 4U:
            transferSize = kEDMA_TransferSize4Bytes;
            break;
#if (defined(FSL_FEATURE_EDMA_SUPPORT_8_BYTES_TRANSFER) && FSL_FEATURE_EDMA_SUPPORT_8_BYTES_TRANSFER)
        /* width 64bit */
        case 8U:
            transferSize = kEDMA_TransferSize8Bytes;
            break;
#endif
#if (defined(FSL_FEATURE_EDMA_SUPPORT_16_BYTES_TRANSFER) && FSL_FEATURE_EDMA_SUPPORT_16_BYTES_TRANSFER)
        /* width 128bit */
        case 16U:
            transferSize = kEDMA_TransferSize16Bytes;
            break;
#endif
        /* width 256bit */
        case 32U:
            transferSize = kEDMA_TransferSize32Bytes;
            break;
        default:
            /* All the cases have been listed above, the default clause should not be reached. */
            assert(false);
            break;
    }

    return transferSize;
}

/*!
 * brief Prepares the eDMA transfer structure configurations.
 *
 * This function prepares the transfer configuration structure according to the user input.
 *
 * param config The user configuration structure of type edma_transfer_t.
 * param srcAddr eDMA transfer source address.
 * param srcWidth eDMA transfer source address width(bytes).
 * param srcOffset source address offset.
 * param destAddr eDMA transfer destination address.
 * param destWidth eDMA transfer destination address width(bytes).
 * param destOffset destination address offset.
 * param bytesEachRequest eDMA transfer bytes per channel request.
 * param transferBytes eDMA transfer bytes to be transferred.
 * note The data address and the data width must be consistent. For example, if the SRC
 *       is 4 bytes, the source address must be 4 bytes aligned, or it results in
 *       source address error (SAE).
 */
void EDMA_PrepareTransferConfig(edma_transfer_config_t *config,
                                void *srcAddr,
                                uint32_t srcWidth,
                                int16_t srcOffset,
                                void *destAddr,
                                uint32_t destWidth,
                                int16_t destOffset,
                                uint32_t bytesEachRequest,
                                uint32_t transferBytes)
{
    assert(config != NULL);
    assert(srcAddr != NULL);
    assert(destAddr != NULL);
    assert((srcWidth != 0U) && (srcWidth <= 32U) && ((srcWidth & (srcWidth - 1U)) == 0U));
    assert((destWidth != 0U) && (destWidth <= 32U) && ((destWidth & (destWidth - 1U)) == 0U));
    assert((transferBytes % bytesEachRequest) == 0U);
    assert((((uint32_t)(uint32_t *)srcAddr) % srcWidth) == 0U);
    assert((((uint32_t)(uint32_t *)destAddr) % destWidth) == 0U);

    /* Initializes the configure structure to zero. */
    (void)memset(config, 0, sizeof(*config));

    config->destAddr         = (uint32_t)(uint32_t *)destAddr;
    config->srcAddr          = (uint32_t)(uint32_t *)srcAddr;
    config->minorLoopBytes   = bytesEachRequest;
    config->majorLoopCounts  = transferBytes / bytesEachRequest;
    config->srcTransferSize  = EDMA_TransferWidthMapping(srcWidth);
    config->destTransferSize = EDMA_TransferWidthMapping(destWidth);
    config->destOffset       = destOffset;
    config->srcOffset        = srcOffset;
}

/*!
 * brief Prepares the eDMA transfer structure.
 *
 * This function prepares the transfer configuration structure according to the user input.
 *
 * param config The user configuration structure of type edma_transfer_t.
 * param srcAddr eDMA transfer source address.
 * param srcWidth eDMA transfer source address width(bytes).
 * param destAddr eDMA transfer destination address.
 * param destWidth eDMA transfer destination address width(bytes).
 * param bytesEachRequest eDMA transfer bytes per channel request.
 * param transferBytes eDMA transfer bytes to be transferred.
 * param type eDMA transfer type.
 * note The data address and the data width must be consistent. For example, if the SRC
 *       is 4 bytes, the source address must be 4 bytes aligned, or it results in
 *       source address error (SAE).
 */
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

    int16_t srcOffset = 0, destOffset = 0;

    switch (type)
    {
        case kEDMA_MemoryToMemory:
            destOffset = (int16_t)destWidth;
            srcOffset  = (int16_t)srcWidth;
            break;
        case kEDMA_MemoryToPeripheral:
            destOffset = 0;
            srcOffset  = (int16_t)srcWidth;
            break;
        case kEDMA_PeripheralToMemory:
            destOffset = (int16_t)destWidth;
            srcOffset  = 0;
            break;
        case kEDMA_PeripheralToPeripheral:
            destOffset = 0;
            srcOffset  = 0;
            break;
        default:
            /* All the cases have been listed above, the default clause should not be reached. */
            assert(false);
            break;
    }

    EDMA_PrepareTransferConfig(config, srcAddr, srcWidth, srcOffset, destAddr, destWidth, destOffset, bytesEachRequest,
                               transferBytes);
}

/*!
 * brief Submits the eDMA transfer request.
 *
 * This function submits the eDMA transfer request according to the transfer configuration structure.
 * In scatter gather mode, call this function will add a configured tcd to the circular list of tcd pool.
 * The tcd pools is setup by call function EDMA_InstallTCDMemory before.
 *
 * param handle eDMA handle pointer.
 * param config Pointer to eDMA transfer configuration structure.
 * retval kStatus_EDMA_Success It means submit transfer request succeed.
 * retval kStatus_EDMA_QueueFull It means TCD queue is full. Submit transfer request is not allowed.
 * retval kStatus_EDMA_Busy It means the given channel is busy, need to submit request later.
 */
status_t EDMA_SubmitTransfer(edma_handle_t *handle, const edma_transfer_config_t *config)
{
    assert(handle != NULL);
    assert(config != NULL);

    edma_tcd_t *tcdRegs = (edma_tcd_t *)(uint32_t)&handle->base->TCD[handle->channel];

    if (handle->tcdPool == NULL)
    {
        /*
         *    Check if EDMA channel is busy:
         *    1. if channel active bit is set, it implies that minor loop is executing, then channel is busy
         *    2. if channel active bit is not set and BITER not equal to CITER, it implies that major loop is executing,
         * then channel is busy
         *
         *    There is one case can not be covered in below condition:
         *    When transfer request is submitted, but no request from peripheral, that is to say channel sevice doesn't
         *    begin, if application would like to submit another transfer , then the TCD will be overwritten, since the
         *    ACTIVE is 0 and BITER = CITER, for such case, it is a scatter gather(link TCD) case actually, so
         *    application should enabled TCD pool for dynamic scatter gather mode by calling EDMA_InstallTCDMemory.
         */
        if (((handle->base->TCD[handle->channel].CSR & DMA_CSR_ACTIVE_MASK) != 0U) ||
            (((handle->base->TCD[handle->channel].CITER_ELINKNO & DMA_CITER_ELINKNO_CITER_MASK) !=
              (handle->base->TCD[handle->channel].BITER_ELINKNO & DMA_BITER_ELINKNO_BITER_MASK))))
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
        uint16_t csr;
        int8_t currentTcd;
        int8_t previousTcd;
        int8_t nextTcd;
        int8_t tmpTcdUsed;
        int8_t tmpTcdSize;

        /* Check if tcd pool is full. */
        primask    = DisableGlobalIRQ();
        tmpTcdUsed = handle->tcdUsed;
        tmpTcdSize = handle->tcdSize;
        if (tmpTcdUsed >= tmpTcdSize)
        {
            EnableGlobalIRQ(primask);

            return kStatus_EDMA_QueueFull;
        }
        currentTcd = handle->tail;
        handle->tcdUsed++;
        /* Calculate index of next TCD */
        nextTcd = currentTcd + 1;
        if (nextTcd == handle->tcdSize)
        {
            nextTcd = 0;
        }
        /* Advance queue tail index */
        handle->tail = nextTcd;
        EnableGlobalIRQ(primask);
        /* Calculate index of previous TCD */
        previousTcd = currentTcd != 0 ? currentTcd - 1 : (handle->tcdSize - 1);
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
            csr = handle->tcdPool[previousTcd].CSR | ((uint16_t)DMA_CSR_ESG_MASK);
            csr &= ~((uint16_t)DMA_CSR_DREQ_MASK);
            handle->tcdPool[previousTcd].CSR = csr;
            /*
                Check if the TCD block in the registers is the previous one (points to current TCD block). It
                is used to check if the previous TCD linked has been loaded in TCD register. If so, it need to
                link the TCD register in case link the current TCD with the dead chain when TCD loading occurs
                before link the previous TCD block.
            */
            if (tcdRegs->DLAST_SGA == (uint32_t)&handle->tcdPool[currentTcd])
            {
                /* Clear the DREQ bits for the dynamic scatter gather */
                tcdRegs->CSR |= DMA_CSR_DREQ_MASK;
                /* Enable scatter/gather also in the TCD registers. */
                csr = tcdRegs->CSR | DMA_CSR_ESG_MASK;
                /* Must write the CSR register one-time, because the transfer maybe finished anytime. */
                tcdRegs->CSR = csr;
                /*
                    It is very important to check the ESG bit!
                    Because this hardware design: if DONE bit is set, the ESG bit can not be set. So it can
                    be used to check if the dynamic TCD link operation is successful. If ESG bit is not set
                    and the DLAST_SGA is not the next TCD address(it means the dynamic TCD link succeed and
                    the current TCD block has been loaded into TCD registers), it means transfer finished
                    and TCD link operation fail, so must install TCD content into TCD registers and enable
                    transfer again. And if ESG is set, it means transfer has not finished, so TCD dynamic
                    link succeed.
                */
                if (0U != (tcdRegs->CSR & DMA_CSR_ESG_MASK))
                {
                    tcdRegs->CSR &= ~(uint16_t)DMA_CSR_DREQ_MASK;
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
            else if (tcdRegs->DLAST_SGA != 0UL)
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
        if (0U != (handle->flags & EDMA_TRANSFER_ENABLED_MASK))
        {
            handle->base->SERQ = DMA_SERQ_SERQ(handle->channel);
        }

        return kStatus_Success;
    }
}

/*!
 * brief eDMA starts transfer.
 *
 * This function enables the channel request. Users can call this function after submitting the transfer request
 * or before submitting the transfer request.
 *
 * param handle eDMA handle pointer.
 */
void EDMA_StartTransfer(edma_handle_t *handle)
{
    assert(handle != NULL);
    uint32_t tmpCSR = 0;

    if (handle->tcdPool == NULL)
    {
        handle->base->SERQ = DMA_SERQ_SERQ(handle->channel);
    }
    else /* Use the TCD queue. */
    {
        uint32_t primask;
        edma_tcd_t *tcdRegs = (edma_tcd_t *)(uint32_t)&handle->base->TCD[handle->channel];

        handle->flags |= EDMA_TRANSFER_ENABLED_MASK;

        /* Check if there was at least one descriptor submitted since reset (TCD in registers is valid) */
        if (tcdRegs->DLAST_SGA != 0U)
        {
            primask = DisableGlobalIRQ();
            /* Check if channel request is actually disable. */
            if ((handle->base->ERQ & ((uint32_t)1U << handle->channel)) == 0U)
            {
                /* Check if transfer is paused. */
                tmpCSR = tcdRegs->CSR;
                if ((0U == (tmpCSR & DMA_CSR_DONE_MASK)) || (0U != (tmpCSR & DMA_CSR_ESG_MASK)))
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

/*!
 * brief eDMA stops transfer.
 *
 * This function disables the channel request to pause the transfer. Users can call EDMA_StartTransfer()
 * again to resume the transfer.
 *
 * param handle eDMA handle pointer.
 */
void EDMA_StopTransfer(edma_handle_t *handle)
{
    assert(handle != NULL);

    handle->flags &= (~(uint8_t)EDMA_TRANSFER_ENABLED_MASK);
    handle->base->CERQ = DMA_CERQ_CERQ(handle->channel);
}

/*!
 * brief eDMA aborts transfer.
 *
 * This function disables the channel request and clear transfer status bits.
 * Users can submit another transfer after calling this API.
 *
 * param handle DMA handle pointer.
 */
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
    /* clear the CITER and BITER to make sure the TCD register in a correct state for next calling of
     * EDMA_SubmitTransfer */
    handle->base->TCD[handle->channel].CITER_ELINKNO = 0;
    handle->base->TCD[handle->channel].BITER_ELINKNO = 0;

    /* Handle the tcd */
    if (handle->tcdPool != NULL)
    {
        handle->header  = 0;
        handle->tail    = 0;
        handle->tcdUsed = 0;
    }
}

/*!
 * brief eDMA IRQ handler for the current major loop transfer completion.
 *
 * This function clears the channel major interrupt flag and calls
 * the callback function if it is not NULL.
 *
 * Note:
 * For the case using TCD queue, when the major iteration count is exhausted, additional operations are performed.
 * These include the final address adjustments and reloading of the BITER field into the CITER.
 * Assertion of an optional interrupt request also occurs at this time, as does a possible fetch of a new TCD from
 * memory using the scatter/gather address pointer included in the descriptor (if scatter/gather is enabled).
 *
 * For instance, when the time interrupt of TCD[0] happens, the TCD[1] has already been loaded into the eDMA engine.
 * As sga and sga_index are calculated based on the DLAST_SGA bitfield lies in the TCD_CSR register, the sga_index
 * in this case should be 2 (DLAST_SGA of TCD[1] stores the address of TCD[2]). Thus, the "tcdUsed" updated should be
 * (tcdUsed - 2U) which indicates the number of TCDs can be loaded in the memory pool (because TCD[0] and TCD[1] have
 * been loaded into the eDMA engine at this point already.).
 *
 * For the last two continuous ISRs in a scatter/gather process, they  both load the last TCD (The last ISR does not
 * load a new TCD) from the memory pool to the eDMA engine when major loop completes.
 * Therefore, ensure that the header and tcdUsed updated are identical for them.
 * tcdUsed are both 0 in this case as no TCD to be loaded.
 *
 * See the "eDMA basic data flow" in the eDMA Functional description section of the Reference Manual for
 * further details.
 *
 * param handle eDMA handle pointer.
 */
void EDMA_HandleIRQ(edma_handle_t *handle)
{
    assert(handle != NULL);

    bool transfer_done;

    /* Clear EDMA interrupt flag */
    handle->base->CINT = handle->channel;
    /* Check if transfer is already finished. */
    transfer_done = ((handle->base->TCD[handle->channel].CSR & DMA_CSR_DONE_MASK) != 0U);

    if (handle->tcdPool == NULL)
    {
        if (handle->callback != NULL)
        {
            (handle->callback)(handle, handle->userData, transfer_done, 0);
        }
    }
    else /* Use the TCD queue. Please refer to the API descriptions in the eDMA header file for detailed information. */
    {
        uint32_t sga = handle->base->TCD[handle->channel].DLAST_SGA;
        uint32_t sga_index;
        int32_t tcds_done;
        uint8_t new_header;

        /* Get the offset of the next transfer TCD blocks to be loaded into the eDMA engine. */
        sga -= (uint32_t)handle->tcdPool;
        /* Get the index of the next transfer TCD blocks to be loaded into the eDMA engine. */
        sga_index = sga / sizeof(edma_tcd_t);
        /* Adjust header positions. */
        if (transfer_done)
        {
            /* New header shall point to the next TCD to be loaded (current one is already finished) */
            new_header = (uint8_t)sga_index;
        }
        else
        {
            /* New header shall point to this descriptor currently loaded (not finished yet) */
            new_header = sga_index != 0U ? (uint8_t)sga_index - 1U : (uint8_t)handle->tcdSize - 1U;
        }
        /* Calculate the number of finished TCDs */
        if (new_header == (uint8_t)handle->header)
        {
            int8_t tmpTcdUsed = handle->tcdUsed;
            int8_t tmpTcdSize = handle->tcdSize;

            if (tmpTcdUsed == tmpTcdSize)
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
            tcds_done = (int32_t)new_header - (int32_t)handle->header;
            if (tcds_done < 0)
            {
                tcds_done += handle->tcdSize;
            }
        }
        /* Advance header which points to the TCD to be loaded into the eDMA engine from memory. */
        handle->header = (int8_t)new_header;
        /* Release TCD blocks. tcdUsed is the TCD number which can be used/loaded in the memory pool. */
        handle->tcdUsed -= (int8_t)tcds_done;
        /* Invoke callback function. */
        if (NULL != handle->callback)
        {
            (handle->callback)(handle, handle->userData, transfer_done, tcds_done);
        }

        /*
         * 1.clear the DONE bit here is meaningful for below cases:
         * A new TCD has been loaded to EDMA already:
         * need to clear the DONE bit in the IRQ handler to avoid TCD in EDMA been overwritten
         * if peripheral request isn't coming before next transfer request.
         * 2. Don't clear DONE bit for below case,
         * for the case that transfer request submitted in the privious edma callback, this is a case that doesn't
         * need scatter gather, so keep DONE bit during the next transfer request submission will re-install the TCD and
         * the DONE bit will be cleared together with TCD re-installation.
         */
        if (transfer_done)
        {
            if ((handle->base->TCD[handle->channel].CSR & DMA_CSR_ESG_MASK) != 0U)
            {
                handle->base->CDNE = handle->channel;
            }
        }
    }
}

#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET) && \
    (FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET == 4)
/* 8 channels (Shared): kl28 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL == 8U)

#if defined(DMA0)
void DMA0_04_DriverIRQHandler(void);
void DMA0_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_15_DriverIRQHandler(void);
void DMA0_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_26_DriverIRQHandler(void);
void DMA0_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_37_DriverIRQHandler(void);
void DMA0_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    SDK_ISR_EXIT_BARRIER;
}
#endif

#if defined(DMA1)

#if defined(DMA0)
void DMA1_04_DriverIRQHandler(void);
void DMA1_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_15_DriverIRQHandler(void);
void DMA1_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_26_DriverIRQHandler(void);
void DMA1_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_37_DriverIRQHandler(void);
void DMA1_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    SDK_ISR_EXIT_BARRIER;
}

#else
void DMA1_04_DriverIRQHandler(void);
void DMA1_04_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_15_DriverIRQHandler(void);
void DMA1_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_26_DriverIRQHandler(void);
void DMA1_26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_37_DriverIRQHandler(void);
void DMA1_37_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    SDK_ISR_EXIT_BARRIER;
}
#endif
#endif
#endif /* 8 channels (Shared) */
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET */

#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET) && \
    (FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET == 8)
/* 16 channels (Shared): K32H844P */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL == 16U)

void DMA0_08_DriverIRQHandler(void);
void DMA0_08_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_19_DriverIRQHandler(void);
void DMA0_19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_210_DriverIRQHandler(void);
void DMA0_210_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_311_DriverIRQHandler(void);
void DMA0_311_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_412_DriverIRQHandler(void);
void DMA0_412_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_513_DriverIRQHandler(void);
void DMA0_513_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_614_DriverIRQHandler(void);
void DMA0_614_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_715_DriverIRQHandler(void);
void DMA0_715_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    SDK_ISR_EXIT_BARRIER;
}

#if defined(DMA1)
void DMA1_08_DriverIRQHandler(void);
void DMA1_08_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 8U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_19_DriverIRQHandler(void);
void DMA1_19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 9U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_210_DriverIRQHandler(void);
void DMA1_210_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 10U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_311_DriverIRQHandler(void);
void DMA1_311_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 11U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_412_DriverIRQHandler(void);
void DMA1_412_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 12U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_513_DriverIRQHandler(void);
void DMA1_513_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 13U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_614_DriverIRQHandler(void);
void DMA1_614_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 14U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_715_DriverIRQHandler(void);
void DMA1_715_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 15U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    SDK_ISR_EXIT_BARRIER;
}
#endif
#endif /* 16 channels (Shared) */
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET */

#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET) && \
    (FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET == 16)
/* 32 channels (Shared): k80 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && FSL_FEATURE_EDMA_MODULE_CHANNEL == 32U
#if defined(DMA0)
void DMA0_DMA16_DriverIRQHandler(void);
void DMA0_DMA16_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 16U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_DMA17_DriverIRQHandler(void);
void DMA1_DMA17_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 17U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA2_DMA18_DriverIRQHandler(void);
void DMA2_DMA18_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 18U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA3_DMA19_DriverIRQHandler(void);
void DMA3_DMA19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 19U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA4_DMA20_DriverIRQHandler(void);
void DMA4_DMA20_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 20U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA5_DMA21_DriverIRQHandler(void);
void DMA5_DMA21_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 21U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA6_DMA22_DriverIRQHandler(void);
void DMA6_DMA22_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 22U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA7_DMA23_DriverIRQHandler(void);
void DMA7_DMA23_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 23U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA8_DMA24_DriverIRQHandler(void);
void DMA8_DMA24_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 24U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA9_DMA25_DriverIRQHandler(void);
void DMA9_DMA25_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 25U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA10_DMA26_DriverIRQHandler(void);
void DMA10_DMA26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 26U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA11_DMA27_DriverIRQHandler(void);
void DMA11_DMA27_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 27U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA12_DMA28_DriverIRQHandler(void);
void DMA12_DMA28_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 28U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA13_DMA29_DriverIRQHandler(void);
void DMA13_DMA29_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 29U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA14_DMA30_DriverIRQHandler(void);
void DMA14_DMA30_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 30U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA15_DMA31_DriverIRQHandler(void);
void DMA15_DMA31_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 31U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    SDK_ISR_EXIT_BARRIER;
}

#else
void DMA0_DMA16_DriverIRQHandler(void);
void DMA0_DMA16_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 16U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_DMA17_DriverIRQHandler(void);
void DMA1_DMA17_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 17U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA2_DMA18_DriverIRQHandler(void);
void DMA2_DMA18_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 18U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA3_DMA19_DriverIRQHandler(void);
void DMA3_DMA19_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 19U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA4_DMA20_DriverIRQHandler(void);
void DMA4_DMA20_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 20U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA5_DMA21_DriverIRQHandler(void);
void DMA5_DMA21_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 21U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA6_DMA22_DriverIRQHandler(void);
void DMA6_DMA22_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 22U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA7_DMA23_DriverIRQHandler(void);
void DMA7_DMA23_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 23U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA8_DMA24_DriverIRQHandler(void);
void DMA8_DMA24_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 8U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 24U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA9_DMA25_DriverIRQHandler(void);
void DMA9_DMA25_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 9U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 25U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA10_DMA26_DriverIRQHandler(void);
void DMA10_DMA26_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 10U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 26U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA11_DMA27_DriverIRQHandler(void);
void DMA11_DMA27_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 11U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 27U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA12_DMA28_DriverIRQHandler(void);
void DMA12_DMA28_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 12U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 28U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA13_DMA29_DriverIRQHandler(void);
void DMA13_DMA29_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 13U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 29U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA14_DMA30_DriverIRQHandler(void);
void DMA14_DMA30_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 14U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 30U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA15_DMA31_DriverIRQHandler(void);
void DMA15_DMA31_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA1, 15U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA1, 31U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    SDK_ISR_EXIT_BARRIER;
}

#endif
#endif /* 32 channels (Shared) */
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET */

#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET) && \
    (FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET == 4)
/* 32 channels (Shared): MCIMX7U5_M4 */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL == 32U)

void DMA0_0_4_DriverIRQHandler(void);
void DMA0_0_4_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 0U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[0]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 4U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[4]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_1_5_DriverIRQHandler(void);
void DMA0_1_5_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 1U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[1]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 5U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[5]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_2_6_DriverIRQHandler(void);
void DMA0_2_6_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 2U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[2]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 6U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[6]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_3_7_DriverIRQHandler(void);
void DMA0_3_7_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 3U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[3]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 7U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[7]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_8_12_DriverIRQHandler(void);
void DMA0_8_12_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 8U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[8]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 12U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[12]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_9_13_DriverIRQHandler(void);
void DMA0_9_13_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 9U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[9]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 13U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[13]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_10_14_DriverIRQHandler(void);
void DMA0_10_14_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 10U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[10]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 14U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[14]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_11_15_DriverIRQHandler(void);
void DMA0_11_15_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 11U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[11]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 15U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[15]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_16_20_DriverIRQHandler(void);
void DMA0_16_20_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 16U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[16]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 20U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[20]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_17_21_DriverIRQHandler(void);
void DMA0_17_21_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 17U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[17]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 21U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[21]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_18_22_DriverIRQHandler(void);
void DMA0_18_22_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 18U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[18]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 22U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[22]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_19_23_DriverIRQHandler(void);
void DMA0_19_23_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 19U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[19]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 23U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[23]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_24_28_DriverIRQHandler(void);
void DMA0_24_28_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 24U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[24]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 28U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[28]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_25_29_DriverIRQHandler(void);
void DMA0_25_29_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 25U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[25]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 29U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[29]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_26_30_DriverIRQHandler(void);
void DMA0_26_30_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 26U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[26]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 30U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[30]);
    }
    SDK_ISR_EXIT_BARRIER;
}

void DMA0_27_31_DriverIRQHandler(void);
void DMA0_27_31_DriverIRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(DMA0, 27U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[27]);
    }
    if ((EDMA_GetChannelStatusFlags(DMA0, 31U) & (uint32_t)kEDMA_InterruptFlag) != 0U)
    {
        EDMA_HandleIRQ(s_EDMAHandle[31]);
    }
    SDK_ISR_EXIT_BARRIER;
}
#endif /* 32 channels (Shared): MCIMX7U5 */
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET */

#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET) && \
    (FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET == 0)
/* 4 channels (No Shared): kv10  */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL > 0)

void DMA0_DriverIRQHandler(void);
void DMA0_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[0]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA1_DriverIRQHandler(void);
void DMA1_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[1]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA2_DriverIRQHandler(void);
void DMA2_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[2]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA3_DriverIRQHandler(void);
void DMA3_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[3]);
    SDK_ISR_EXIT_BARRIER;
}

/* 8 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL > 4U)

void DMA4_DriverIRQHandler(void);
void DMA4_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[4]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA5_DriverIRQHandler(void);
void DMA5_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[5]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA6_DriverIRQHandler(void);
void DMA6_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[6]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA7_DriverIRQHandler(void);
void DMA7_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[7]);
    SDK_ISR_EXIT_BARRIER;
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 8 */

/* 16 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL > 8U)

void DMA8_DriverIRQHandler(void);
void DMA8_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[8]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA9_DriverIRQHandler(void);
void DMA9_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[9]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA10_DriverIRQHandler(void);
void DMA10_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[10]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA11_DriverIRQHandler(void);
void DMA11_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[11]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA12_DriverIRQHandler(void);
void DMA12_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[12]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA13_DriverIRQHandler(void);
void DMA13_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[13]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA14_DriverIRQHandler(void);
void DMA14_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[14]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA15_DriverIRQHandler(void);
void DMA15_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[15]);
    SDK_ISR_EXIT_BARRIER;
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 16 */

/* 32 channels (No Shared) */
#if defined(FSL_FEATURE_EDMA_MODULE_CHANNEL) && (FSL_FEATURE_EDMA_MODULE_CHANNEL > 16U)

void DMA16_DriverIRQHandler(void);
void DMA16_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[16]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA17_DriverIRQHandler(void);
void DMA17_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[17]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA18_DriverIRQHandler(void);
void DMA18_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[18]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA19_DriverIRQHandler(void);
void DMA19_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[19]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA20_DriverIRQHandler(void);
void DMA20_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[20]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA21_DriverIRQHandler(void);
void DMA21_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[21]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA22_DriverIRQHandler(void);
void DMA22_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[22]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA23_DriverIRQHandler(void);
void DMA23_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[23]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA24_DriverIRQHandler(void);
void DMA24_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[24]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA25_DriverIRQHandler(void);
void DMA25_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[25]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA26_DriverIRQHandler(void);
void DMA26_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[26]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA27_DriverIRQHandler(void);
void DMA27_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[27]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA28_DriverIRQHandler(void);
void DMA28_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[28]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA29_DriverIRQHandler(void);
void DMA29_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[29]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA30_DriverIRQHandler(void);
void DMA30_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[30]);
    SDK_ISR_EXIT_BARRIER;
}

void DMA31_DriverIRQHandler(void);
void DMA31_DriverIRQHandler(void)
{
    EDMA_HandleIRQ(s_EDMAHandle[31]);
    SDK_ISR_EXIT_BARRIER;
}
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL == 32 */

#endif /* 4/8/16/32 channels (No Shared)  */
#endif /* FSL_FEATURE_EDMA_MODULE_CHANNEL_IRQ_ENTRY_SHARED_OFFSET */
