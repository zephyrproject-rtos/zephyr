/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_gint.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.gint"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to GINT bases for each instance. */
static GINT_Type *const s_gintBases[FSL_FEATURE_SOC_GINT_COUNT] = GINT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Clocks for each instance. */
static const clock_ip_name_t s_gintClocks[FSL_FEATURE_SOC_GINT_COUNT] = GINT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
/*! @brief Resets for each instance. */
static const reset_ip_name_t s_gintResets[FSL_FEATURE_SOC_GINT_COUNT] = GINT_RSTS;
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/* @brief Irq number for each instance */
static const IRQn_Type s_gintIRQ[FSL_FEATURE_SOC_GINT_COUNT] = GINT_IRQS;

/*! @brief Callback function array for GINT(s). */
static gint_cb_t s_gintCallback[FSL_FEATURE_SOC_GINT_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t GINT_GetInstance(GINT_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_gintBases); instance++)
    {
        if (s_gintBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_gintBases));

    return instance;
}

/*!
 * brief	Initialize GINT peripheral.

 * This function initializes the GINT peripheral and enables the clock.
 *
 * param base Base address of the GINT peripheral.
 *
 * retval None.
 */
void GINT_Init(GINT_Type *base)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);

    s_gintCallback[instance] = NULL;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the peripheral clock */
    CLOCK_EnableClock(s_gintClocks[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(s_gintResets[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */
}

/*!
 * brief	Setup GINT peripheral control parameters.

 * This function sets the control parameters of GINT peripheral.
 *
 * param base Base address of the GINT peripheral.
 * param comb Controls if the enabled inputs are logically ORed or ANDed for interrupt generation.
 * param trig Controls if the enabled inputs are level or edge sensitive based on polarity.
 * param callback This function is called when configured group interrupt is generated.
 *
 * retval None.
 */
void GINT_SetCtrl(GINT_Type *base, gint_comb_t comb, gint_trig_t trig, gint_cb_t callback)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);

    base->CTRL = (GINT_CTRL_COMB(comb) | GINT_CTRL_TRIG(trig));

    /* Save callback pointer */
    s_gintCallback[instance] = callback;
}

/*!
 * brief	Get GINT peripheral control parameters.

 * This function returns the control parameters of GINT peripheral.
 *
 * param base Base address of the GINT peripheral.
 * param comb Pointer to store combine input value.
 * param trig Pointer to store trigger value.
 * param callback Pointer to store callback function.
 *
 * retval None.
 */
void GINT_GetCtrl(GINT_Type *base, gint_comb_t *comb, gint_trig_t *trig, gint_cb_t *callback)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);

    *comb = (gint_comb_t)((base->CTRL & GINT_CTRL_COMB_MASK) >> GINT_CTRL_COMB_SHIFT);
    *trig = (gint_trig_t)((base->CTRL & GINT_CTRL_TRIG_MASK) >> GINT_CTRL_TRIG_SHIFT);
    *callback = s_gintCallback[instance];
}

/*!
 * brief	Configure GINT peripheral pins.

 * This function enables and controls the polarity of enabled pin(s) of a given port.
 *
 * param base Base address of the GINT peripheral.
 * param port Port number.
 * param polarityMask Each bit position selects the polarity of the corresponding enabled pin.
 *        0 = The pin is active LOW. 1 = The pin is active HIGH.
 * param enableMask Each bit position selects if the corresponding pin is enabled or not.
 *        0 = The pin is disabled. 1 = The pin is enabled.
 *
 * retval None.
 */
void GINT_ConfigPins(GINT_Type *base, gint_port_t port, uint32_t polarityMask, uint32_t enableMask)
{
    base->PORT_POL[port] = polarityMask;
    base->PORT_ENA[port] = enableMask;
}

/*!
 * brief	Get GINT peripheral pin configuration.

 * This function returns the pin configuration of a given port.
 *
 * param base Base address of the GINT peripheral.
 * param port Port number.
 * param polarityMask Pointer to store the polarity mask Each bit position indicates the polarity of the corresponding
 enabled pin.
 *        0 = The pin is active LOW. 1 = The pin is active HIGH.
 * param enableMask Pointer to store the enable mask. Each bit position indicates if the corresponding pin is enabled
 or not.
 *        0 = The pin is disabled. 1 = The pin is enabled.
 *
 * retval None.
 */
void GINT_GetConfigPins(GINT_Type *base, gint_port_t port, uint32_t *polarityMask, uint32_t *enableMask)
{
    *polarityMask = base->PORT_POL[port];
    *enableMask = base->PORT_ENA[port];
}

/*!
 * brief	Enable callback.

 * This function enables the interrupt for the selected GINT peripheral. Although the pin(s) are monitored
 * as soon as they are enabled, the callback function is not enabled until this function is called.
 *
 * param base Base address of the GINT peripheral.
 *
 * retval None.
 */
void GINT_EnableCallback(GINT_Type *base)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);
    /* If GINT is configured in "AND" mode a spurious interrupt is generated.
       Clear status and pending interrupt before enabling the irq in NVIC. */
    GINT_ClrStatus(base);
    NVIC_ClearPendingIRQ(s_gintIRQ[instance]);
    EnableIRQ(s_gintIRQ[instance]);
}

/*!
 * brief	Disable callback.

 * This function disables the interrupt for the selected GINT peripheral. Although the pins are still
 * being monitored but the callback function is not called.
 *
 * param base Base address of the peripheral.
 *
 * retval None.
 */
void GINT_DisableCallback(GINT_Type *base)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);
    DisableIRQ(s_gintIRQ[instance]);
    GINT_ClrStatus(base);
    NVIC_ClearPendingIRQ(s_gintIRQ[instance]);
}

/*!
 * brief	Deinitialize GINT peripheral.

 * This function disables the GINT clock.
 *
 * param base Base address of the GINT peripheral.
 *
 * retval None.
 */
void GINT_Deinit(GINT_Type *base)
{
    uint32_t instance;

    instance = GINT_GetInstance(base);

    /* Cleanup */
    GINT_DisableCallback(base);
    s_gintCallback[instance] = NULL;

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(s_gintResets[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the peripheral clock */
    CLOCK_DisableClock(s_gintClocks[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/* IRQ handler functions overloading weak symbols in the startup */
#if defined(GINT0)
void GINT0_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[0]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[0] != NULL)
    {
        s_gintCallback[0]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT1)
void GINT1_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[1]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[1] != NULL)
    {
        s_gintCallback[1]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT2)
void GINT2_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[2]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[2] != NULL)
    {
        s_gintCallback[2]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT3)
void GINT3_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[3]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[3] != NULL)
    {
        s_gintCallback[3]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT4)
void GINT4_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[4]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[4] != NULL)
    {
        s_gintCallback[4]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT5)
void GINT5_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[5]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[5] != NULL)
    {
        s_gintCallback[5]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT6)
void GINT6_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[6]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[6] != NULL)
    {
        s_gintCallback[6]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(GINT7)
void GINT7_DriverIRQHandler(void)
{
    /* Clear interrupt before callback */
    s_gintBases[7]->CTRL |= GINT_CTRL_INT_MASK;
    /* Call user function */
    if (s_gintCallback[7] != NULL)
    {
        s_gintCallback[7]();
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
