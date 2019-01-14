/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_FMEAS_H_
#define _FSL_FMEAS_H_

#include "fsl_common.h"

/*!
 * @addtogroup fmeas
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Defines LPC Frequency Measure driver version 2.0.0.
 *
 * Change log:
 * - Version 2.0.0
 *   - initial version
 */
#define FSL_FMEAS_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name FMEAS Functional Operation
 * @{
 */

/*!
 * @brief    Starts a frequency measurement cycle.
 *
 * @param    base : SYSCON peripheral base address.
 */
static inline void FMEAS_StartMeasure(SYSCON_Type *base)
{
    base->FREQMECTRL = 0;
    base->FREQMECTRL = (1UL << 31);
}

/*!
 * @brief    Indicates when a frequency measurement cycle is complete.
 *
 * @param    base : SYSCON peripheral base address.
 * @return   true if a measurement cycle is active, otherwise false.
 */
static inline bool FMEAS_IsMeasureComplete(SYSCON_Type *base)
{
    return (bool)((base->FREQMECTRL & (1UL << 31)) == 0);
}

/*!
 * @brief    Returns the computed value for a frequency measurement cycle
 *
 * @param    base         : SYSCON peripheral base address.
 * @param    refClockRate : Reference clock rate used during the frequency measurement cycle.
 *
 * @return   Frequency in Hz.
 */
uint32_t FMEAS_GetFrequency(SYSCON_Type *base, uint32_t refClockRate);

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_FMEAS_H_ */
