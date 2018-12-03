/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_MMDVSQ_H_
#define _FSL_MMDVSQ_H_

#include "fsl_common.h"

/*!
 * @addtogroup mmdvsq
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_MMSVSQ_DRIVER_VERSION (MAKE_VERSION(2, 0, 2)) /*!< Version 2.0.2. */
/*@}*/

/*! @brief MMDVSQ execution status */
typedef enum _mmdvsq_execution_status
{
    kMMDVSQ_IdleSquareRoot = 0x01U, /*!< MMDVSQ is idle; the last calculation was a square root */
    kMMDVSQ_IdleDivide = 0x02U,     /*!< MMDVSQ is idle; the last calculation was division */
    kMMDVSQ_BusySquareRoot = 0x05U, /*!< MMDVSQ is busy processing a square root calculation */
    kMMDVSQ_BusyDivide = 0x06U      /*!< MMDVSQ is busy processing a division calculation */
} mmdvsq_execution_status_t;

/*! @brief MMDVSQ divide fast start select */
typedef enum _mmdvsq_fast_start_select
{
    kMMDVSQ_EnableFastStart = 0U, /*!< Division operation is initiated by a write to the DSOR register */
    kMMDVSQ_DisableFastStart =
        1U /*!< Division operation is initiated by a write to CSR[SRT] = 1; normal start instead fast start */
} mmdvsq_fast_start_select_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name MMDVSQ functional Operation
 * @{
 */

/*!
 * @brief Performs the MMDVSQ division operation and returns the remainder.
 *
 * @param   base        MMDVSQ peripheral address
 * @param   dividend    Dividend value
 * @param   divisor     Divisor value
 * @param   isUnsigned  Mode of unsigned divide
 *                      - true   unsigned divide
 *                      - false  signed divide
 *
 */
int32_t MMDVSQ_GetDivideRemainder(MMDVSQ_Type *base, int32_t dividend, int32_t divisor, bool isUnsigned);

/*!
 * @brief Performs the MMDVSQ division operation and returns the quotient.
 *
 * @param   base        MMDVSQ peripheral address
 * @param   dividend    Dividend value
 * @param   divisor     Divisor value
 * @param   isUnsigned  Mode of unsigned divide
 *                      - true   unsigned divide
 *                      - false  signed divide
 *
 */
int32_t MMDVSQ_GetDivideQuotient(MMDVSQ_Type *base, int32_t dividend, int32_t divisor, bool isUnsigned);

/*!
 * @brief Performs the MMDVSQ square root operation.
 *
 * This function performs the MMDVSQ square root operation and returns the square root
 * result of a given radicand value.
 *
 * @param   base        MMDVSQ peripheral address
 * @param   radicand    Radicand value
 *
 */
uint16_t MMDVSQ_Sqrt(MMDVSQ_Type *base, uint32_t radicand);

/* @} */

/*!
 * @name MMDVSQ status Operation
 * @{
 */

/*!
 * @brief Gets the MMDVSQ execution status.
 *
 * This function checks the current MMDVSQ execution status of the combined
 * CSR[BUSY, DIV, SQRT] indicators.
 *
 * @param   base       MMDVSQ peripheral address
 *
 * @return  Current MMDVSQ execution status
 */
static inline mmdvsq_execution_status_t MMDVSQ_GetExecutionStatus(MMDVSQ_Type *base)
{
    return (mmdvsq_execution_status_t)(base->CSR >> MMDVSQ_CSR_SQRT_SHIFT);
}

/*!
 * @brief Configures  MMDVSQ fast start mode.
 *
 * This function sets the MMDVSQ division fast start. The MMDVSQ supports two
 * mechanisms for initiating a division operation. The default mechanism is
 * a “fast start” where a write to the DSOR register begins the division.
 * Alternatively, the start mechanism can begin after a write to the CSR
 * register with CSR[SRT] set.
 *
 * @param   base        MMDVSQ peripheral address
 * @param   mode        Mode of Divide-Fast-Start
 *                      - kMmdvsqDivideFastStart   = 0
 *                      - kMmdvsqDivideNormalStart = 1
 */
static inline void MMDVSQ_SetFastStartConfig(MMDVSQ_Type *base, mmdvsq_fast_start_select_t mode)
{
    if (mode)
    {
        base->CSR |= MMDVSQ_CSR_DFS_MASK;
    }
    else
    {
        base->CSR &= ~MMDVSQ_CSR_DFS_MASK;
    }
}

/*!
 * @brief Configures the MMDVSQ divide-by-zero mode.
 *
 * This function configures the MMDVSQ response to divide-by-zero
 * calculations. If both CSR[DZ] and CSR[DZE] are set, then a subsequent read
 * of the RES register is error-terminated to signal the processor of the
 * attempted divide-by-zero. Otherwise, the register contents are returned.
 *
 * @param   base           MMDVSQ peripheral address
 * @param   isDivByZero    Mode of Divide-By-Zero
 *                          - kMmdvsqDivideByZeroDis = 0
 *                          - kMmdvsqDivideByZeroEn  = 1
 */
static inline void MMDVSQ_SetDivideByZeroConfig(MMDVSQ_Type *base, bool isDivByZero)
{
    if (isDivByZero)
    {
        base->CSR |= MMDVSQ_CSR_DZE_MASK;
    }
    else
    {
        base->CSR &= ~MMDVSQ_CSR_DZE_MASK;
    }
}

/* @} */

#if defined(__cplusplus)
}

#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_MMDVSQ_H_ */
