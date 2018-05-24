/*
 * The Clear BSD License
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
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
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

#include "fsl_bee.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

static void aligned_memcpy(void *dst, const void *src, size_t size)
{
    register uint32_t *to32 = (uint32_t *)(uintptr_t)dst;
    register const uint32_t *from32 = (const uint32_t *)(uintptr_t)src;

    while (size >= sizeof(uint32_t))
    {
        *to32 = *from32;
        size -= sizeof(uint32_t);
        to32++;
        from32++;
    }
}

void BEE_Init(BEE_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Bee);
#endif

    base->CTRL = BEE_CTRL_CTRL_SFTRST_N_MASK | BEE_CTRL_CTRL_CLK_EN_MASK;
}

void BEE_Deinit(BEE_Type *base)
{
    base->CTRL &=
        ~(BEE_CTRL_BEE_ENABLE_MASK | BEE_CTRL_CTRL_SFTRST_N_MASK | BEE_CTRL_CTRL_CLK_EN_MASK | BEE_CTRL_KEY_VALID_MASK);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Bee);
#endif
}

void BEE_GetDefaultConfig(bee_region_config_t *config)
{
    assert(config);

    config->mode = kBEE_AesEcbMode;
    config->regionBot = 0U;
    config->regionTop = 0U;
    config->addrOffset = 0xF0000000U;
    config->regionEn = kBEE_RegionDisabled;
}

status_t BEE_SetRegionConfig(BEE_Type *base, bee_region_t region, const bee_region_config_t *config)
{
    IOMUXC_GPR_Type *iomuxc = IOMUXC_GPR;
    bool reenable = false;

    /* Wait until BEE is in idle state */
    while (!(BEE_GetStatusFlags(base) & kBEE_IdleFlag))
    {
    }

    /* Disable BEE before region configuration in case it is enabled. */
    if ((base->CTRL >> BEE_CTRL_BEE_ENABLE_SHIFT) & 1)
    {
        BEE_Disable(base);
        reenable = true;
    }

    if (region == kBEE_Region0)
    {
        /* Region 0 config */
        iomuxc->GPR18 = config->regionBot;
        iomuxc->GPR19 = config->regionTop;

        base->CTRL |= BEE_CTRL_CTRL_AES_MODE_R0(config->mode);
        base->ADDR_OFFSET0 = BEE_ADDR_OFFSET0_ADDR_OFFSET0(config->addrOffset);
    }

    else if (region == kBEE_Region1)
    {
        /* Region 1 config */
        iomuxc->GPR20 = config->regionBot;
        iomuxc->GPR21 = config->regionTop;

        base->CTRL |= BEE_CTRL_CTRL_AES_MODE_R1(config->mode);
        base->ADDR_OFFSET1 = BEE_ADDR_OFFSET1_ADDR_OFFSET0(config->addrOffset);
        base->REGION1_BOT = BEE_REGION1_BOT_REGION1_BOT(config->regionBot);
        base->REGION1_TOP = BEE_REGION1_TOP_REGION1_TOP(config->regionTop);
    }

    else
    {
        return kStatus_InvalidArgument;
    }

    /* Enable/disable region if desired */
    if (config->regionEn == kBEE_RegionEnabled)
    {
        iomuxc->GPR11 |= IOMUXC_GPR_GPR11_BEE_DE_RX_EN(1 << region);
    }
    else
    {
        iomuxc->GPR11 &= ~IOMUXC_GPR_GPR11_BEE_DE_RX_EN(1 << region);
    }

    /* Reenable BEE if it was enabled before. */
    if (reenable)
    {
        BEE_Enable(base);
    }

    return kStatus_Success;
}

status_t BEE_SetRegionKey(
    BEE_Type *base, bee_region_t region, const uint8_t *key, size_t keySize, const uint8_t *nonce, size_t nonceSize)
{
    bool reenable = false;

    /* Key must be 32-bit aligned */
    if (((uintptr_t)key & 0x3u) || (keySize != 16))
    {
        return kStatus_InvalidArgument;
    }

    /* Wait until BEE is in idle state */
    while (!(BEE_GetStatusFlags(base) & kBEE_IdleFlag))
    {
    }

    /* Disable BEE before region configuration in case it is enabled. */
    if ((base->CTRL >> BEE_CTRL_BEE_ENABLE_SHIFT) & 1)
    {
        BEE_Disable(base);
        reenable = true;
    }

    if (region == kBEE_Region0)
    {
        base->CTRL &= ~BEE_CTRL_KEY_REGION_SEL_MASK;

        if (nonce)
        {
            if (nonceSize != 16)
            {
                return kStatus_InvalidArgument;
            }
            memcpy((uint32_t *)&base->CTR_NONCE0_W0, nonce, nonceSize);
        }
    }

    else if (region == kBEE_Region1)
    {
        base->CTRL |= BEE_CTRL_KEY_REGION_SEL_MASK;

        if (nonce)
        {
            if (nonceSize != 16)
            {
                return kStatus_InvalidArgument;
            }
            memcpy((uint32_t *)&base->CTR_NONCE1_W0, nonce, nonceSize);
        }
    }

    else
    {
        return kStatus_InvalidArgument;
    }

    /* Try to load key. If BEE key selection fuse is programmed to use OTMP key on this device, this operation should
     * fail. */
    aligned_memcpy((uint32_t *)&base->AES_KEY0_W0, key, keySize);
    if (memcmp((uint32_t *)&base->AES_KEY0_W0, key, keySize) != 0)
    {
        return kStatus_Fail;
    }

    /* Reenable BEE if it was enabled before. */
    if (reenable)
    {
        BEE_Enable(base);
    }

    return kStatus_Success;
}

uint32_t BEE_GetStatusFlags(BEE_Type *base)
{
    return base->STATUS;
}

void BEE_ClearStatusFlags(BEE_Type *base, uint32_t mask)
{
    /* w1c */
    base->STATUS |= mask;
}
