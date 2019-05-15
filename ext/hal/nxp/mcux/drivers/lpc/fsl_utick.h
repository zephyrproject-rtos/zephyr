/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_UTICK_H_
#define _FSL_UTICK_H_

#include "fsl_common.h"
/*!
 * @addtogroup utick
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief UTICK driver version 2.0.2. */
#define FSL_UTICK_DRIVER_VERSION (MAKE_VERSION(2, 0, 2))
/*@}*/

/*! @brief UTICK timer operational mode. */
typedef enum _utick_mode
{
    kUTICK_Onetime = 0x0U, /*!< Trigger once*/
    kUTICK_Repeat = 0x1U,  /*!< Trigger repeatedly */
} utick_mode_t;

/*! @brief UTICK callback function. */
typedef void (*utick_callback_t)(void);

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
* @brief Initializes an UTICK by turning its bus clock on
*
*/
void UTICK_Init(UTICK_Type *base);

/*!
 * @brief Deinitializes a UTICK instance.
 *
 * This function shuts down Utick bus clock
 *
 * @param base UTICK peripheral base address.
 */
void UTICK_Deinit(UTICK_Type *base);
/*!
 * @brief Get Status Flags.
 *
 * This returns the status flag
 *
 * @param base UTICK peripheral base address.
 * @return status register value
 */
uint32_t UTICK_GetStatusFlags(UTICK_Type *base);
/*!
 * @brief Clear Status Interrupt Flags.
 *
 * This clears intr status flag
 *
 * @param base UTICK peripheral base address.
 * @return none
 */
void UTICK_ClearStatusFlags(UTICK_Type *base);

/*!
 * @brief Starts UTICK.
 *
 * This function starts a repeat/onetime countdown with an optional callback
 *
 * @param base   UTICK peripheral base address.
 * @param mode  UTICK timer mode (ie kUTICK_onetime or kUTICK_repeat)
 * @param count  UTICK timer mode (ie kUTICK_onetime or kUTICK_repeat)
 * @param cb  UTICK callback (can be left as NULL if none, otherwise should be a void func(void))
 * @return none
 */
void UTICK_SetTick(UTICK_Type *base, utick_mode_t mode, uint32_t count, utick_callback_t cb);
/*!
 * @brief UTICK Interrupt Service Handler.
 *
 * This function handles the interrupt and refers to the callback array in the driver to callback user (as per request
 * in UTICK_SetTick()).
 * if no user callback is scheduled, the interrupt will simply be cleared.
 *
 * @param base   UTICK peripheral base address.
 * @param cb  callback scheduled for this instance of UTICK
 * @return none
 */
void UTICK_HandleIRQ(UTICK_Type *base, utick_callback_t cb);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_UTICK_H_ */
