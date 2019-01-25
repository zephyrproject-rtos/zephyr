/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_dmic.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.dmic"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Array of DMIC peripheral base address. */
static DMIC_Type *const s_dmicBases[FSL_FEATURE_SOC_DMIC_COUNT] = DMIC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of DMIC clock name. */
static const clock_ip_name_t s_dmicClock[FSL_FEATURE_SOC_DMIC_COUNT] = DMIC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* Array of DMIC IRQ number. */
static const IRQn_Type s_dmicIRQ[FSL_FEATURE_SOC_DMIC_COUNT] = DMIC_IRQS;

/*! @brief Callback function array for DMIC(s). */
static dmic_callback_t s_dmicCallback[FSL_FEATURE_SOC_DMIC_COUNT];

/* Array of HWVAD IRQ number. */
static const IRQn_Type s_dmicHwvadIRQ[FSL_FEATURE_SOC_DMIC_COUNT] = DMIC_HWVAD_IRQS;

/*! @brief Callback function array for HWVAD(s). */
static dmic_hwvad_callback_t s_dmicHwvadCallback[FSL_FEATURE_SOC_DMIC_COUNT];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the DMIC instance from peripheral base address.
 *
 * @param base DMIC peripheral base address.
 * @return DMIC instance.
 */
/*!
 * brief Get the DMIC instance from peripheral base address.
 *
 * param base DMIC peripheral base address.
 * return DMIC instance.
 */
uint32_t DMIC_GetInstance(DMIC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dmicBases); instance++)
    {
        if (s_dmicBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dmicBases));

    return instance;
}

/*!
 * brief	Turns DMIC Clock on
 * param	base	: DMIC base
 * return	Nothing
 */
void DMIC_Init(DMIC_Type *base)
{
    assert(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_dmicClock[DMIC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(kDMIC_RST_SHIFT_RSTn);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

    /* Disable DMA request*/
    base->CHANNEL[0].FIFO_CTRL &= ~DMIC_CHANNEL_FIFO_CTRL_DMAEN(1);
    base->CHANNEL[1].FIFO_CTRL &= ~DMIC_CHANNEL_FIFO_CTRL_DMAEN(1);

    /* Disable DMIC interrupt. */
    base->CHANNEL[0].FIFO_CTRL &= ~DMIC_CHANNEL_FIFO_CTRL_INTEN(1);
    base->CHANNEL[1].FIFO_CTRL &= ~DMIC_CHANNEL_FIFO_CTRL_INTEN(1);
}

/*!
 * brief	Turns DMIC Clock off
 * param	base	: DMIC base
 * return	Nothing
 */
void DMIC_DeInit(DMIC_Type *base)
{
    assert(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_dmicClock[DMIC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief	Configure DMIC io
 * param	base	: The base address of DMIC interface
 * param	config		: DMIC io configuration
 * return	Nothing
 */
void DMIC_ConfigIO(DMIC_Type *base, dmic_io_t config)
{
    base->IOCFG = config;
}

/*!
 * brief	Set DMIC operating mode
 * param	base	: The base address of DMIC interface
 * param	mode	: DMIC mode
 * return	Nothing
 */
void DMIC_SetOperationMode(DMIC_Type *base, operation_mode_t mode)
{
    if (mode == kDMIC_OperationModeInterrupt)
    {
        /* Enable DMIC interrupt. */
        base->CHANNEL[0].FIFO_CTRL |= DMIC_CHANNEL_FIFO_CTRL_INTEN(1);
        base->CHANNEL[1].FIFO_CTRL |= DMIC_CHANNEL_FIFO_CTRL_INTEN(1);
    }
    if (mode == kDMIC_OperationModeDma)
    {
        /* enable DMA request*/
        base->CHANNEL[0].FIFO_CTRL |= DMIC_CHANNEL_FIFO_CTRL_DMAEN(1);
        base->CHANNEL[1].FIFO_CTRL |= DMIC_CHANNEL_FIFO_CTRL_DMAEN(1);
    }
}

/*!
 * brief	Configure DMIC channel
 * param	base		: The base address of DMIC interface
 * param	channel		: DMIC channel
 * param side     : stereo_side_t, choice of left or right
 * param	channel_config	: Channel configuration
 * return	Nothing
 */
void DMIC_ConfigChannel(DMIC_Type *base,
                        dmic_channel_t channel,
                        stereo_side_t side,
                        dmic_channel_config_t *channel_config)
{
    base->CHANNEL[channel].DIVHFCLK = channel_config->divhfclk;
    base->CHANNEL[channel].OSR = channel_config->osr;
    base->CHANNEL[channel].GAINSHIFT = channel_config->gainshft;
    base->CHANNEL[channel].PREAC2FSCOEF = channel_config->preac2coef;
    base->CHANNEL[channel].PREAC4FSCOEF = channel_config->preac4coef;
    base->CHANNEL[channel].PHY_CTRL =
        DMIC_CHANNEL_PHY_CTRL_PHY_FALL(side) | DMIC_CHANNEL_PHY_CTRL_PHY_HALF(channel_config->sample_rate);
    base->CHANNEL[channel].DC_CTRL = DMIC_CHANNEL_DC_CTRL_DCPOLE(channel_config->dc_cut_level) |
                                     DMIC_CHANNEL_DC_CTRL_DCGAIN(channel_config->post_dc_gain_reduce) |
                                     DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT(channel_config->saturate16bit);
}

/*!
 * brief	Configure DMIC channel
 * param	base		: The base address of DMIC interface
 * param	channel		: DMIC channel
 * param   dc_cut_level  : dc_removal_t, Cut off Frequency
 * param	post_dc_gain_reduce	: Fine gain adjustment in the form of a number of bits to downshift.
 * param   saturate16bit : If selects 16-bit saturation.
 */
void DMIC_CfgChannelDc(DMIC_Type *base,
                       dmic_channel_t channel,
                       dc_removal_t dc_cut_level,
                       uint32_t post_dc_gain_reduce,
                       bool saturate16bit)
{
    base->CHANNEL[channel].DC_CTRL = DMIC_CHANNEL_DC_CTRL_DCPOLE(dc_cut_level) |
                                     DMIC_CHANNEL_DC_CTRL_DCGAIN(post_dc_gain_reduce) |
                                     DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT(saturate16bit);
}

/*!
 * brief	Configure Clock scaling
 * param	base		: The base address of DMIC interface
 * param	use2fs		: clock scaling
 * return	Nothing
 */
void DMIC_Use2fs(DMIC_Type *base, bool use2fs)
{
    base->USE2FS = (use2fs) ? 0x1 : 0x0;
}

/*!
 * brief	Enable a particualr channel
 * param	base		: The base address of DMIC interface
 * param	channelmask	: Channel selection
 * return	Nothing
 */
void DMIC_EnableChannnel(DMIC_Type *base, uint32_t channelmask)
{
    base->CHANEN = channelmask;
}

/*!
 * brief	Configure fifo settings for DMIC channel
 * param	base		: The base address of DMIC interface
 * param	channel		: DMIC channel
 * param	trig_level	: FIFO trigger level
 * param	enable		: FIFO level
 * param	resetn		: FIFO reset
 * return	Nothing
 */
void DMIC_FifoChannel(DMIC_Type *base, uint32_t channel, uint32_t trig_level, uint32_t enable, uint32_t resetn)
{
    base->CHANNEL[channel].FIFO_CTRL |=
        (base->CHANNEL[channel].FIFO_CTRL & (DMIC_CHANNEL_FIFO_CTRL_INTEN_MASK | DMIC_CHANNEL_FIFO_CTRL_DMAEN_MASK)) |
        DMIC_CHANNEL_FIFO_CTRL_TRIGLVL(trig_level) | DMIC_CHANNEL_FIFO_CTRL_ENABLE(enable) |
        DMIC_CHANNEL_FIFO_CTRL_RESETN(resetn);
}

/*!
 * brief	Enable callback.

 * This function enables the interrupt for the selected DMIC peripheral.
 * The callback function is not enabled until this function is called.
 *
 * param base Base address of the DMIC peripheral.
 * param cb callback Pointer to store callback function.
 * retval None.
 */
void DMIC_EnableIntCallback(DMIC_Type *base, dmic_callback_t cb)
{
    uint32_t instance;

    instance = DMIC_GetInstance(base);
    NVIC_ClearPendingIRQ(s_dmicIRQ[instance]);
    /* Save callback pointer */
    s_dmicCallback[instance] = cb;
    EnableIRQ(s_dmicIRQ[instance]);
}

/*!
 * brief	Disable callback.

 * This function disables the interrupt for the selected DMIC peripheral.
 *
 * param base Base address of the DMIC peripheral.
 * param cb callback Pointer to store callback function..
 * retval None.
 */
void DMIC_DisableIntCallback(DMIC_Type *base, dmic_callback_t cb)
{
    uint32_t instance;

    instance = DMIC_GetInstance(base);
    DisableIRQ(s_dmicIRQ[instance]);
    s_dmicCallback[instance] = NULL;
    NVIC_ClearPendingIRQ(s_dmicIRQ[instance]);
}

/*!
 * brief	Enable hwvad callback.

 * This function enables the hwvad interrupt for the selected DMIC  peripheral.
 * The callback function is not enabled until this function is called.
 *
 * param base Base address of the DMIC peripheral.
 * param vadcb callback Pointer to store callback function.
 * retval None.
 */
void DMIC_HwvadEnableIntCallback(DMIC_Type *base, dmic_hwvad_callback_t vadcb)
{
    uint32_t instance;

    instance = DMIC_GetInstance(base);
    NVIC_ClearPendingIRQ(s_dmicHwvadIRQ[instance]);
    /* Save callback pointer */
    s_dmicHwvadCallback[instance] = vadcb;
    EnableIRQ(s_dmicHwvadIRQ[instance]);
}

/*!
 * brief	Disable callback.

 * This function disables the hwvad interrupt for the selected DMIC peripheral.
 *
 * param base Base address of the DMIC peripheral.
 * param vadcb callback Pointer to store callback function..
 * retval None.
 */
void DMIC_HwvadDisableIntCallback(DMIC_Type *base, dmic_hwvad_callback_t vadcb)
{
    uint32_t instance;

    instance = DMIC_GetInstance(base);
    DisableIRQ(s_dmicHwvadIRQ[instance]);
    s_dmicHwvadCallback[instance] = NULL;
    NVIC_ClearPendingIRQ(s_dmicHwvadIRQ[instance]);
}

/* IRQ handler functions overloading weak symbols in the startup */
#if defined(DMIC0)
/*DMIC0 IRQ handler */
void DMIC0_DriverIRQHandler(void)
{
    if (s_dmicCallback[0] != NULL)
    {
        s_dmicCallback[0]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
/*DMIC0 HWVAD IRQ handler */
void HWVAD0_DriverIRQHandler(void)
{
    if (s_dmicHwvadCallback[0] != NULL)
    {
        s_dmicHwvadCallback[0]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
