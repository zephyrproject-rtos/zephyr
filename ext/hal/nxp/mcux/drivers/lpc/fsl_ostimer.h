/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_OSTIMER_H_
#define _FSL_OSTIMER_H_

#include "fsl_common.h"

/*!
 * @addtogroup ostimer
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief OSTIMER driver version 2.0.0. */
#define FSL_OSTIMER_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief OSTIMER status flags.
 */
enum _ostimer_flags
{
    kOSTIMER_MatchInterruptFlag = (OSTIMER_OSEVENT_CTRL_OSTIMER_INTRFLAG_MASK), /*!< Match interrupt flag bit, sets if
                                                                                   the match value was reached. */
};

/*! @brief ostimer callback function. */
typedef void (*ostimer_callback_t)(void);

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
* @brief Initializes an OSTIMER by turning its bus clock on
*
*/
void OSTIMER_Init(OSTIMER_Type *base);

/*!
 * @brief Deinitializes a OSTIMER instance.
 *
 * This function shuts down OSTIMER bus clock
 *
 * @param base OSTIMER peripheral base address.
 */
void OSTIMER_Deinit(OSTIMER_Type *base);

/*!
 * @brief OSTIMER software reset.
 *
 * This function will use software to trigger an OSTIMER reset.
 * Please note that, the OS timer reset bit was in PMC->OSTIMERr register.
 *
 * @param base OSTIMER peripheral base address.
 */
static inline void OSTIMER_SoftwareReset(OSTIMER_Type *base)
{
    PMC->OSTIMERr |= PMC_OSTIMER_SOFTRESET_MASK;
    PMC->OSTIMERr &= ~PMC_OSTIMER_SOFTRESET_MASK;
}

/*!
 * @brief Get OSTIMER status Flags.
 *
 * This returns the status flag.
 * Currently, only match interrupt flag can be got.
 *
 * @param base OSTIMER peripheral base address.
 * @return status register value
 */
uint32_t OSTIMER_GetStatusFlags(OSTIMER_Type *base);

/*!
 * @brief Clear Status Interrupt Flags.
 *
 * This clears intrrupt status flag.
 * Currently, only match interrupt flag can be cleared.
 *
 * @param base OSTIMER peripheral base address.
 * @return none
 */
void OSTIMER_ClearStatusFlags(OSTIMER_Type *base, uint32_t mask);

/*!
 * @brief Set the match raw value for OSTIMER.
 *
 * This function will set a match value for OSTIMER with an optional callback. And this callback
 * will be called while the data in dedicated pair match register is equals to the value of central EVTIMER.
 * Please note that, the data format is gray-code, if decimal data was desired, please using OSTIMER_SetMatchValue().
 *
 * @param base   OSTIMER peripheral base address.
 * @param count  OSTIMER timer match value.(Value is gray-code format)
 *
 * @param cb     OSTIMER callback (can be left as NULL if none, otherwise should be a void func(void)).
 * @return       none
 */
void OSTIMER_SetMatchRawValue(OSTIMER_Type *base, uint64_t count, ostimer_callback_t cb);

/*!
 * @brief Set the match value for OSTIMER.
 *
 * This function will set a match value for OSTIMER with an optional callback. And this callback
 * will be called while the data in dedicated pair match register is equals to the value of central OS TIMER.
 *
 * @param base   OSTIMER peripheral base address.
 * @param count  OSTIMER timer match value.(Value is decimal format, and this value will be translate to Gray code
 * internally.)
 *
 * @param cb     OSTIMER callback (can be left as NULL if none, otherwise should be a void func(void)).
 * @return       none
 */
void OSTIMER_SetMatchValue(OSTIMER_Type *base, uint64_t count, ostimer_callback_t cb);

/*!
 * @brief Get current timer raw count value from OSTIMER.
 *
 * This function will get a gray code type timer count value from OS timer register.
 * The raw value of timer count is gray code format.
 *
 * @param base   OSTIMER peripheral base address.
 * @return       Raw value of OSTIMER, gray code format.
 */
static inline uint64_t OSTIMER_GetCurrentTimerRawValue(OSTIMER_Type *base)
{
    uint64_t tmp = 0U;

    tmp = base->EVTIMERL;
    tmp |= (uint64_t)(base->EVTIMERH) << 32U;

    return tmp;
}

/*!
 * @brief Get current timer count value from OSTIMER.
 *
 * This function will get a decimal timer count value.
 * The RAW value of timer count is gray code format, will be translated to decimal data internally.
 *
 * @param base   OSTIMER peripheral base address.
 * @return       Value of OSTIMER which will be formated to decimal value.
 */
uint64_t OSTIMER_GetCurrentTimerValue(OSTIMER_Type *base);

/*!
 * @brief Get the capture value from OSTIMER.
 *
 * This function will get a captured gray-code value from OSTIMER.
 * The Raw value of timer capture is gray code format.
 *
 * @param base   OSTIMER peripheral base address.
 * @return       Raw value of capture register, data format is gray code.
 */
static inline uint64_t OSTIMER_GetCaptureRawValue(OSTIMER_Type *base)
{
    uint64_t tmp = 0U;

    tmp = base->CAPTUREN_L;
    tmp |= (uint64_t)(base->CAPTUREN_H) << 32U;

    return tmp;
}

/*!
 * @brief Get the capture value from OSTIMER.
 *
 * This function will get a capture decimal-value from OSTIMER.
 * The RAW value of timer capture is gray code format, will be translated to decimal data internally.
 *
 * @param base   OSTIMER peripheral base address.
 * @return Value of capture register, data format is decimal.
 */
uint64_t OSTIMER_GetCaptureValue(OSTIMER_Type *base);

/*!
 * @brief OS timer interrupt Service Handler.
 *
 * This function handles the interrupt and refers to the callback array in the driver to callback user (as per request
 * in OSTIMER_SetMatchValue()).
 * if no user callback is scheduled, the interrupt will simply be cleared.
 *
 * @param base   OS timer peripheral base address.
 * @param cb     callback scheduled for this instance of OS timer
 * @return       none
 */
void OSTIMER_HandleIRQ(OSTIMER_Type *base, ostimer_callback_t cb);
/*!
 * @}
 */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* _FSL_OSTIMER_H_ */
