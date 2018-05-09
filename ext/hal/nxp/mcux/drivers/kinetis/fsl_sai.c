/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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

#include "fsl_sai.h"

/*******************************************************************************
 * Definitations
 ******************************************************************************/
enum _sai_transfer_state
{
    kSAI_Busy = 0x0U, /*!< SAI is busy */
    kSAI_Idle,        /*!< Transfer is done. */
    kSAI_Error        /*!< Transfer error occured. */
};

/*! @brief Typedef for sai tx interrupt handler. */
typedef void (*sai_tx_isr_t)(I2S_Type *base, sai_handle_t *saiHandle);

/*! @brief Typedef for sai rx interrupt handler. */
typedef void (*sai_rx_isr_t)(I2S_Type *base, sai_handle_t *saiHandle);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)

/*!
 * @brief Set the master clock divider.
 *
 * This API will compute the master clock divider according to master clock frequency and master
 * clock source clock source frequency.
 *
 * @param base SAI base pointer.
 * @param mclk_Hz Mater clock frequency in Hz.
 * @param mclkSrcClock_Hz Master clock source frequency in Hz.
 */
static void SAI_SetMasterClockDivider(I2S_Type *base, uint32_t mclk_Hz, uint32_t mclkSrcClock_Hz);
#endif /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */

/*!
 * @brief Get the instance number for SAI.
 *
 * @param base SAI base pointer.
 */
uint32_t SAI_GetInstance(I2S_Type *base);

/*!
 * @brief sends a piece of data in non-blocking way.
 *
 * @param base SAI base pointer
 * @param channel Data channel used.
 * @param bitWidth How many bits in a audio word, usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be written.
 * @param size Bytes to be written.
 */
static void SAI_WriteNonBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Receive a piece of data in non-blocking way.
 *
 * @param base SAI base pointer
 * @param channel Data channel used.
 * @param bitWidth How many bits in a audio word, usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be read.
 * @param size Bytes to be read.
 */
static void SAI_ReadNonBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);
/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Base pointer array */
static I2S_Type *const s_saiBases[] = I2S_BASE_PTRS;
/*!@brief SAI handle pointer */
sai_handle_t *s_saiHandle[ARRAY_SIZE(s_saiBases)][2];
/* IRQ number array */
static const IRQn_Type s_saiTxIRQ[] = I2S_TX_IRQS;
static const IRQn_Type s_saiRxIRQ[] = I2S_RX_IRQS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Clock name array */
static const clock_ip_name_t s_saiClock[] = SAI_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
/*! @brief Pointer to tx IRQ handler for each instance. */
static sai_tx_isr_t s_saiTxIsr;
/*! @brief Pointer to tx IRQ handler for each instance. */
static sai_rx_isr_t s_saiRxIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
static void SAI_SetMasterClockDivider(I2S_Type *base, uint32_t mclk_Hz, uint32_t mclkSrcClock_Hz)
{
    uint32_t freq = mclkSrcClock_Hz;
    uint16_t fract, divide;
    uint32_t remaind = 0;
    uint32_t current_remainder = 0xFFFFFFFFU;
    uint16_t current_fract = 0;
    uint16_t current_divide = 0;
    uint32_t mul_freq = 0;
    uint32_t max_fract = 256;

    /*In order to prevent overflow */
    freq /= 100;
    mclk_Hz /= 100;

    /* Compute the max fract number */
    max_fract = mclk_Hz * 4096 / freq + 1;
    if (max_fract > 256)
    {
        max_fract = 256;
    }

    /* Looking for the closet frequency */
    for (fract = 1; fract < max_fract; fract++)
    {
        mul_freq = freq * fract;
        remaind = mul_freq % mclk_Hz;
        divide = mul_freq / mclk_Hz;

        /* Find the exactly frequency */
        if (remaind == 0)
        {
            current_fract = fract;
            current_divide = mul_freq / mclk_Hz;
            break;
        }

        /* Closer to next one, set the closest to next data */
        if (remaind > mclk_Hz / 2)
        {
            remaind = mclk_Hz - remaind;
            divide += 1;
        }

        /* Update the closest div and fract */
        if (remaind < current_remainder)
        {
            current_fract = fract;
            current_divide = divide;
            current_remainder = remaind;
        }
    }

    /* Fill the computed fract and divider to registers */
    base->MDR = I2S_MDR_DIVIDE(current_divide - 1) | I2S_MDR_FRACT(current_fract - 1);

    /* Waiting for the divider updated */
    while (base->MCR & I2S_MCR_DUF_MASK)
    {
    }
}
#endif /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */

uint32_t SAI_GetInstance(I2S_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_saiBases); instance++)
    {
        if (s_saiBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_saiBases));

    return instance;
}

static void SAI_WriteNonBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i = 0;
    uint8_t j = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data = 0;
    uint32_t temp = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        for (j = 0; j < bytesPerWord; j++)
        {
            temp = (uint32_t)(*buffer);
            data |= (temp << (8U * j));
            buffer++;
        }
        base->TDR[channel] = data;
        data = 0;
    }
}

static void SAI_ReadNonBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i = 0;
    uint8_t j = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        data = base->RDR[channel];
        for (j = 0; j < bytesPerWord; j++)
        {
            *buffer = (data >> (8U * j)) & 0xFF;
            buffer++;
        }
    }
}

void SAI_TxInit(I2S_Type *base, const sai_config_t *config)
{
    uint32_t val = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the SAI clock */
    CLOCK_EnableClock(s_saiClock[SAI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    /* Master clock source setting */
    val = (base->MCR & ~I2S_MCR_MICS_MASK);
    base->MCR = (val | I2S_MCR_MICS(config->mclkSource));

    /* Configure Master clock output enable */
    val = (base->MCR & ~I2S_MCR_MOE_MASK);
    base->MCR = (val | I2S_MCR_MOE(config->mclkOutputEnable));
#endif /* FSL_FEATURE_SAI_HAS_MCR */

    /* Configure audio protocol */
    switch (config->protocol)
    {
        case kSAI_BusLeftJustified:
            base->TCR2 |= I2S_TCR2_BCP_MASK;
            base->TCR3 &= ~I2S_TCR3_WDFL_MASK;
            base->TCR4 = I2S_TCR4_MF(1U) | I2S_TCR4_SYWD(31U) | I2S_TCR4_FSE(0U) | I2S_TCR4_FSP(0U) | I2S_TCR4_FRSZ(1U);
            break;

        case kSAI_BusRightJustified:
            base->TCR2 |= I2S_TCR2_BCP_MASK;
            base->TCR3 &= ~I2S_TCR3_WDFL_MASK;
            base->TCR4 = I2S_TCR4_MF(1U) | I2S_TCR4_SYWD(31U) | I2S_TCR4_FSE(0U) | I2S_TCR4_FSP(0U) | I2S_TCR4_FRSZ(1U);
            break;

        case kSAI_BusI2S:
            base->TCR2 |= I2S_TCR2_BCP_MASK;
            base->TCR3 &= ~I2S_TCR3_WDFL_MASK;
            base->TCR4 = I2S_TCR4_MF(1U) | I2S_TCR4_SYWD(31U) | I2S_TCR4_FSE(1U) | I2S_TCR4_FSP(1U) | I2S_TCR4_FRSZ(1U);
            break;

        case kSAI_BusPCMA:
            base->TCR2 &= ~I2S_TCR2_BCP_MASK;
            base->TCR3 &= ~I2S_TCR3_WDFL_MASK;
            base->TCR4 = I2S_TCR4_MF(1U) | I2S_TCR4_SYWD(0U) | I2S_TCR4_FSE(1U) | I2S_TCR4_FSP(0U) | I2S_TCR4_FRSZ(1U);
            break;

        case kSAI_BusPCMB:
            base->TCR2 &= ~I2S_TCR2_BCP_MASK;
            base->TCR3 &= ~I2S_TCR3_WDFL_MASK;
            base->TCR4 = I2S_TCR4_MF(1U) | I2S_TCR4_SYWD(0U) | I2S_TCR4_FSE(0U) | I2S_TCR4_FSP(0U) | I2S_TCR4_FRSZ(1U);
            break;

        default:
            break;
    }

    /* Set master or slave */
    if (config->masterSlave == kSAI_Master)
    {
        base->TCR2 |= I2S_TCR2_BCD_MASK;
        base->TCR4 |= I2S_TCR4_FSD_MASK;

        /* Bit clock source setting */
        val = base->TCR2 & (~I2S_TCR2_MSEL_MASK);
        base->TCR2 = (val | I2S_TCR2_MSEL(config->bclkSource));
    }
    else
    {
        base->TCR2 &= ~I2S_TCR2_BCD_MASK;
        base->TCR4 &= ~I2S_TCR4_FSD_MASK;
    }

    /* Set Sync mode */
    switch (config->syncMode)
    {
        case kSAI_ModeAsync:
            val = base->TCR2;
            val &= ~I2S_TCR2_SYNC_MASK;
            base->TCR2 = (val | I2S_TCR2_SYNC(0U));
            break;
        case kSAI_ModeSync:
            val = base->TCR2;
            val &= ~I2S_TCR2_SYNC_MASK;
            base->TCR2 = (val | I2S_TCR2_SYNC(1U));
            /* If sync with Rx, should set Rx to async mode */
            val = base->RCR2;
            val &= ~I2S_RCR2_SYNC_MASK;
            base->RCR2 = (val | I2S_RCR2_SYNC(0U));
            break;
        case kSAI_ModeSyncWithOtherTx:
            val = base->TCR2;
            val &= ~I2S_TCR2_SYNC_MASK;
            base->TCR2 = (val | I2S_TCR2_SYNC(2U));
            break;
        case kSAI_ModeSyncWithOtherRx:
            val = base->TCR2;
            val &= ~I2S_TCR2_SYNC_MASK;
            base->TCR2 = (val | I2S_TCR2_SYNC(3U));
            break;
        default:
            break;
    }
}

void SAI_RxInit(I2S_Type *base, const sai_config_t *config)
{
    uint32_t val = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable SAI clock first. */
    CLOCK_EnableClock(s_saiClock[SAI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    /* Master clock source setting */
    val = (base->MCR & ~I2S_MCR_MICS_MASK);
    base->MCR = (val | I2S_MCR_MICS(config->mclkSource));

    /* Configure Master clock output enable */
    val = (base->MCR & ~I2S_MCR_MOE_MASK);
    base->MCR = (val | I2S_MCR_MOE(config->mclkOutputEnable));
#endif /* FSL_FEATURE_SAI_HAS_MCR */

    /* Configure audio protocol */
    switch (config->protocol)
    {
        case kSAI_BusLeftJustified:
            base->RCR2 |= I2S_RCR2_BCP_MASK;
            base->RCR3 &= ~I2S_RCR3_WDFL_MASK;
            base->RCR4 = I2S_RCR4_MF(1U) | I2S_RCR4_SYWD(31U) | I2S_RCR4_FSE(0U) | I2S_RCR4_FSP(0U) | I2S_RCR4_FRSZ(1U);
            break;

        case kSAI_BusRightJustified:
            base->RCR2 |= I2S_RCR2_BCP_MASK;
            base->RCR3 &= ~I2S_RCR3_WDFL_MASK;
            base->RCR4 = I2S_RCR4_MF(1U) | I2S_RCR4_SYWD(31U) | I2S_RCR4_FSE(0U) | I2S_RCR4_FSP(0U) | I2S_RCR4_FRSZ(1U);
            break;

        case kSAI_BusI2S:
            base->RCR2 |= I2S_RCR2_BCP_MASK;
            base->RCR3 &= ~I2S_RCR3_WDFL_MASK;
            base->RCR4 = I2S_RCR4_MF(1U) | I2S_RCR4_SYWD(31U) | I2S_RCR4_FSE(1U) | I2S_RCR4_FSP(1U) | I2S_RCR4_FRSZ(1U);
            break;

        case kSAI_BusPCMA:
            base->RCR2 &= ~I2S_RCR2_BCP_MASK;
            base->RCR3 &= ~I2S_RCR3_WDFL_MASK;
            base->RCR4 = I2S_RCR4_MF(1U) | I2S_RCR4_SYWD(0U) | I2S_RCR4_FSE(1U) | I2S_RCR4_FSP(0U) | I2S_RCR4_FRSZ(1U);
            break;

        case kSAI_BusPCMB:
            base->RCR2 &= ~I2S_RCR2_BCP_MASK;
            base->RCR3 &= ~I2S_RCR3_WDFL_MASK;
            base->RCR4 = I2S_RCR4_MF(1U) | I2S_RCR4_SYWD(0U) | I2S_RCR4_FSE(0U) | I2S_RCR4_FSP(0U) | I2S_RCR4_FRSZ(1U);
            break;

        default:
            break;
    }

    /* Set master or slave */
    if (config->masterSlave == kSAI_Master)
    {
        base->RCR2 |= I2S_RCR2_BCD_MASK;
        base->RCR4 |= I2S_RCR4_FSD_MASK;

        /* Bit clock source setting */
        val = base->RCR2 & (~I2S_RCR2_MSEL_MASK);
        base->RCR2 = (val | I2S_RCR2_MSEL(config->bclkSource));
    }
    else
    {
        base->RCR2 &= ~I2S_RCR2_BCD_MASK;
        base->RCR4 &= ~I2S_RCR4_FSD_MASK;
    }

    /* Set Sync mode */
    switch (config->syncMode)
    {
        case kSAI_ModeAsync:
            val = base->RCR2;
            val &= ~I2S_RCR2_SYNC_MASK;
            base->RCR2 = (val | I2S_RCR2_SYNC(0U));
            break;
        case kSAI_ModeSync:
            val = base->RCR2;
            val &= ~I2S_RCR2_SYNC_MASK;
            base->RCR2 = (val | I2S_RCR2_SYNC(1U));
            /* If sync with Tx, should set Tx to async mode */
            val = base->TCR2;
            val &= ~I2S_TCR2_SYNC_MASK;
            base->TCR2 = (val | I2S_TCR2_SYNC(0U));
            break;
        case kSAI_ModeSyncWithOtherTx:
            val = base->RCR2;
            val &= ~I2S_RCR2_SYNC_MASK;
            base->RCR2 = (val | I2S_RCR2_SYNC(2U));
            break;
        case kSAI_ModeSyncWithOtherRx:
            val = base->RCR2;
            val &= ~I2S_RCR2_SYNC_MASK;
            base->RCR2 = (val | I2S_RCR2_SYNC(3U));
            break;
        default:
            break;
    }
}

void SAI_Deinit(I2S_Type *base)
{
    SAI_TxEnable(base, false);
    SAI_RxEnable(base, false);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_saiClock[SAI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void SAI_TxGetDefaultConfig(sai_config_t *config)
{
    config->bclkSource = kSAI_BclkSourceMclkDiv;
    config->masterSlave = kSAI_Master;
    config->mclkSource = kSAI_MclkSourceSysclk;
    config->protocol = kSAI_BusLeftJustified;
    config->syncMode = kSAI_ModeAsync;
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    config->mclkOutputEnable = true;
#endif /* FSL_FEATURE_SAI_HAS_MCR */
}

void SAI_RxGetDefaultConfig(sai_config_t *config)
{
    config->bclkSource = kSAI_BclkSourceMclkDiv;
    config->masterSlave = kSAI_Master;
    config->mclkSource = kSAI_MclkSourceSysclk;
    config->protocol = kSAI_BusLeftJustified;
    config->syncMode = kSAI_ModeSync;
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    config->mclkOutputEnable = true;
#endif /* FSL_FEATURE_SAI_HAS_MCR */
}

void SAI_TxReset(I2S_Type *base)
{
    /* Set the software reset and FIFO reset to clear internal state */
    base->TCSR = I2S_TCSR_SR_MASK | I2S_TCSR_FR_MASK;

    /* Clear software reset bit, this should be done by software */
    base->TCSR &= ~I2S_TCSR_SR_MASK;

    /* Reset all Tx register values */
    base->TCR2 = 0;
    base->TCR3 = 0;
    base->TCR4 = 0;
    base->TCR5 = 0;
    base->TMR = 0;
}

void SAI_RxReset(I2S_Type *base)
{
    /* Set the software reset and FIFO reset to clear internal state */
    base->RCSR = I2S_RCSR_SR_MASK | I2S_RCSR_FR_MASK;

    /* Clear software reset bit, this should be done by software */
    base->RCSR &= ~I2S_RCSR_SR_MASK;

    /* Reset all Rx register values */
    base->RCR2 = 0;
    base->RCR3 = 0;
    base->RCR4 = 0;
    base->RCR5 = 0;
    base->RMR = 0;
}

void SAI_TxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        /* If clock is sync with Rx, should enable RE bit. */
        if (((base->TCR2 & I2S_TCR2_SYNC_MASK) >> I2S_TCR2_SYNC_SHIFT) == 0x1U)
        {
            base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | I2S_RCSR_RE_MASK);
        }
        base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | I2S_TCSR_TE_MASK);
        /* Also need to clear the FIFO error flag before start */
        SAI_TxClearStatusFlags(base, kSAI_FIFOErrorFlag);
    }
    else
    {
        /* If RE not sync with TE, than disable TE, otherwise, shall not disable TE */
        if (((base->RCR2 & I2S_RCR2_SYNC_MASK) >> I2S_RCR2_SYNC_SHIFT) != 0x1U)
        {
            /* Should not close RE even sync with Rx */
            base->TCSR = ((base->TCSR & 0xFFE3FFFFU) & (~I2S_TCSR_TE_MASK));
        }
    }
}

void SAI_RxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        /* If clock is sync with Tx, should enable TE bit. */
        if (((base->RCR2 & I2S_RCR2_SYNC_MASK) >> I2S_RCR2_SYNC_SHIFT) == 0x1U)
        {
            base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | I2S_TCSR_TE_MASK);
        }
        base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | I2S_RCSR_RE_MASK);
        /* Also need to clear the FIFO error flag before start */
        SAI_RxClearStatusFlags(base, kSAI_FIFOErrorFlag);
    }
    else
    {
        /* While TX is not sync with RX, close RX */
        if (((base->TCR2 & I2S_TCR2_SYNC_MASK) >> I2S_TCR2_SYNC_SHIFT) != 0x1U)
        {
            base->RCSR = ((base->RCSR & 0xFFE3FFFFU) & (~I2S_RCSR_RE_MASK));
        }
    }
}

void SAI_TxSoftwareReset(I2S_Type *base, sai_reset_type_t type)
{
    base->TCSR |= (uint32_t)type;

    /* Clear the software reset */
    base->TCSR &= ~I2S_TCSR_SR_MASK;
}

void SAI_RxSoftwareReset(I2S_Type *base, sai_reset_type_t type)
{
    base->RCSR |= (uint32_t)type;

    /* Clear the software reset */
    base->RCSR &= ~I2S_RCSR_SR_MASK;
}

void SAI_TxSetChannelFIFOMask(I2S_Type *base, uint8_t mask)
{
    base->TCR3 &= ~I2S_TCR3_TCE_MASK;
    base->TCR3 |= I2S_TCR3_TCE(mask);
}

void SAI_RxSetChannelFIFOMask(I2S_Type *base, uint8_t mask)
{
    base->RCR3 &= ~I2S_RCR3_RCE_MASK;
    base->RCR3 |= I2S_RCR3_RCE(mask);
}

void SAI_TxSetFormat(I2S_Type *base,
                     sai_transfer_format_t *format,
                     uint32_t mclkSourceClockHz,
                     uint32_t bclkSourceClockHz)
{
    uint32_t bclk = 0;
    uint32_t val = 0;
    uint32_t channels = 2U;

    if (format->stereo != kSAI_Stereo)
    {
        channels = 1U;
    }

    if (format->isFrameSyncCompact)
    {
        bclk = format->sampleRate_Hz * format->bitWidth * channels;
        val = (base->TCR4 & (~I2S_TCR4_SYWD_MASK));
        val |= I2S_TCR4_SYWD(format->bitWidth - 1U);
        base->TCR4 = val;
    }
    else
    {
        bclk = format->sampleRate_Hz * 32U * 2U;
    }

/* Compute the mclk */
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
    /* Check if master clock divider enabled, then set master clock divider */
    if (base->MCR & I2S_MCR_MOE_MASK)
    {
        SAI_SetMasterClockDivider(base, format->masterClockHz, mclkSourceClockHz);
    }
#endif /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */

    /* Set bclk if needed */
    if (base->TCR2 & I2S_TCR2_BCD_MASK)
    {
        base->TCR2 &= ~I2S_TCR2_DIV_MASK;
        base->TCR2 |= I2S_TCR2_DIV((bclkSourceClockHz / bclk) / 2U - 1U);
    }

    /* Set bitWidth */
    val = (format->isFrameSyncCompact) ? (format->bitWidth - 1) : 31U;
    if (format->protocol == kSAI_BusRightJustified)
    {
        base->TCR5 = I2S_TCR5_WNW(val) | I2S_TCR5_W0W(val) | I2S_TCR5_FBT(val);
    }
    else
    {
        base->TCR5 = I2S_TCR5_WNW(val) | I2S_TCR5_W0W(val) | I2S_TCR5_FBT(format->bitWidth - 1);
    }

    /* Set mono or stereo */
    base->TMR = (uint32_t)format->stereo;

    /* Set data channel */
    base->TCR3 &= ~I2S_TCR3_TCE_MASK;
    base->TCR3 |= I2S_TCR3_TCE(1U << format->channel);

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Set watermark */
    base->TCR1 = format->watermark;
#endif /* FSL_FEATURE_SAI_FIFO_COUNT  */
}

void SAI_RxSetFormat(I2S_Type *base,
                     sai_transfer_format_t *format,
                     uint32_t mclkSourceClockHz,
                     uint32_t bclkSourceClockHz)
{
    uint32_t bclk = 0;
    uint32_t val = 0;
    uint32_t channels = 2U;

    if (format->stereo != kSAI_Stereo)
    {
        channels = 1U;
    }

    if (format->isFrameSyncCompact)
    {
        bclk = format->sampleRate_Hz * format->bitWidth * channels;
        val = (base->RCR4 & (~I2S_RCR4_SYWD_MASK));
        val |= I2S_RCR4_SYWD(format->bitWidth - 1U);
        base->RCR4 = val;
    }
    else
    {
        bclk = format->sampleRate_Hz * 32U * 2U;
    }

/* Compute the mclk */
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
    /* Check if master clock divider enabled */
    if (base->MCR & I2S_MCR_MOE_MASK)
    {
        SAI_SetMasterClockDivider(base, format->masterClockHz, mclkSourceClockHz);
    }
#endif /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */

    /* Set bclk if needed */
    if (base->RCR2 & I2S_RCR2_BCD_MASK)
    {
        base->RCR2 &= ~I2S_RCR2_DIV_MASK;
        base->RCR2 |= I2S_RCR2_DIV((bclkSourceClockHz / bclk) / 2U - 1U);
    }

    /* Set bitWidth */
    val = (format->isFrameSyncCompact) ? (format->bitWidth - 1) : 31U;
    if (format->protocol == kSAI_BusRightJustified)
    {
        base->RCR5 = I2S_RCR5_WNW(val) | I2S_RCR5_W0W(val) | I2S_RCR5_FBT(val);
    }
    else
    {
        base->RCR5 = I2S_RCR5_WNW(val) | I2S_RCR5_W0W(val) | I2S_RCR5_FBT(format->bitWidth - 1);
    }

    /* Set mono or stereo */
    base->RMR = (uint32_t)format->stereo;

    /* Set data channel */
    base->RCR3 &= ~I2S_RCR3_RCE_MASK;
    base->RCR3 |= I2S_RCR3_RCE(1U << format->channel);

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Set watermark */
    base->RCR1 = format->watermark;
#endif /* FSL_FEATURE_SAI_FIFO_COUNT  */
}

void SAI_WriteBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i = 0;
    uint8_t bytesPerWord = bitWidth / 8U;

    while (i < size)
    {
        /* Wait until it can write data */
        while (!(base->TCSR & I2S_TCSR_FWF_MASK))
        {
        }

        SAI_WriteNonBlocking(base, channel, bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        i += bytesPerWord;
    }

    /* Wait until the last data is sent */
    while (!(base->TCSR & I2S_TCSR_FWF_MASK))
    {
    }
}

void SAI_ReadBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i = 0;
    uint8_t bytesPerWord = bitWidth / 8U;

    while (i < size)
    {
        /* Wait until data is received */
        while (!(base->RCSR & I2S_RCSR_FWF_MASK))
        {
        }

        SAI_ReadNonBlocking(base, channel, bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        i += bytesPerWord;
    }
}

void SAI_TransferTxCreateHandle(I2S_Type *base, sai_handle_t *handle, sai_transfer_callback_t callback, void *userData)
{
    assert(handle);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    s_saiHandle[SAI_GetInstance(base)][0] = handle;

    handle->callback = callback;
    handle->userData = userData;

    /* Set the isr pointer */
    s_saiTxIsr = SAI_TransferTxHandleIRQ;

    /* Enable Tx irq */
    EnableIRQ(s_saiTxIRQ[SAI_GetInstance(base)]);
}

void SAI_TransferRxCreateHandle(I2S_Type *base, sai_handle_t *handle, sai_transfer_callback_t callback, void *userData)
{
    assert(handle);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    s_saiHandle[SAI_GetInstance(base)][1] = handle;

    handle->callback = callback;
    handle->userData = userData;

    /* Set the isr pointer */
    s_saiRxIsr = SAI_TransferRxHandleIRQ;

    /* Enable Rx irq */
    EnableIRQ(s_saiRxIRQ[SAI_GetInstance(base)]);
}

status_t SAI_TransferTxSetFormat(I2S_Type *base,
                                 sai_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz)
{
    assert(handle);

    if ((mclkSourceClockHz < format->sampleRate_Hz) || (bclkSourceClockHz < format->sampleRate_Hz))
    {
        return kStatus_InvalidArgument;
    }

    /* Copy format to handle */
    handle->bitWidth = format->bitWidth;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    handle->watermark = format->watermark;
#endif
    handle->channel = format->channel;

    SAI_TxSetFormat(base, format, mclkSourceClockHz, bclkSourceClockHz);

    return kStatus_Success;
}

status_t SAI_TransferRxSetFormat(I2S_Type *base,
                                 sai_handle_t *handle,
                                 sai_transfer_format_t *format,
                                 uint32_t mclkSourceClockHz,
                                 uint32_t bclkSourceClockHz)
{
    assert(handle);

    if ((mclkSourceClockHz < format->sampleRate_Hz) || (bclkSourceClockHz < format->sampleRate_Hz))
    {
        return kStatus_InvalidArgument;
    }

    /* Copy format to handle */
    handle->bitWidth = format->bitWidth;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    handle->watermark = format->watermark;
#endif
    handle->channel = format->channel;

    SAI_RxSetFormat(base, format, mclkSourceClockHz, bclkSourceClockHz);

    return kStatus_Success;
}

status_t SAI_TransferSendNonBlocking(I2S_Type *base, sai_handle_t *handle, sai_transfer_t *xfer)
{
    assert(handle);

    /* Check if the queue is full */
    if (handle->saiQueue[handle->queueUser].data)
    {
        return kStatus_SAI_QueueFull;
    }

    /* Add into queue */
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->saiQueue[handle->queueUser].data = xfer->data;
    handle->saiQueue[handle->queueUser].dataSize = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % SAI_XFER_QUEUE_SIZE;

    /* Set the state to busy */
    handle->state = kSAI_Busy;

/* Enable interrupt */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Use FIFO request interrupt and fifo error*/
    SAI_TxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFORequestInterruptEnable);
#else
    SAI_TxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFOWarningInterruptEnable);
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* Enable Tx transfer */
    SAI_TxEnable(base, true);

    return kStatus_Success;
}

status_t SAI_TransferReceiveNonBlocking(I2S_Type *base, sai_handle_t *handle, sai_transfer_t *xfer)
{
    assert(handle);

    /* Check if the queue is full */
    if (handle->saiQueue[handle->queueUser].data)
    {
        return kStatus_SAI_QueueFull;
    }

    /* Add into queue */
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->saiQueue[handle->queueUser].data = xfer->data;
    handle->saiQueue[handle->queueUser].dataSize = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % SAI_XFER_QUEUE_SIZE;

    /* Set state to busy */
    handle->state = kSAI_Busy;

/* Enable interrupt */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Use FIFO request interrupt and fifo error*/
    SAI_RxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFORequestInterruptEnable);
#else
    SAI_RxEnableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFOWarningInterruptEnable);
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* Enable Rx transfer */
    SAI_RxEnable(base, true);

    return kStatus_Success;
}

status_t SAI_TransferGetSendCount(I2S_Type *base, sai_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kSAI_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = (handle->transferSize[handle->queueDriver] - handle->saiQueue[handle->queueDriver].dataSize);
    }

    return status;
}

status_t SAI_TransferGetReceiveCount(I2S_Type *base, sai_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kSAI_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = (handle->transferSize[handle->queueDriver] - handle->saiQueue[handle->queueDriver].dataSize);
    }

    return status;
}

void SAI_TransferAbortSend(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    /* Stop Tx transfer and disable interrupt */
    SAI_TxEnable(base, false);
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Use FIFO request interrupt and fifo error */
    SAI_TxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFORequestInterruptEnable);
#else
    SAI_TxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFOWarningInterruptEnable);
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    handle->state = kSAI_Idle;

    /* Clear the queue */
    memset(handle->saiQueue, 0, sizeof(sai_transfer_t) * SAI_XFER_QUEUE_SIZE);
    handle->queueDriver = 0;
    handle->queueUser = 0;
}

void SAI_TransferAbortReceive(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    /* Stop Tx transfer and disable interrupt */
    SAI_RxEnable(base, false);
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* Use FIFO request interrupt and fifo error */
    SAI_RxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFORequestInterruptEnable);
#else
    SAI_RxDisableInterrupts(base, kSAI_FIFOErrorInterruptEnable | kSAI_FIFOWarningInterruptEnable);
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    handle->state = kSAI_Idle;

    /* Clear the queue */
    memset(handle->saiQueue, 0, sizeof(sai_transfer_t) * SAI_XFER_QUEUE_SIZE);
    handle->queueDriver = 0;
    handle->queueUser = 0;
}

void SAI_TransferTerminateSend(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    /* Abort the current transfer */
    SAI_TransferAbortSend(base, handle);

    /* Clear all the internal information */
    memset(handle->saiQueue, 0U, sizeof(handle->saiQueue));
    memset(handle->transferSize, 0U, sizeof(handle->transferSize));
    handle->queueUser = 0U;
    handle->queueDriver = 0U;
}

void SAI_TransferTerminateReceive(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    /* Abort the current transfer */
    SAI_TransferAbortReceive(base, handle);

    /* Clear all the internal information */
    memset(handle->saiQueue, 0U, sizeof(handle->saiQueue));
    memset(handle->transferSize, 0U, sizeof(handle->transferSize));
    handle->queueUser = 0U;
    handle->queueDriver = 0U;
}

void SAI_TransferTxHandleIRQ(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    uint8_t *buffer = handle->saiQueue[handle->queueDriver].data;
    uint8_t dataSize = handle->bitWidth / 8U;

    /* Handle Error */
    if (base->TCSR & I2S_TCSR_FEF_MASK)
    {
        /* Clear FIFO error flag to continue transfer */
        SAI_TxClearStatusFlags(base, kSAI_FIFOErrorFlag);

        /* Reset FIFO for safety */
        SAI_TxSoftwareReset(base, kSAI_ResetTypeFIFO);

        /* Call the callback */
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_SAI_TxError, handle->userData);
        }
    }

/* Handle transfer */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if (base->TCSR & I2S_TCSR_FRF_MASK)
    {
        /* Judge if the data need to transmit is less than space */
        uint8_t size = MIN((handle->saiQueue[handle->queueDriver].dataSize),
                           (size_t)((FSL_FEATURE_SAI_FIFO_COUNT - handle->watermark) * dataSize));

        /* Copy the data from sai buffer to FIFO */
        SAI_WriteNonBlocking(base, handle->channel, handle->bitWidth, buffer, size);

        /* Update the internal counter */
        handle->saiQueue[handle->queueDriver].dataSize -= size;
        handle->saiQueue[handle->queueDriver].data += size;
    }
#else
    if (base->TCSR & I2S_TCSR_FWF_MASK)
    {
        uint8_t size = MIN((handle->saiQueue[handle->queueDriver].dataSize), dataSize);

        SAI_WriteNonBlocking(base, handle->channel, handle->bitWidth, buffer, size);

        /* Update internal counter */
        handle->saiQueue[handle->queueDriver].dataSize -= size;
        handle->saiQueue[handle->queueDriver].data += size;
    }
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* If finished a blcok, call the callback function */
    if (handle->saiQueue[handle->queueDriver].dataSize == 0U)
    {
        memset(&handle->saiQueue[handle->queueDriver], 0, sizeof(sai_transfer_t));
        handle->queueDriver = (handle->queueDriver + 1) % SAI_XFER_QUEUE_SIZE;
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_SAI_TxIdle, handle->userData);
        }
    }

    /* If all data finished, just stop the transfer */
    if (handle->saiQueue[handle->queueDriver].data == NULL)
    {
        SAI_TransferAbortSend(base, handle);
    }
}

void SAI_TransferRxHandleIRQ(I2S_Type *base, sai_handle_t *handle)
{
    assert(handle);

    uint8_t *buffer = handle->saiQueue[handle->queueDriver].data;
    uint8_t dataSize = handle->bitWidth / 8U;

    /* Handle Error */
    if (base->RCSR & I2S_RCSR_FEF_MASK)
    {
        /* Clear FIFO error flag to continue transfer */
        SAI_RxClearStatusFlags(base, kSAI_FIFOErrorFlag);

        /* Reset FIFO for safety */
        SAI_RxSoftwareReset(base, kSAI_ResetTypeFIFO);

        /* Call the callback */
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_SAI_RxError, handle->userData);
        }
    }

/* Handle transfer */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if (base->RCSR & I2S_RCSR_FRF_MASK)
    {
        /* Judge if the data need to transmit is less than space */
        uint8_t size = MIN((handle->saiQueue[handle->queueDriver].dataSize), (handle->watermark * dataSize));

        /* Copy the data from sai buffer to FIFO */
        SAI_ReadNonBlocking(base, handle->channel, handle->bitWidth, buffer, size);

        /* Update the internal counter */
        handle->saiQueue[handle->queueDriver].dataSize -= size;
        handle->saiQueue[handle->queueDriver].data += size;
    }
#else
    if (base->RCSR & I2S_RCSR_FWF_MASK)
    {
        uint8_t size = MIN((handle->saiQueue[handle->queueDriver].dataSize), dataSize);

        SAI_ReadNonBlocking(base, handle->channel, handle->bitWidth, buffer, size);

        /* Update internal state */
        handle->saiQueue[handle->queueDriver].dataSize -= size;
        handle->saiQueue[handle->queueDriver].data += size;
    }
#endif /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* If finished a blcok, call the callback function */
    if (handle->saiQueue[handle->queueDriver].dataSize == 0U)
    {
        memset(&handle->saiQueue[handle->queueDriver], 0, sizeof(sai_transfer_t));
        handle->queueDriver = (handle->queueDriver + 1) % SAI_XFER_QUEUE_SIZE;
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_SAI_RxIdle, handle->userData);
        }
    }

    /* If all data finished, just stop the transfer */
    if (handle->saiQueue[handle->queueDriver].data == NULL)
    {
        SAI_TransferAbortReceive(base, handle);
    }
}

#if defined(I2S0)
void I2S0_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][1]) && ((I2S0->RCSR & kSAI_FIFORequestFlag) || (I2S0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S0->RCSR & kSAI_FIFORequestInterruptEnable) || (I2S0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][1]) && ((I2S0->RCSR & kSAI_FIFOWarningFlag) || (I2S0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S0->RCSR & kSAI_FIFOWarningInterruptEnable) || (I2S0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(I2S0, s_saiHandle[0][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][0]) && ((I2S0->TCSR & kSAI_FIFORequestFlag) || (I2S0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S0->TCSR & kSAI_FIFORequestInterruptEnable) || (I2S0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][0]) && ((I2S0->TCSR & kSAI_FIFOWarningFlag) || (I2S0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S0->TCSR & kSAI_FIFOWarningInterruptEnable) || (I2S0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(I2S0, s_saiHandle[0][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S0_Tx_DriverIRQHandler(void)
{
    assert(s_saiHandle[0][0]);
    s_saiTxIsr(I2S0, s_saiHandle[0][0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S0_Rx_DriverIRQHandler(void)
{
    assert(s_saiHandle[0][1]);
    s_saiRxIsr(I2S0, s_saiHandle[0][1]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* I2S0*/

#if defined(I2S1)
void I2S1_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][1]) && ((I2S1->RCSR & kSAI_FIFORequestFlag) || (I2S1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S1->RCSR & kSAI_FIFORequestInterruptEnable) || (I2S1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][1]) && ((I2S1->RCSR & kSAI_FIFOWarningFlag) || (I2S1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S1->RCSR & kSAI_FIFOWarningInterruptEnable) || (I2S1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(I2S1, s_saiHandle[1][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][0]) && ((I2S1->TCSR & kSAI_FIFORequestFlag) || (I2S1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S1->TCSR & kSAI_FIFORequestInterruptEnable) || (I2S1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][0]) && ((I2S1->TCSR & kSAI_FIFOWarningFlag) || (I2S1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S1->TCSR & kSAI_FIFOWarningInterruptEnable) || (I2S1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(I2S1, s_saiHandle[1][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S1_Tx_DriverIRQHandler(void)
{
    assert(s_saiHandle[1][0]);
    s_saiTxIsr(I2S1, s_saiHandle[1][0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S1_Rx_DriverIRQHandler(void)
{
    assert(s_saiHandle[1][1]);
    s_saiRxIsr(I2S1, s_saiHandle[1][1]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* I2S1*/

#if defined(I2S2)
void I2S2_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][1]) && ((I2S2->RCSR & kSAI_FIFORequestFlag) || (I2S2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S2->RCSR & kSAI_FIFORequestInterruptEnable) || (I2S2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][1]) && ((I2S2->RCSR & kSAI_FIFOWarningFlag) || (I2S2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S2->RCSR & kSAI_FIFOWarningInterruptEnable) || (I2S2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(I2S2, s_saiHandle[2][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][0]) && ((I2S2->TCSR & kSAI_FIFORequestFlag) || (I2S2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S2->TCSR & kSAI_FIFORequestInterruptEnable) || (I2S2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][0]) && ((I2S2->TCSR & kSAI_FIFOWarningFlag) || (I2S2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S2->TCSR & kSAI_FIFOWarningInterruptEnable) || (I2S2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(I2S2, s_saiHandle[2][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S2_Tx_DriverIRQHandler(void)
{
    assert(s_saiHandle[2][0]);
    s_saiTxIsr(I2S2, s_saiHandle[2][0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S2_Rx_DriverIRQHandler(void)
{
    assert(s_saiHandle[2][1]);
    s_saiRxIsr(I2S2, s_saiHandle[2][1]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* I2S2*/

#if defined(I2S3)
void I2S3_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][1]) && ((I2S3->RCSR & kSAI_FIFORequestFlag) || (I2S3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S3->RCSR & kSAI_FIFORequestInterruptEnable) || (I2S3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][1]) && ((I2S3->RCSR & kSAI_FIFOWarningFlag) || (I2S3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S3->RCSR & kSAI_FIFOWarningInterruptEnable) || (I2S3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(I2S3, s_saiHandle[3][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][0]) && ((I2S3->TCSR & kSAI_FIFORequestFlag) || (I2S3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S3->TCSR & kSAI_FIFORequestInterruptEnable) || (I2S3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][0]) && ((I2S3->TCSR & kSAI_FIFOWarningFlag) || (I2S3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((I2S3->TCSR & kSAI_FIFOWarningInterruptEnable) || (I2S3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(I2S3, s_saiHandle[3][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S3_Tx_DriverIRQHandler(void)
{
    assert(s_saiHandle[3][0]);
    s_saiTxIsr(I2S3, s_saiHandle[3][0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void I2S3_Rx_DriverIRQHandler(void)
{
    assert(s_saiHandle[3][1]);
    s_saiRxIsr(I2S3, s_saiHandle[3][1]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* I2S3*/

#if defined(AUDIO__SAI0)
void AUDIO_SAI0_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][1]) &&
        ((AUDIO__SAI0->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI0->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][1]) &&
        ((AUDIO__SAI0->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI0->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI0, s_saiHandle[0][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][0]) &&
        ((AUDIO__SAI0->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI0->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][0]) &&
        ((AUDIO__SAI0->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI0->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI0, s_saiHandle[0][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* AUDIO__SAI0 */

#if defined(AUDIO__SAI1)
void AUDIO_SAI1_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][1]) &&
        ((AUDIO__SAI1->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI1->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][1]) &&
        ((AUDIO__SAI1->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI1->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI1, s_saiHandle[1][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][0]) &&
        ((AUDIO__SAI1->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI1->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][0]) &&
        ((AUDIO__SAI1->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI1->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI1, s_saiHandle[1][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* AUDIO__SAI1 */

#if defined(AUDIO__SAI2)
void AUDIO_SAI2_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][1]) &&
        ((AUDIO__SAI2->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI2->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][1]) &&
        ((AUDIO__SAI2->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI2->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI2, s_saiHandle[2][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][0]) &&
        ((AUDIO__SAI2->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI2->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][0]) &&
        ((AUDIO__SAI2->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI2->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI2, s_saiHandle[2][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* AUDIO__SAI2 */

#if defined(AUDIO__SAI3)
void AUDIO_SAI3_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][1]) &&
        ((AUDIO__SAI3->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI3->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][1]) &&
        ((AUDIO__SAI3->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI3->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI3, s_saiHandle[3][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][0]) &&
        ((AUDIO__SAI3->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI3->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][0]) &&
        ((AUDIO__SAI3->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI3->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI3, s_saiHandle[3][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(AUDIO__SAI6)
void AUDIO_SAI6_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[6][1]) &&
        ((AUDIO__SAI6->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI6->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI6->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI6->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[6][1]) &&
        ((AUDIO__SAI6->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI6->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI6->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI6->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI6, s_saiHandle[6][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[6][0]) &&
        ((AUDIO__SAI6->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI6->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI6->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI6->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[6][0]) &&
        ((AUDIO__SAI6->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI6->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI6->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI6->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI6, s_saiHandle[6][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* AUDIO__SAI6 */

#if defined(AUDIO__SAI7)
void AUDIO_SAI7_INT_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[7][1]) &&
        ((AUDIO__SAI7->RCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI7->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI7->RCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI7->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[7][1]) &&
        ((AUDIO__SAI7->RCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI7->RCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI7->RCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI7->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(AUDIO__SAI7, s_saiHandle[7][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[7][0]) &&
        ((AUDIO__SAI7->TCSR & kSAI_FIFORequestFlag) || (AUDIO__SAI7->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI7->TCSR & kSAI_FIFORequestInterruptEnable) || (AUDIO__SAI7->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[7][0]) &&
        ((AUDIO__SAI7->TCSR & kSAI_FIFOWarningFlag) || (AUDIO__SAI7->TCSR & kSAI_FIFOErrorFlag)) &&
        ((AUDIO__SAI7->TCSR & kSAI_FIFOWarningInterruptEnable) || (AUDIO__SAI7->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI7, s_saiHandle[7][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* AUDIO__SAI7 */

#if defined(SAI0)
void SAI0_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][1]) && ((SAI0->RCSR & kSAI_FIFORequestFlag) || (SAI0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI0->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][1]) && ((SAI0->RCSR & kSAI_FIFOWarningFlag) || (SAI0->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI0->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI0->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI0, s_saiHandle[0][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[0][0]) && ((SAI0->TCSR & kSAI_FIFORequestFlag) || (SAI0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI0->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[0][0]) && ((SAI0->TCSR & kSAI_FIFOWarningFlag) || (SAI0->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI0->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI0->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(AUDIO__SAI0, s_saiHandle[0][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI0 */

#if defined(SAI1)
void SAI1_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][1]) && ((SAI1->RCSR & kSAI_FIFORequestFlag) || (SAI1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI1->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][1]) && ((SAI1->RCSR & kSAI_FIFOWarningFlag) || (SAI1->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI1->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI1->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI1, s_saiHandle[1][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[1][0]) && ((SAI1->TCSR & kSAI_FIFORequestFlag) || (SAI1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI1->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[1][0]) && ((SAI1->TCSR & kSAI_FIFOWarningFlag) || (SAI1->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI1->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI1->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI1, s_saiHandle[1][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI1 */

#if defined(SAI2)
void SAI2_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][1]) && ((SAI2->RCSR & kSAI_FIFORequestFlag) || (SAI2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI2->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][1]) && ((SAI2->RCSR & kSAI_FIFOWarningFlag) || (SAI2->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI2->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI2->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI2, s_saiHandle[2][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[2][0]) && ((SAI2->TCSR & kSAI_FIFORequestFlag) || (SAI2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI2->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[2][0]) && ((SAI2->TCSR & kSAI_FIFOWarningFlag) || (SAI2->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI2->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI2->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI2, s_saiHandle[2][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI2 */

#if defined(SAI3)
void SAI3_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][1]) && ((SAI3->RCSR & kSAI_FIFORequestFlag) || (SAI3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI3->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][1]) && ((SAI3->RCSR & kSAI_FIFOWarningFlag) || (SAI3->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI3->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI3->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI3, s_saiHandle[3][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[3][0]) && ((SAI3->TCSR & kSAI_FIFORequestFlag) || (SAI3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI3->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[3][0]) && ((SAI3->TCSR & kSAI_FIFOWarningFlag) || (SAI3->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI3->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI3->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI3, s_saiHandle[3][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI3 */

#if defined(SAI4)
void SAI4_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[4][1]) && ((SAI4->RCSR & kSAI_FIFORequestFlag) || (SAI4->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI4->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI4->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[4][1]) && ((SAI4->RCSR & kSAI_FIFOWarningFlag) || (SAI4->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI4->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI4->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI4, s_saiHandle[4][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[4][0]) && ((SAI4->TCSR & kSAI_FIFORequestFlag) || (SAI4->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI4->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI4->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[4][0]) && ((SAI4->TCSR & kSAI_FIFOWarningFlag) || (SAI4->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI4->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI4->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI4, s_saiHandle[4][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI4 */

#if defined(SAI5)
void SAI5_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[5][1]) && ((SAI5->RCSR & kSAI_FIFORequestFlag) || (SAI5->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI5->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI5->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[5][1]) && ((SAI5->RCSR & kSAI_FIFOWarningFlag) || (SAI5->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI5->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI5->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI5, s_saiHandle[5][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[5][0]) && ((SAI5->TCSR & kSAI_FIFORequestFlag) || (SAI5->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI5->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI5->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[5][0]) && ((SAI5->TCSR & kSAI_FIFOWarningFlag) || (SAI5->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI5->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI5->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI5, s_saiHandle[5][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI5 */

#if defined(SAI6)
void SAI6_DriverIRQHandler(void)
{
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[6][1]) && ((SAI6->RCSR & kSAI_FIFORequestFlag) || (SAI6->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI6->RCSR & kSAI_FIFORequestInterruptEnable) || (SAI6->RCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[6][1]) && ((SAI6->RCSR & kSAI_FIFOWarningFlag) || (SAI6->RCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI6->RCSR & kSAI_FIFOWarningInterruptEnable) || (SAI6->RCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiRxIsr(SAI6, s_saiHandle[6][1]);
    }
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    if ((s_saiHandle[6][0]) && ((SAI6->TCSR & kSAI_FIFORequestFlag) || (SAI6->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI6->TCSR & kSAI_FIFORequestInterruptEnable) || (SAI6->TCSR & kSAI_FIFOErrorInterruptEnable)))
#else
    if ((s_saiHandle[6][0]) && ((SAI6->TCSR & kSAI_FIFOWarningFlag) || (SAI6->TCSR & kSAI_FIFOErrorFlag)) &&
        ((SAI6->TCSR & kSAI_FIFOWarningInterruptEnable) || (SAI6->TCSR & kSAI_FIFOErrorInterruptEnable)))
#endif
    {
        s_saiTxIsr(SAI6, s_saiHandle[6][0]);
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif /* SAI6 */
