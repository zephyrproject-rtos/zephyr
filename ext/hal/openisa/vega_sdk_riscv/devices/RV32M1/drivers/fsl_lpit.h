/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_LPIT_H_
#define _FSL_LPIT_H_

#include "fsl_common.h"

/*!
 * @addtogroup lpit
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_LPIT_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
                                                        /*@{*/

/*!
 * @brief List of LPIT channels
 * @note Actual number of available channels is SoC-dependent
 */
typedef enum _lpit_chnl
{
    kLPIT_Chnl_0 = 0U, /*!< LPIT channel number 0*/
    kLPIT_Chnl_1,      /*!< LPIT channel number 1 */
    kLPIT_Chnl_2,      /*!< LPIT channel number 2 */
    kLPIT_Chnl_3,      /*!< LPIT channel number 3 */
} lpit_chnl_t;

/*! @brief Mode options available for the LPIT timer. */
typedef enum _lpit_timer_modes
{
    kLPIT_PeriodicCounter = 0U, /*!< Use the all 32-bits, counter loads and decrements to zero */
    kLPIT_DualPeriodicCounter,  /*!< Counter loads, lower 16-bits  decrement to zero, then
                                     upper 16-bits  decrement */
    kLPIT_TriggerAccumulator,   /*!< Counter loads on first trigger and decrements on each trigger */
    kLPIT_InputCapture          /*!< Counter  loads with 0xFFFFFFFF, decrements to zero. It stores
                                    the inverse of the current value when a input trigger is detected */
} lpit_timer_modes_t;

/*!
 * @brief Trigger options available.
 *
 * This is used for both internal and external trigger sources. The actual trigger options
 * available is SoC-specific, user should refer to the reference manual.
 */
typedef enum _lpit_trigger_select
{
    kLPIT_Trigger_TimerChn0 = 0U, /*!< Channel 0 is selected as a trigger source */
    kLPIT_Trigger_TimerChn1,      /*!< Channel 1 is selected as a trigger source */
    kLPIT_Trigger_TimerChn2,      /*!< Channel 2 is selected as a trigger source */
    kLPIT_Trigger_TimerChn3,      /*!< Channel 3 is selected as a trigger source */
    kLPIT_Trigger_TimerChn4,      /*!< Channel 4 is selected as a trigger source */
    kLPIT_Trigger_TimerChn5,      /*!< Channel 5 is selected as a trigger source */
    kLPIT_Trigger_TimerChn6,      /*!< Channel 6 is selected as a trigger source */
    kLPIT_Trigger_TimerChn7,      /*!< Channel 7 is selected as a trigger source */
    kLPIT_Trigger_TimerChn8,      /*!< Channel 8 is selected as a trigger source */
    kLPIT_Trigger_TimerChn9,      /*!< Channel 9 is selected as a trigger source */
    kLPIT_Trigger_TimerChn10,     /*!< Channel 10 is selected as a trigger source */
    kLPIT_Trigger_TimerChn11,     /*!< Channel 11 is selected as a trigger source */
    kLPIT_Trigger_TimerChn12,     /*!< Channel 12 is selected as a trigger source */
    kLPIT_Trigger_TimerChn13,     /*!< Channel 13 is selected as a trigger source */
    kLPIT_Trigger_TimerChn14,     /*!< Channel 14 is selected as a trigger source */
    kLPIT_Trigger_TimerChn15      /*!< Channel 15 is selected as a trigger source */
} lpit_trigger_select_t;

/*! @brief Trigger source options available */
typedef enum _lpit_trigger_source
{
    kLPIT_TriggerSource_External = 0U, /*!< Use external trigger input */
    kLPIT_TriggerSource_Internal       /*!< Use internal trigger */
} lpit_trigger_source_t;

/*!
 * @brief List of LPIT interrupts.
 *
 * @note Number of timer channels are SoC-specific. See the SoC Reference Manual.
 */
typedef enum _lpit_interrupt_enable
{
    kLPIT_Channel0TimerInterruptEnable = (1U << 0), /*!< Channel 0 Timer interrupt */
    kLPIT_Channel1TimerInterruptEnable = (1U << 1), /*!< Channel 1 Timer interrupt */
    kLPIT_Channel2TimerInterruptEnable = (1U << 2), /*!< Channel 2 Timer interrupt */
    kLPIT_Channel3TimerInterruptEnable = (1U << 3), /*!< Channel 3 Timer interrupt */
} lpit_interrupt_enable_t;

/*!
 * @brief List of LPIT status flags
 *
 * @note Number of timer channels are SoC-specific. See the SoC Reference Manual.
 */
typedef enum _lpit_status_flags
{
    kLPIT_Channel0TimerFlag = (1U << 0), /*!< Channel 0 Timer interrupt flag */
    kLPIT_Channel1TimerFlag = (1U << 1), /*!< Channel 1 Timer interrupt flag */
    kLPIT_Channel2TimerFlag = (1U << 2), /*!< Channel 2 Timer interrupt flag */
    kLPIT_Channel3TimerFlag = (1U << 3), /*!< Channel 3 Timer interrupt flag */
} lpit_status_flags_t;

/*! @brief Structure to configure the channel timer. */
typedef struct _lpit_chnl_params
{
    bool chainChannel;                   /*!< true: Timer chained to previous timer;
                                              false: Timer not chained */
    lpit_timer_modes_t timerMode;        /*!< Timers mode of operation. */
    lpit_trigger_select_t triggerSelect; /*!< Trigger selection for the timer */
    lpit_trigger_source_t triggerSource; /*!< Decides if we use external or internal trigger. */
    bool enableReloadOnTrigger;          /*!< true: Timer reloads when a trigger is detected;
                                              false: No effect */
    bool enableStopOnTimeout;            /*!< true: Timer will stop after timeout;
                                              false: does not stop after timeout */
    bool enableStartOnTrigger;           /*!< true: Timer starts when a trigger is detected;
                                              false: decrement immediately */
} lpit_chnl_params_t;

/*!
 * @brief LPIT configuration structure
 *
 * This structure holds the configuration settings for the LPIT peripheral. To initialize this
 * structure to reasonable defaults, call the LPIT_GetDefaultConfig() function and pass a
 * pointer to the configuration structure instance.
 *
 * The configuration structure can be made constant so as to reside in flash.
 */
typedef struct _lpit_config
{
    bool enableRunInDebug; /*!< true: Timers run in debug mode; false: Timers stop in debug mode */
    bool enableRunInDoze;  /*!< true: Timers run in doze mode; false: Timers stop in doze mode */
} lpit_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Ungates the LPIT clock and configures the peripheral for a basic operation.
 *
 * This function issues a software reset to reset all channels and registers except the Module
 * Control register.
 *
 * @note This API should be called at the beginning of the application using the LPIT driver.
 *
 * @param base   LPIT peripheral base address.
 * @param config Pointer to the user configuration structure.
 */
void LPIT_Init(LPIT_Type* base, const lpit_config_t* config);

/*!
 * @brief Disables the module and gates the LPIT clock.
 *
 * @param base LPIT peripheral base address.
 */
void LPIT_Deinit(LPIT_Type* base);

/*!
 * @brief Fills in the LPIT configuration structure with default settings.
 *
 * The default values are:
 * @code
 *    config->enableRunInDebug = false;
 *    config->enableRunInDoze = false;
 * @endcode
 * @param config Pointer to the user configuration structure.
 */
void LPIT_GetDefaultConfig(lpit_config_t* config);

/*!
 * @brief Sets up an LPIT channel based on the user's preference.
 *
 * This function sets up the operation mode to one of the options available in the
 * enumeration ::lpit_timer_modes_t. It sets the trigger source as either internal or external,
 * trigger selection and the timers behaviour when a timeout occurs. It also chains
 * the timer if a prior timer if requested by the user.
 *
 * @param base      LPIT peripheral base address.
 * @param channel   Channel that is being configured.
 * @param chnlSetup Configuration parameters.
 */
status_t LPIT_SetupChannel(LPIT_Type* base, lpit_chnl_t channel, const lpit_chnl_params_t* chnlSetup);

/*! @}*/

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enables the selected PIT interrupts.
 *
 * @param base LPIT peripheral base address.
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::lpit_interrupt_enable_t
 */
static inline void LPIT_EnableInterrupts(LPIT_Type* base, uint32_t mask)
{
    base->MIER |= mask;
}

/*!
 * @brief Disables the selected PIT interrupts.
 *
 * @param base LPIT peripheral base address.
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::lpit_interrupt_enable_t
 */
static inline void LPIT_DisableInterrupts(LPIT_Type* base, uint32_t mask)
{
    base->MIER &= ~mask;
}

/*!
 * @brief Gets the enabled LPIT interrupts.
 *
 * @param base LPIT peripheral base address.
 *
 * @return The enabled interrupts. This is the logical OR of members of the
 *         enumeration ::lpit_interrupt_enable_t
 */
static inline uint32_t LPIT_GetEnabledInterrupts(LPIT_Type* base)
{
    return base->MIER;
}

/*! @}*/

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Gets the LPIT status flags.
 *
 * @param base LPIT peripheral base address.
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::lpit_status_flags_t
 */
static inline uint32_t LPIT_GetStatusFlags(LPIT_Type* base)
{
    return base->MSR;
}

/*!
 * @brief Clears the LPIT status flags.
 *
 * @param base LPIT peripheral base address.
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::lpit_status_flags_t
 */
static inline void LPIT_ClearStatusFlags(LPIT_Type* base, uint32_t mask)
{
    /* Writing a 1 to the status bit will clear the flag */
    base->MSR = mask;
}

/*! @}*/

/*!
 * @name Read and Write the timer period
 * @{
 */

/*!
 * @brief Sets the timer period in units of count.
 *
 * Timers begin counting down from the value set by this function until it reaches 0, at which point
 * it generates an interrupt and loads this register value again.
 * Writing a new value to this register does not restart the timer. Instead, the value
 * is loaded after the timer expires.
 *
 * @note User can call the utility macros provided in fsl_common.h to convert to ticks.
 *
 * @param base    LPIT peripheral base address.
 * @param channel Timer channel number.
 * @param ticks   Timer period in units of ticks.
 */
static inline void LPIT_SetTimerPeriod(LPIT_Type* base, lpit_chnl_t channel, uint32_t ticks)
{
    base->CHANNEL[channel].TVAL = ticks;
}

/*!
 * @brief Reads the current timer counting value.
 *
 * This function returns the real-time timer counting value, in a range from 0 to a
 * timer period.
 *
 * @note User can call the utility macros provided in fsl_common.h to convert ticks to microseconds or milliseconds.
 *
 * @param base    LPIT peripheral base address.
 * @param channel Timer channel number.
 *
 * @return Current timer counting value in ticks.
 */
static inline uint32_t LPIT_GetCurrentTimerCount(LPIT_Type* base, lpit_chnl_t channel)
{
    return base->CHANNEL[channel].CVAL;
}

/*! @}*/

/*!
 * @name Timer Start and Stop
 * @{
 */

/*!
 * @brief Starts the timer counting.
 *
 * After calling this function, timers load the period value and count down to 0. When the timer
 * reaches 0, it generates a trigger pulse and sets the timeout interrupt flag.
 *
 * @param base    LPIT peripheral base address.
 * @param channel Timer channel number.
 */
static inline void LPIT_StartTimer(LPIT_Type* base, lpit_chnl_t channel)
{
    base->SETTEN |= (LPIT_SETTEN_SET_T_EN_0_MASK << channel);
}

/*!
 * @brief Stops the timer counting.
 *
 * @param base    LPIT peripheral base address.
 * @param channel Timer channel number.
 */
static inline void LPIT_StopTimer(LPIT_Type* base, lpit_chnl_t channel)
{
    base->CLRTEN |= (LPIT_CLRTEN_CLR_T_EN_0_MASK << channel);
}

/*! @}*/

/*!
 * @brief Performs a software reset on the LPIT module.
 *
 * This resets all channels and registers except the Module Control Register.
 *
 * @param base LPIT peripheral base address.
 */
static inline void LPIT_Reset(LPIT_Type* base)
{
    base->MCR |= LPIT_MCR_SW_RST_MASK;
    base->MCR &= ~LPIT_MCR_SW_RST_MASK;
}

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __FSL_LPIT_H__ */
