/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_bee.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.bee"
#endif


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

    config->region0Mode = kBEE_AesCtrMode;
    config->region1Mode = kBEE_AesCtrMode;
    config->region0AddrOffset = 0U;
    config->region1AddrOffset = 0U;
    config->region0SecLevel = kBEE_SecurityLevel3;
    config->region1SecLevel = kBEE_SecurityLevel3;
    config->region1Bot = 0U;
    config->region1Top = 0U;
    config->accessPermission = kBEE_AccessProtDisabled;
    config->endianSwapEn = kBEE_EndianSwapEnabled;
}

void BEE_SetConfig(BEE_Type *base, const bee_region_config_t *config)
{
    uint32_t beeCtrlVal;
    bool reenable = false;

    /* Wait until BEE is in idle state */
    while (!(BEE_GetStatusFlags(base) & kBEE_IdleFlag))
    {
    }

    /* Disable BEE before region configuration in case it is enabled. */
    if (base->CTRL & BEE_CTRL_BEE_ENABLE_MASK)
    {
        BEE_Disable(base);
        reenable = true;
    }

    /* Preserve CTRL bit values that are not set by this function */
    beeCtrlVal = base->CTRL & 0xFFFF0037;

    /* Set variable according to configuration */
    beeCtrlVal |= BEE_CTRL_AC_PROT_EN(config->accessPermission) | BEE_CTRL_LITTLE_ENDIAN(config->endianSwapEn) |\
                  BEE_CTRL_SECURITY_LEVEL_R0(config->region0SecLevel) | BEE_CTRL_CTRL_AES_MODE_R0(config->region0Mode) |\
                  BEE_CTRL_SECURITY_LEVEL_R1(config->region1SecLevel) | BEE_CTRL_CTRL_AES_MODE_R1(config->region1Mode);

    /* Load values to registers */
    base->CTRL = beeCtrlVal;
    base->ADDR_OFFSET0 = config->region0AddrOffset;
    base->ADDR_OFFSET1 = config->region1AddrOffset;
    base->REGION1_BOT = config->region1Bot;
    base->REGION1_TOP = config->region1Top;

    /* Reenable BEE if it was enabled before. */
    if (reenable)
    {
        BEE_Enable(base);
    }
}

status_t BEE_SetRegionKey(
    BEE_Type *base, bee_region_t region, const uint8_t *key, size_t keySize)
{
    bool redisable = false;

    /* Key must be 32-bit aligned */
    if (((uintptr_t)key & 0x3u) || (keySize != 16))
    {
        return kStatus_InvalidArgument;
    }

    /* Wait until BEE is in idle state */
    while (!(BEE_GetStatusFlags(base) & kBEE_IdleFlag))
    {
    }

    /* Clear KEY_VALID bit before new key is loaded */
    base->CTRL &= ~BEE_CTRL_KEY_VALID_MASK;

    /* Write key registers, key is stored in little-endian format in memory */
    aligned_memcpy((uint32_t *)&base->AES_KEY0_W0, key, keySize);

    /* Enable BEE before key configuration. */
    if (!(base->CTRL & BEE_CTRL_BEE_ENABLE_MASK))
    {
        BEE_Enable(base);
        redisable = true;
    }

    if (region == kBEE_Region0)
    {
        base->CTRL &= ~BEE_CTRL_KEY_REGION_SEL_MASK;
    }

    else if (region == kBEE_Region1)
    {
        base->CTRL |= BEE_CTRL_KEY_REGION_SEL_MASK;
    }

    else
    {
        return kStatus_InvalidArgument;
    }

    /* Set KEY_VALID bit to trigger key loading */
    base->CTRL |= BEE_CTRL_KEY_VALID_MASK;
    /* Wait until key is ready */
    while (!(base->CTRL & BEE_CTRL_KEY_VALID_MASK))
    {
    }

    /* Redisable BEE if it was disabled before this function call. */
    if (redisable)
    {
        BEE_Disable(base);
    }

    return kStatus_Success;
}

status_t BEE_SetRegionNonce(
    BEE_Type *base, bee_region_t region, const uint8_t *nonce, size_t nonceSize)
{
    /* Nonce must be 32-bit aligned */
    if (((uintptr_t)nonce & 0x3u) || (nonceSize != 16))
    {
        return kStatus_InvalidArgument;
    }

    /* Wait until BEE is in idle state */
    while (!(BEE_GetStatusFlags(base) & kBEE_IdleFlag))
    {
    }

    /* Write nonce registers, nonce is stored in little-endian format in memory */
    if (region == kBEE_Region0)
    {
        aligned_memcpy((uint32_t *)&base->CTR_NONCE0_W0, nonce, nonceSize);
    }

    else if (region == kBEE_Region1)
    {
        aligned_memcpy((uint32_t *)&base->CTR_NONCE1_W0, nonce, nonceSize);
    }

    else
    {
        return kStatus_InvalidArgument;
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
