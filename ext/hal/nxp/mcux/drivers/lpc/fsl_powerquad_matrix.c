/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_powerquad.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.powerquad_matrix"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void PQ_MatrixAddition(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pAData;
    base->INBBASE = (int32_t)pBData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_ADD;
}

void PQ_MatrixSubtraction(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pAData;
    base->INBBASE = (int32_t)pBData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_SUB;
}

void PQ_MatrixMultiplication(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pAData;
    base->INBBASE = (int32_t)pBData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_MULT;
}

void PQ_MatrixProduct(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pAData;
    base->INBBASE = (int32_t)pBData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_PROD;
}

void PQ_VectorDotProduct(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pAData;
    base->INBBASE = (int32_t)pBData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_VEC_DOTP;
}

void PQ_MatrixInversion(POWERQUAD_Type *base, uint32_t length, void *pData, void *pTmpData, void *pResult)
{
    assert(pData);
    assert(pTmpData);
    assert(pResult);

    /* Workaround:
     *
     * Matrix inv depends on the coproc 1/x function, this puts coproc to right state.
     */
    _pq_inv0(1.0);

    base->INABASE = (uint32_t)pData;
    base->TMPBASE = (uint32_t)pTmpData;
    base->OUTBASE = (uint32_t)pResult;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_INV;
}

void PQ_MatrixTranspose(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_TRAN;
}

void PQ_MatrixScale(POWERQUAD_Type *base, uint32_t length, float misc, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    base->MISC = *(uint32_t *)&misc;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    base->CONTROL = (CP_MTX << 4) | PQ_MTX_SCALE;
}
