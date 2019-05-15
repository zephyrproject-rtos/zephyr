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
#define FSL_COMPONENT_ID "platform.drivers.powerquad_transform"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void PQ_TransformCFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_CFFT;
}

void PQ_TransformRFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    /* Set 0's for imaginary inputs as not be reading them in by the machine */
    base->GPREG[1] = 0;
    base->GPREG[3] = 0;
    base->GPREG[5] = 0;
    base->GPREG[7] = 0;
    base->GPREG[9] = 0;
    base->GPREG[11] = 0;
    base->GPREG[13] = 0;
    base->GPREG[15] = 0;
    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_RFFT;
}

void PQ_TransformIFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_IFFT;
}

void PQ_TransformCDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_CDCT;
}

void PQ_TransformRDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->GPREG[1] = 0;
    base->GPREG[3] = 0;
    base->GPREG[5] = 0;
    base->GPREG[7] = 0;
    base->GPREG[9] = 0;
    base->GPREG[11] = 0;
    base->GPREG[13] = 0;
    base->GPREG[15] = 0;
    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_RDCT;
}

void PQ_TransformIDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult)
{
    assert(pData);
    assert(pResult);

    base->OUTBASE = (int32_t)pResult;
    base->INABASE = (int32_t)pData;
    base->LENGTH = length;
    base->CONTROL = (CP_FFT << 4) | PQ_TRANS_IDCT;
}
