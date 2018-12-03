/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_TSTMR_H_
#define _FSL_TSTMR_H_

#include "fsl_common.h"

/*!
 * @addtogroup tstmr_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_TSTMR_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
                                                         /*@}*/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Reads the time stamp.
 *
 * This function reads the low and high registers and returns the 56-bit free running
 * counter value. This can be read by software at any time to determine the software ticks.
 *
 * @param base TSTMR peripheral base address.
 *
 * @return The 56-bit time stamp value.
 */
static inline uint64_t TSTMR_ReadTimeStamp(TSTMR_Type* base)
{
    return *(volatile uint64_t*)(base);
}

/*!
 * @brief Delays for a specified number of microseconds.
 *
 * This function repeatedly reads the timestamp register and waits for the user-specified
 * delay value.
 *
 * @param base      TSTMR peripheral base address.
 * @param delayInUs Delay value in microseconds.
 */
static inline void TSTMR_DelayUs(TSTMR_Type* base, uint32_t delayInUs)
{
    uint64_t startTime = TSTMR_ReadTimeStamp(base);
#if defined(FSL_FEATURE_TSTMR_CLOCK_FREQUENCY_1MHZ) && FSL_FEATURE_TSTMR_CLOCK_FREQUENCY_1MHZ
    while (TSTMR_ReadTimeStamp(base) - startTime < delayInUs)
#elif defined(FSL_FEATURE_TSTMR_CLOCK_FREQUENCY_8MHZ) && FSL_FEATURE_TSTMR_CLOCK_FREQUENCY_8MHZ
    while (TSTMR_ReadTimeStamp(base) - startTime < 8 * delayInUs)
#else
    assert(0);
#endif
    {
    }
}

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_TSTMR_H_ */
