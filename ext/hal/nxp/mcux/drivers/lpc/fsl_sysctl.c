/*
 * Copyright (c) 2018, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_sysctl.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.sysctl"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance.
 *
 * @param base SYSCTL peripheral base address.
 * @return Instance number.
 */
static uint32_t SYSCTL_GetInstance(SYSCTL_Type *base);

/*!
 * @brief Enable SYSCTL write protect
 *
 * @param base SYSCTL peripheral base address.
 * @param regAddr register address
 * @param value value to write.
 */
static void SYSCTL_UpdateRegister(SYSCTL_Type *base, volatile uint32_t *regAddr, uint32_t value);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief SYSCTL base address array name */
static SYSCTL_Type *const s_sysctlBase[] = SYSCTL_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief SYSCTL clock array name */
static const clock_ip_name_t s_sysctlClock[] = SYSCTL_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static void SYSCTL_UpdateRegister(SYSCTL_Type *base, volatile uint32_t *regAddr, uint32_t value)
{
    base->UPDATELCKOUT &= ~SYSCTL_UPDATELCKOUT_UPDATELCKOUT_MASK;
    *regAddr = value;
    base->UPDATELCKOUT |= SYSCTL_UPDATELCKOUT_UPDATELCKOUT_MASK;
}

static uint32_t SYSCTL_GetInstance(SYSCTL_Type *base)
{
    uint8_t instance = 0;

    while ((instance < ARRAY_SIZE(s_sysctlBase)) && (s_sysctlBase[instance] != base))
    {
        instance++;
    }

    assert(instance < ARRAY_SIZE(s_sysctlBase));

    return instance;
}

/*!
 * @brief SYSCTL initial
 *
 * @param base Base address of the SYSCTL peripheral.
 */
void SYSCTL_Init(SYSCTL_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable SYSCTL clock. */
    CLOCK_EnableClock(s_sysctlClock[SYSCTL_GetInstance(base)]);
#endif
}

/*!
 * @brief SYSCTL deinit
 *
 * @param base Base address of the SYSCTL peripheral.
 */
void SYSCTL_Deinit(SYSCTL_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable SYSCTL clock. */
    CLOCK_DisableClock(s_sysctlClock[SYSCTL_GetInstance(base)]);
#endif
}

/*!
 * @brief SYSCTL share set configure for separate signal
 *
 * @param base Base address of the SYSCTL peripheral
 * @param flexCommIndex index of flexcomm,reference _sysctl_share_src
 * @param setIndex share set for sck, reference _sysctl_share_set_index
 *
 */
void SYSCTL_SetShareSet(SYSCTL_Type *base, uint32_t flexCommIndex, sysctl_fcctrlsel_signal_t signal, uint32_t set)
{
    uint32_t tempReg = base->FCCTRLSEL[flexCommIndex];

    tempReg &= ~(SYSCTL_FCCTRLSEL_SCKINSEL_MASK << signal);
    tempReg |= (set + 1U) << signal;

    SYSCTL_UpdateRegister(base, &base->FCCTRLSEL[flexCommIndex], tempReg);
}

/*!
 * @brief SYSCTL share set configure for flexcomm
 *
 * @param base Base address of the SYSCTL peripheral.
 * @param flexCommIndex index of flexcomm, reference _sysctl_share_src
 * @param sckSet share set for sck,reference _sysctl_share_set_index
 * @param wsSet share set for ws, reference _sysctl_share_set_index
 * @param dataInSet share set for data in, reference _sysctl_share_set_index
 * @param dataOutSet share set for data out, reference _sysctl_share_set_index
 *
 */
void SYSCTL_SetFlexcommShareSet(
    SYSCTL_Type *base, uint32_t flexCommIndex, uint32_t sckSet, uint32_t wsSet, uint32_t dataInSet, uint32_t dataOutSet)
{
    uint32_t tempReg = base->FCCTRLSEL[flexCommIndex];

    tempReg &= ~(SYSCTL_FCCTRLSEL_SCKINSEL_MASK | SYSCTL_FCCTRLSEL_WSINSEL_MASK | SYSCTL_FCCTRLSEL_DATAINSEL_MASK |
                 SYSCTL_FCCTRLSEL_DATAOUTSEL_MASK);
    tempReg |= SYSCTL_FCCTRLSEL_SCKINSEL(sckSet + 1U) | SYSCTL_FCCTRLSEL_WSINSEL(wsSet + 1U) |
               SYSCTL_FCCTRLSEL_DATAINSEL(dataInSet + 1U) | SYSCTL_FCCTRLSEL_DATAOUTSEL(dataOutSet + 1U);

    SYSCTL_UpdateRegister(base, &base->FCCTRLSEL[flexCommIndex], tempReg);
}

/*!
 * @brief SYSCTL share set source configure
 *
 * @param base Base address of the SYSCTL peripheral
 * @param setIndex index of share set, reference _sysctl_share_set_index
 * @param sckShareSrc sck source for this share set,reference _sysctl_share_src
 * @param wsShareSrc ws source for this share set,reference _sysctl_share_src
 * @param dataInShareSrc data in source for this share set,reference _sysctl_share_src
 * @param dataOutShareSrc data out source for this share set,reference _sysctl_dataout_mask
 *
 */
void SYSCTL_SetShareSetSrc(SYSCTL_Type *base,
                           uint32_t setIndex,
                           uint32_t sckShareSrc,
                           uint32_t wsShareSrc,
                           uint32_t dataInShareSrc,
                           uint32_t dataOutMask)
{
    uint32_t tempReg = base->SHAREDCTRLSET[setIndex];

    /* WS,SCK,DATA IN */
    tempReg &=
        ~(SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDSCKSEL_MASK | SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDWSSEL_MASK |
          SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDDATASEL_MASK);
    tempReg |= SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDSCKSEL(sckShareSrc) |
               SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDWSSEL(wsShareSrc) |
               SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDDATASEL(dataInShareSrc);

    /* data out */
    tempReg &=
        ~(SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC0DATAOUTEN_MASK | SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC1DATAOUTEN_MASK |
          SYSCTL_SHARECTRLSET_SHAREDCTRLSET_F20DATAOUTEN_MASK | SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC6DATAOUTEN_MASK |
          SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC7DATAOUTEN_MASK);
    tempReg |= dataOutMask;

    SYSCTL_UpdateRegister(base, &base->SHAREDCTRLSET[setIndex], tempReg);
}

/*!
 * @brief SYSCTL sck source configure
 *
 * @param base Base address of the SYSCTL peripheral
 * @param setIndex index of share set, reference _sysctl_share_set_index
 * @param sckShareSrc sck source fro this share set,reference _sysctl_share_src
 *
 */
void SYSCTL_SetShareSignalSrc(SYSCTL_Type *base,
                              uint32_t setIndex,
                              sysctl_sharedctrlset_signal_t signal,
                              uint32_t shareSrc)
{
    uint32_t tempReg = base->SHAREDCTRLSET[setIndex];

    if (signal == kSYSCTL_SharedCtrlSignalDataOut)
    {
        tempReg |= 1 << (signal + shareSrc);
    }
    else
    {
        tempReg &= ~(SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDSCKSEL_MASK << signal);
        tempReg |= shareSrc << signal;
    }

    SYSCTL_UpdateRegister(base, &base->SHAREDCTRLSET[setIndex], tempReg);
}
