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
#define FSL_COMPONENT_ID "platform.drivers.powerquad_math"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void PQ_VectorLnF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_ln0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readAdd0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_LN, 1, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorInvF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_inv0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readMult0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_INV, 0, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorSqrtF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sqrt0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readMult0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_SQRT, 0, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorInvSqrtF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_invsqrt0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readMult0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_INVSQRT, 0, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorEtoxF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etox0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readMult0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_ETOX, 0, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorEtonxF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etonx0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readMult0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_ETONX, 0, PQ_TRANS);
        PQ_EndVector();
    }
}

void PQ_VectorSinF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sin0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readAdd0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_SIN, 1, PQ_TRIG);
        PQ_EndVector();
    }
}

void PQ_VectorCosF32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_cos0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readAdd0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8F32(PQ_COS, 1, PQ_TRIG);
        PQ_EndVector();
    }
}

void PQ_VectorLnFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_ln_fx0(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_LN, 1, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorInvFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_inv_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_INV, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorSqrtFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sqrt_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_SQRT, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorInvSqrtFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_invsqrt_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_INVSQRT, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorEtoxFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etox_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_ETOX, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorEtonxFixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etonx_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_ETONX, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorSinQ31(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    const int32_t magic = 0x30c90fdb;
    float valFloat;
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            valFloat = *(const float *)(&magic) * (float)(*pSrc++);
            _pq_sin0(*(int32_t *)(&valFloat));
            _pq_readAdd0();
            *pDst++ = (_pq_readAdd0_fx());
        }
    }

    while (length > 0)
    {
        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        length -= 8;
    }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#else
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sin_fx0(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_SIN, 1, PQ_TRIG_FIXED);
        PQ_EndVector();
    }
#endif

    POWERQUAD->CPPRE = cppre;
}

void PQ_VectorCosQ31(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    const int32_t magic = 0x30c90fdb;
    float valFloat;
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            valFloat = *(const float *)(&magic) * (float)(*pSrc++);
            _pq_cos0(*(int32_t *)(&valFloat));
            _pq_readAdd0();
            *pDst++ = (_pq_readAdd0_fx());
        }
    }

    while (length > 0)
    {
        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        valFloat = *(const float *)(&magic) * (float)(*pSrc++);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx());

        length -= 8;
    }

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#else
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_cos_fx0(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8Fixed32(PQ_COS, 1, PQ_TRIG_FIXED);
        PQ_EndVector();
    }
#endif

    POWERQUAD->CPPRE = cppre;
}

void PQ_VectorLnFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_ln_fx0(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_LN, 1, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorInvFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_inv_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_INV, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorSqrtFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sqrt_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_SQRT, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorInvSqrtFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_invsqrt_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_INVSQRT, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorEtoxFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etox_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_ETOX, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorEtonxFixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_etonx_fx0(*pSrc++);
            *pDst++ = _pq_readMult0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8Fixed16(PQ_ETONX, 0, PQ_TRANS_FIXED);
        PQ_EndVector();
    }
}

void PQ_VectorSinQ15(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    const int32_t magic = 0x30c90fdb;
    float valFloat;
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

    int32_t remainderBy8 = length % 8;

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
            _pq_sin0(*(int32_t *)(&valFloat));
            _pq_readAdd0();
            *pDst++ = (_pq_readAdd0_fx()) >> 16;
        }
    }

    while (length > 0)
    {
        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_sin0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        length -= 8;
    }

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#else

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_sin_fx0((uint32_t)(*pSrc++) << 16);
            *pDst++ = (_pq_readAdd0_fx()) >> 16;
        }
    }

    if (length)
    {
        PQ_StartVectorQ15(pSrc, pDst, length);
        PQ_Vector8Q15(PQ_SIN, 1, PQ_TRIG_FIXED);
        PQ_EndVector();
    }
#endif

    POWERQUAD->CPPRE = cppre;
}

void PQ_VectorCosQ15(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    const int32_t magic = 0x30c90fdb;
    float valFloat;
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

    int32_t remainderBy8 = length % 8;

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
            _pq_cos0(*(int32_t *)(&valFloat));
            _pq_readAdd0();
            *pDst++ = (_pq_readAdd0_fx()) >> 16;
        }
    }

    while (length > 0)
    {
        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        valFloat = *(const float *)(&magic) * (float)((uint32_t)(*pSrc++) << 16);
        _pq_cos0(*(int32_t *)(&valFloat));
        _pq_readAdd0();
        *pDst++ = (_pq_readAdd0_fx()) >> 16;

        length -= 8;
    }

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#else

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_cos_fx0((uint32_t)(*pSrc++) << 16);
            *pDst++ = (_pq_readAdd0_fx()) >> 16;
        }
    }

    if (length)
    {
        PQ_StartVectorQ15(pSrc, pDst, length);
        PQ_Vector8Q15(PQ_COS, 1, PQ_TRIG_FIXED);
        PQ_EndVector();
    }
#endif

    POWERQUAD->CPPRE = cppre;
}

int32_t PQ_ArctanFixed(POWERQUAD_Type *base, int32_t x, int32_t y, pq_cordic_iter_t iteration)
{
    base->CORDIC_X = x;
    base->CORDIC_Y = y;
    base->CORDIC_Z = 0;
    base->CONTROL = (CP_CORDIC << 4) | CORDIC_ARCTAN | CORDIC_ITER(iteration);

    PQ_WaitDone(base);
    return base->CORDIC_Z;
}

int32_t PQ_ArctanhFixed(POWERQUAD_Type *base, int32_t x, int32_t y, pq_cordic_iter_t iteration)
{
    base->CORDIC_X = x;
    base->CORDIC_Y = y;
    base->CORDIC_Z = 0;
    base->CONTROL = (CP_CORDIC << 4) | CORDIC_ARCTANH | CORDIC_ITER(iteration);

    PQ_WaitDone(base);
    return base->CORDIC_Z;
}
