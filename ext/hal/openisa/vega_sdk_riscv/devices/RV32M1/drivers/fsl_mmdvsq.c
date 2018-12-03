/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_mmdvsq.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

int32_t MMDVSQ_GetDivideRemainder(MMDVSQ_Type *base, int32_t dividend, int32_t divisor, bool isUnsigned)
{
    uint32_t temp = 0;

    temp = base->CSR;
    temp &= ~(MMDVSQ_CSR_USGN_MASK | MMDVSQ_CSR_REM_MASK);
    /* Prepare setting for calculation */
    temp |= MMDVSQ_CSR_USGN(isUnsigned) | MMDVSQ_CSR_REM(true);
    /* Write setting to CSR register */
    base->CSR = temp;
    /* Write dividend to DEND register */
    base->DEND = dividend;
    /* Write divisor to DSOR register and start calculation if Fast-Start is enabled */
    base->DSOR = divisor;
    /* Start calculation by writing 1 to SRT bit in case Fast-Start is disabled */
    base->CSR |= MMDVSQ_CSR_SRT_MASK;
    /* Return remainder, if divide-by-zero is enabled and occurred, reading from
    * RES result is error terminated */
    return base->RES;
}

int32_t MMDVSQ_GetDivideQuotient(MMDVSQ_Type *base, int32_t dividend, int32_t divisor, bool isUnsigned)
{
    uint32_t temp = 0;

    temp = base->CSR;
    temp &= ~(MMDVSQ_CSR_USGN_MASK | MMDVSQ_CSR_REM_MASK);
    /* Prepare setting for calculation */
    temp |= MMDVSQ_CSR_USGN(isUnsigned) | MMDVSQ_CSR_REM(false);
    /* Write setting mode to CSR register */
    base->CSR = temp;
    /* Write dividend to DEND register */
    base->DEND = dividend;
    /* Write divisor to DSOR register and start calculation when Fast-Start is enabled */
    base->DSOR = divisor;
    /* Start calculation by writing 1 to SRT bit in case Fast-Start is disabled */
    base->CSR |= MMDVSQ_CSR_SRT_MASK;
    /* Return quotient, if divide-by-zero is enabled and occurred, reading from
    * RES result is error terminated */
    return base->RES;
}

uint16_t MMDVSQ_Sqrt(MMDVSQ_Type *base, uint32_t radicand)
{
    /* Write radicand to RCND register , and start calculation */
    base->RCND = radicand;
    /* Return result */
    return base->RES;
}
