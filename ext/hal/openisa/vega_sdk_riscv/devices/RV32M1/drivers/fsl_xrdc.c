/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_xrdc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define XRDC_DERR_W1_EST_VAL(w1) ((w1 & XRDC_DERR_W_EST_MASK) >> XRDC_DERR_W_EST_SHIFT)
#define XRDC_DERR_W1_EPORT_VAL(w1) ((w1 & XRDC_DERR_W_EPORT_MASK) >> XRDC_DERR_W_EPORT_SHIFT)
#define XRDC_DERR_W1_ERW_VAL(w1) ((w1 & XRDC_DERR_W_ERW_MASK) >> XRDC_DERR_W_ERW_SHIFT)
#define XRDC_DERR_W1_EATR_VAL(w1) ((w1 & XRDC_DERR_W_EATR_MASK) >> XRDC_DERR_W_EATR_SHIFT)
#define XRDC_DERR_W1_EDID_VAL(w1) ((w1 & XRDC_DERR_W_EDID_MASK) >> XRDC_DERR_W_EDID_SHIFT)

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_DXACP) && FSL_FEATURE_XRDC_NO_MRGD_DXACP)
#define XRDC_MRGD_DXACP_WIDTH (3U) /* The width of XRDC_MRDG_DxACP. */
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_DXSEL) && FSL_FEATURE_XRDC_HAS_MRGD_DXSEL)
#define XRDC_MRGD_DXSEL_WIDTH (3U) /* The width of XRDC_MRDG_DxSEL. */
#endif
#define XRDC_PDAC_DXACP_WIDTH (3U) /* The width of XRDC_PDAC_DxACP. */

/* For the force exclusive accesss lock release procedure. */
#define XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL1 (0x02000046U) /* The width of XRDC_MRDG_DxACP. */
#define XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL2 (0x02000052U) /* The width of XRDC_PDAC_DxACP. */

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Clock name of XRDC. */
#if (FSL_CLOCK_XRDC_GATE_COUNT > 1)
static const clock_ip_name_t s_xrdcClock[] = XRDC_CLOCKS;
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/

#if (((__CORTEX_M == 0U) && (defined(__ICCARM__))) || defined(__riscv))
/*!
 * @brief Count the leading zeros.
 *
 * Count the leading zeros of an 32-bit data. This function is only defined
 * for CM0 and CM0+ for IAR, because other cortex series have the clz instruction,
 * KEIL and ARMGCC have toolchain build in function for this purpose.
 *
 * @param data The data to process.
 * @return Count of the leading zeros.
 */
static uint32_t XRDC_CountLeadingZeros(uint32_t data)
{
    uint32_t count = 0U;
    uint32_t mask = 0x80000000U;

    while ((data & mask) == 0U)
    {
        count++;
        mask >>= 1U;
    }

    return count;
}
#endif

void XRDC_Init(XRDC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

#if FSL_CLOCK_XRDC_GATE_COUNT
#if (FSL_CLOCK_XRDC_GATE_COUNT == 1)
    CLOCK_EnableClock(kCLOCK_Xrdc0);
#else
    uint8_t i;

    for (i = 0; i < ARRAY_SIZE(s_xrdcClock); i++)
    {
        CLOCK_EnableClock(s_xrdcClock[i]);
    }
#endif
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void XRDC_Deinit(XRDC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

#if FSL_CLOCK_XRDC_GATE_COUNT
#if (FSL_CLOCK_XRDC_GATE_COUNT == 1)
    CLOCK_EnableClock(kCLOCK_Xrdc0);
#else
    uint8_t i;

    for (i = 0; i < ARRAY_SIZE(s_xrdcClock); i++)
    {
        CLOCK_DisableClock(s_xrdcClock[i]);
    }
#endif
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void XRDC_GetHardwareConfig(XRDC_Type *base, xrdc_hardware_config_t *config)
{
    assert(config);

    config->masterNumber = ((base->HWCFG0 & XRDC_HWCFG0_NMSTR_MASK) >> XRDC_HWCFG0_NMSTR_SHIFT) + 1U;
    config->domainNumber = ((base->HWCFG0 & XRDC_HWCFG0_NDID_MASK) >> XRDC_HWCFG0_NDID_SHIFT) + 1U;
    config->pacNumber = ((base->HWCFG0 & XRDC_HWCFG0_NPAC_MASK) >> XRDC_HWCFG0_NPAC_SHIFT) + 1U;
    config->mrcNumber = ((base->HWCFG0 & XRDC_HWCFG0_NMRC_MASK) >> XRDC_HWCFG0_NMRC_SHIFT) + 1U;
}

status_t XRDC_GetAndClearFirstDomainError(XRDC_Type *base, xrdc_error_t *error)
{
    return XRDC_GetAndClearFirstSpecificDomainError(base, error, XRDC_GetCurrentMasterDomainId(base));
}

status_t XRDC_GetAndClearFirstSpecificDomainError(XRDC_Type *base, xrdc_error_t *error, uint8_t domainId)
{
    assert(error);

    uint32_t errorBitMap; /* Domain error location bit map.   */
    uint32_t errorIndex;  /* The index of first domain error. */
    uint32_t regW1;       /* To save XRDC_DERR_W1.            */

    /* Get the error bitmap. */
    errorBitMap = base->DERRLOC[domainId];

    if (!errorBitMap) /* No error captured. */
    {
        return kStatus_XRDC_NoError;
    }

/* Get the first error controller index. */
#if (((__CORTEX_M == 0U) && (defined(__ICCARM__))) || defined(__riscv))
    errorIndex = 31U - XRDC_CountLeadingZeros(errorBitMap);
#else
    errorIndex = 31U - __CLZ(errorBitMap);
#endif

#if (defined(FSL_FEATURE_XRDC_HAS_FDID) && FSL_FEATURE_XRDC_HAS_FDID)
    /* Must write FDID[FDID] with the domain ID before reading the Domain Error registers. */
    base->FDID = XRDC_FDID_FDID(domainId);
#endif /* FSL_FEATURE_XRDC_HAS_FDID */
    /* Get the error information. */
    regW1 = base->DERR_W[errorIndex][1];
    error->controller = (xrdc_controller_t)errorIndex;
    error->address = base->DERR_W[errorIndex][0];
    error->errorState = (xrdc_error_state_t)XRDC_DERR_W1_EST_VAL(regW1);
    error->errorAttr = (xrdc_error_attr_t)XRDC_DERR_W1_EATR_VAL(regW1);
    error->errorType = (xrdc_error_type_t)XRDC_DERR_W1_ERW_VAL(regW1);
    error->errorPort = XRDC_DERR_W1_EPORT_VAL(regW1);
    error->domainId = XRDC_DERR_W1_EDID_VAL(regW1);

    /* Clear error pending. */
    base->DERR_W[errorIndex][3] = XRDC_DERR_W_RECR(0x01U);

    return kStatus_Success;
}

void XRDC_GetMemAccessDefaultConfig(xrdc_mem_access_config_t *config)
{
    assert(config);

    uint8_t i;

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SE) && FSL_FEATURE_XRDC_NO_MRGD_SE)
    config->enableSema = false;
    config->semaNum = 0U;
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SE */

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SZ) && FSL_FEATURE_XRDC_NO_MRGD_SZ)
    config->size = kXRDC_MemSizeNone;
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SZ */
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SRD) && FSL_FEATURE_XRDC_NO_MRGD_SRD)
    config->subRegionDisableMask = 0U;
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SRD */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_CR) && FSL_FEATURE_XRDC_HAS_MRGD_CR)
    config->codeRegion = kXRDC_MemCodeRegion0;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_CR */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
    config->enableAccset1Lock = false;
    config->enableAccset2Lock = false;
    config->accset1 = 0x000U;
    config->accset2 = 0x000U;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ACCSET */

    config->lockMode = kXRDC_AccessConfigLockWritable;

    config->baseAddress = 0U;
#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR) && FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR)
    config->endAddress = 0U;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR */

    for (i = 0U; i < FSL_FEATURE_XRDC_DOMAIN_COUNT; i++)
    {
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_DXACP) && FSL_FEATURE_XRDC_NO_MRGD_DXACP)
        config->policy[i] = kXRDC_AccessPolicyNone;
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_DXSEL) && FSL_FEATURE_XRDC_HAS_MRGD_DXSEL)
        config->policy[i] = kXRDC_AccessFlagsNone;
#endif
    }

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
    config->exclAccessLockMode = kXRDC_ExclAccessLockDisabled;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_EAL */
}

void XRDC_SetMemAccessConfig(XRDC_Type *base, const xrdc_mem_access_config_t *config)
{
    assert(config);
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SZ) && FSL_FEATURE_XRDC_NO_MRGD_SZ)
    /* Not allowed to set sub-region disable mask for memory region smaller than 256-bytes. */
    assert(!((config->size < kXRDC_MemSize256B) && (config->subRegionDisableMask)));
    /* Memory region minimum size = 32 bytes and base address must be aligned to 0-module-2**(SZ+1). */
    assert(config->size >= kXRDC_MemSize32B);
    assert(!(config->baseAddress & ((1U << (config->size + 1U)) - 1U)));
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SZ */

    uint32_t i;
    uint32_t regValue;
    uint8_t index = (uint8_t)config->mem;

    /* Set MRGD_W0. */
    base->MRGD[index].MRGD_W[0] = config->baseAddress;

/* Set MRGD_W1. */
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SZ) && FSL_FEATURE_XRDC_NO_MRGD_SZ)
    base->MRGD[index].MRGD_W[1] = XRDC_MRGD_W_SZ(config->size) | XRDC_MRGD_W_SRD(config->subRegionDisableMask);
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SZ */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR) && FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR)
    base->MRGD[index].MRGD_W[1] = config->endAddress;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR */

    /* Set MRGD_W2. */
    regValue = 0U;
/* Set MRGD_W2[D0ACP ~ D7ACP] or MRGD_W2[D0SEL ~ D2SEL]. */
#if (FSL_FEATURE_XRDC_DOMAIN_COUNT <= 8U)
    i = FSL_FEATURE_XRDC_DOMAIN_COUNT;
#elif(FSL_FEATURE_XRDC_DOMAIN_COUNT <= 16U)
    i = 8U;
#else
#error Does not support more than 16 domain.
#endif

    while (i--)
    {
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_DXACP) && FSL_FEATURE_XRDC_NO_MRGD_DXACP)
        regValue <<= XRDC_MRGD_DXACP_WIDTH;
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_DXSEL) && FSL_FEATURE_XRDC_HAS_MRGD_DXSEL)
        regValue <<= XRDC_MRGD_DXSEL_WIDTH;
#endif
        regValue |= config->policy[i];
    }

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SE) && FSL_FEATURE_XRDC_NO_MRGD_SE)
    regValue |= XRDC_MRGD_W_SE(config->enableSema) | XRDC_MRGD_W_SNUM(config->semaNum);
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SE */

    base->MRGD[index].MRGD_W[2] = regValue;

    /* Set MRGD_W3. */
    regValue = 0U;

#if ((FSL_FEATURE_XRDC_DOMAIN_COUNT > 8U) && (FSL_FEATURE_XRDC_DOMAIN_COUNT <= 16))
    /* Set MRGD_W3[D8ACP ~ D15ACP]. */
    for (i = FSL_FEATURE_XRDC_DOMAIN_COUNT - 1U; i > 7U; i--)
    {
        regValue <<= XRDC_MRGD_DXACP_WIDTH;
        regValue |= config->policy[i];
    }
#endif

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_CR) && FSL_FEATURE_XRDC_HAS_MRGD_CR)
    regValue |= XRDC_MRGD_W_CR(config->codeRegion);
#endif

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_W3_VLD) && FSL_FEATURE_XRDC_NO_MRGD_W3_VLD)
    regValue |= XRDC_MRGD_W_VLD_MASK | XRDC_MRGD_W_LK2(config->lockMode);
#endif

    base->MRGD[index].MRGD_W[3] = regValue;

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
    /*
     * Set MRGD_W3[EAL].
     * If write with a value of MRGD_W3[EAL]=0, then the other fields of MRGD_W3 are updated.
     * If write with a value of MRGD_W3[EAL]!=0, then only the EAL is updated.
     */
    if (kXRDC_ExclAccessLockDisabled != config->exclAccessLockMode)
    {
        base->MRGD[index].MRGD_W[3] = XRDC_MRGD_W_EAL(config->exclAccessLockMode);
    }
#endif

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
    /* Set MRGD_W4. */
    base->MRGD[index].MRGD_W[4] = XRDC_MRGD_W_LKAS1(config->enableAccset1Lock) | XRDC_MRGD_W_ACCSET1(config->accset1) |
                                  XRDC_MRGD_W_LKAS2(config->enableAccset2Lock) | XRDC_MRGD_W_ACCSET2(config->accset2) |
                                  XRDC_MRGD_W_VLD_MASK | XRDC_MRGD_W_LK2(config->lockMode);
#endif
}

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
void XRDC_SetMemExclAccessLockMode(XRDC_Type *base, xrdc_mem_t mem, xrdc_excl_access_lock_config_t lockMode)
{
    /* Write kXRDC_ExclAccessLockDisabled is not allowed. */
    assert(kXRDC_ExclAccessLockDisabled != lockMode);

    uint32_t reg = base->MRGD[mem].MRGD_W[4];

    /* Step 1. Set the memory region exclusive access lock mode configuration. */
    base->MRGD[mem].MRGD_W[3] = XRDC_MRGD_W_EAL(lockMode);

    /* Step 2. Set MRGD_W3 will clear the MRGD_W4[VLD]. So should re-assert it. */
    base->MRGD[mem].MRGD_W[4] = reg;
}

void XRDC_ForceMemExclAccessLockRelease(XRDC_Type *base, xrdc_mem_t mem)
{
    uint32_t primask;

    primask = DisableGlobalIRQ();
    base->MRGD[mem].MRGD_W[3] = XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL1;
    base->MRGD[mem].MRGD_W[3] = XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL2;
    EnableGlobalIRQ(primask);
}
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_EAL */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
void XRDC_SetMemAccsetLock(XRDC_Type *base, xrdc_mem_t mem, xrdc_mem_accset_t accset, bool lock)
{
    uint32_t lkasMask = 0U;

    switch (accset)
    {
        case kXRDC_MemAccset1:
            lkasMask = XRDC_MRGD_W_LKAS1_MASK;
            break;
        case kXRDC_MemAccset2:
            lkasMask = XRDC_MRGD_W_LKAS2_MASK;
            break;
        default:
            break;
    }

    if (lock)
    {
        base->MRGD[mem].MRGD_W[4] |= lkasMask;
    }
    else
    {
        base->MRGD[mem].MRGD_W[4] &= ~lkasMask;
    }
}
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ACCSET */

void XRDC_GetPeriphAccessDefaultConfig(xrdc_periph_access_config_t *config)
{
    assert(config);

    uint8_t i;

#if !(defined(FSL_FEATURE_XRDC_NO_PDAC_SE) && FSL_FEATURE_XRDC_NO_PDAC_SE)
    config->enableSema = false;
    config->semaNum = 0U;
#endif /* FSL_FEATURE_XRDC_NO_PDAC_SE */
    config->lockMode = kXRDC_AccessConfigLockWritable;
#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
    config->exclAccessLockMode = kXRDC_ExclAccessLockDisabled;
#endif /* FSL_FEATURE_XRDC_HAS_PDAC_EAL */
    for (i = 0U; i < FSL_FEATURE_XRDC_DOMAIN_COUNT; i++)
    {
        config->policy[i] = kXRDC_AccessPolicyNone;
    }
}

void XRDC_SetPeriphAccessConfig(XRDC_Type *base, const xrdc_periph_access_config_t *config)
{
    assert(config);

    uint32_t i;
    uint32_t regValue;
    uint8_t index = (uint8_t)config->periph;

    /* Set PDAC_W0[D0ACP ~ D7ACP]. */
    regValue = 0U;
#if (FSL_FEATURE_XRDC_DOMAIN_COUNT <= 8U)
    i = FSL_FEATURE_XRDC_DOMAIN_COUNT;
#elif(FSL_FEATURE_XRDC_DOMAIN_COUNT <= 16U)
    i = 8U;
#else
#error Does not support more than 16 domain.
#endif

    while (i--)
    {
        regValue <<= XRDC_PDAC_DXACP_WIDTH;
        regValue |= config->policy[i];
    }

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SE) && FSL_FEATURE_XRDC_NO_MRGD_SE)
    regValue |= (XRDC_PDAC_W_SE(config->enableSema) | XRDC_PDAC_W_SNUM(config->semaNum));
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SE */

    /* Set PDAC_W0. */
    base->PDAC_W[index][0U] = regValue;

#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
    /*
     * If write with a value of PDAC_W1[EAL]=0, then the other fields of PDAC_W1 are updated.
     * If write with a value of PDAC_W1[EAL]!=0, then only the EAL is updated.
     */
    base->PDAC_W[index][1U] = XRDC_PDAC_W_EAL(config->exclAccessLockMode);
#endif

    regValue = 0U;
#if ((FSL_FEATURE_XRDC_DOMAIN_COUNT > 8U) && (FSL_FEATURE_XRDC_DOMAIN_COUNT <= 16))
    /* Set PDAC_W1[D8ACP ~ D15ACP]. */

    for (i = FSL_FEATURE_XRDC_DOMAIN_COUNT - 1U; i > 7U; i--)
    {
        regValue <<= XRDC_PDAC_DXACP_WIDTH;
        regValue |= config->policy[i];
    }
#endif
    /* Set PDAC_W1. */
    base->PDAC_W[index][1] = regValue | XRDC_PDAC_W_VLD_MASK | XRDC_PDAC_W_LK2(config->lockMode);
}

#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
void XRDC_ForcePeriphExclAccessLockRelease(XRDC_Type *base, xrdc_periph_t periph)
{
    uint32_t primask;

    primask = DisableGlobalIRQ();
    base->PDAC_W[periph][1] = XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL1;
    base->PDAC_W[periph][1] = XRDC_FORCE_EXCL_ACS_LOCK_REL_VAL2;
    EnableGlobalIRQ(primask);
}
#endif /* FSL_FEATURE_XRDC_HAS_PDAC_EAL */
