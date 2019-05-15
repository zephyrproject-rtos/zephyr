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
#define FSL_COMPONENT_ID "platform.drivers.powerquad_filter"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void PQ_VectorBiqaudDf2F32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_biquad0(*(int32_t *)pSrc++);
            *(int32_t *)pDst++ = _pq_readAdd0();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8BiquadDf2F32();
        PQ_EndVector();
    }
}

void PQ_VectorBiqaudDf2Fixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_biquad0_fx(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVector(pSrc, pDst, length);
        PQ_Vector8BiquadDf2Fixed32();
        PQ_EndVector();
    }
}

void PQ_VectorBiqaudDf2Fixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;
        while (remainderBy8--)
        {
            _pq_biquad0_fx(*pSrc++);
            *pDst++ = _pq_readAdd0_fx();
        }
    }

    if (length)
    {
        PQ_StartVectorFixed16(pSrc, pDst, length);
        PQ_Vector8BiquadDf2Fixed16();
        PQ_EndVector();
    }
}

void PQ_VectorBiqaudCascadeDf2F32(float *pSrc, float *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;

        PQ_Biquad1F32(&pSrc[0], &pDst[0]);

        for (int i = 1; i < remainderBy8; i++)
        {
            _pq_biquad0(*(int32_t *)&pSrc[i - 1]);
            _pq_biquad1(*(int32_t *)&pSrc[i]);
            *(int32_t *)&pDst[i - 1] = _pq_readAdd0();
            *(int32_t *)&pDst[i] = _pq_readAdd1();
        }

        PQ_BiquadF32(&pSrc[remainderBy8 - 1], &pDst[remainderBy8 - 1]);
    }

    if (length)
    {
        PQ_StartVector(&pSrc[remainderBy8], &pDst[remainderBy8], length);
        PQ_Vector8BiqaudDf2CascadeF32();
        PQ_EndVector();
    }
}

void PQ_VectorBiqaudCascadeDf2Fixed32(int32_t *pSrc, int32_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;

        _pq_biquad1_fx(pSrc[0]);
        pDst[0] = _pq_readAdd1_fx();

        for (int i = 1; i < remainderBy8; i++)
        {
            _pq_biquad0_fx(pSrc[i - 1]);
            _pq_biquad1_fx(pSrc[i]);
            pDst[i - 1] = _pq_readAdd0_fx();
            pDst[i] = _pq_readAdd1_fx();
        }

        _pq_biquad0_fx(pSrc[remainderBy8 - 1]);
        pDst[remainderBy8 - 1] = _pq_readAdd0_fx();
    }

    if (length)
    {
        PQ_StartVector(&pSrc[remainderBy8], &pDst[remainderBy8], length);
        PQ_Vector8BiqaudDf2CascadeFixed32();
        PQ_EndVector();
    }
}

void PQ_VectorBiqaudCascadeDf2Fixed16(int16_t *pSrc, int16_t *pDst, int32_t length)
{
    int32_t remainderBy8 = length % 8;

    if (remainderBy8)
    {
        length -= remainderBy8;

        _pq_biquad1_fx(pSrc[0]);
        pDst[0] = _pq_readAdd1_fx();

        for (int i = 1; i < remainderBy8; i++)
        {
            _pq_biquad0_fx(pSrc[i - 1]);
            _pq_biquad1_fx(pSrc[i]);
            pDst[i - 1] = _pq_readAdd0_fx();
            pDst[i] = _pq_readAdd1_fx();
        }

        _pq_biquad0_fx(pSrc[remainderBy8 - 1]);
        pDst[remainderBy8 - 1] = _pq_readAdd0_fx();
    }

    if (length)
    {
        PQ_StartVectorFixed16(&pSrc[remainderBy8], &pDst[remainderBy8], length);
        PQ_Vector8BiqaudDf2CascadeFixed16();
        PQ_EndVector();
    }
}

void PQ_BiquadBackUpInternalState(POWERQUAD_Type *base, int32_t biquad_num, pq_biquad_state_t *state)
{
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (0 == biquad_num)
    {
        state->param = *(volatile pq_biquad_param_t *)&base->GPREG[0];
        state->compreg = base->COMPREG[1];
    }
    else
    {
        state->param = *(volatile pq_biquad_param_t *)&base->GPREG[8];
        state->compreg = base->COMPREG[3];
    }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

void PQ_BiquadRestoreInternalState(POWERQUAD_Type *base, int32_t biquad_num, pq_biquad_state_t *state)
{
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    if (0 == biquad_num)
    {
        *(volatile pq_biquad_param_t *)&base->GPREG[0] = state->param;
        base->COMPREG[1] = state->compreg;
    }
    else
    {
        *(volatile pq_biquad_param_t *)&base->GPREG[8] = state->param;
        base->COMPREG[3] = state->compreg;
    }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

void PQ_FIR(
    POWERQUAD_Type *base, void *pAData, int32_t ALength, void *pBData, int32_t BLength, void *pResult, uint32_t opType)
{
    assert(pAData);
    assert(pBData);
    assert(pResult);

    base->INABASE = (uint32_t)pAData;
    base->INBBASE = (uint32_t)pBData;
    base->LENGTH = ((uint32_t)BLength << 16U) + (uint32_t)ALength;
    base->OUTBASE = (uint32_t)pResult;
    base->CONTROL = (CP_FIR << 4U) | opType;
}

void PQ_FIRIncrement(POWERQUAD_Type *base, int32_t ALength, int32_t BLength, int32_t xOffset)
{
    base->MISC = xOffset;
    base->LENGTH = ((uint32_t)BLength << 16) + (uint32_t)ALength;
    base->CONTROL = (CP_FIR << 4) | PQ_FIR_INCREMENTAL;
}

void PQ_BiquadCascadeDf2Init(pq_biquad_cascade_df2_instance *S, uint8_t numStages, pq_biquad_state_t *pState)
{
    S->numStages = numStages;
    S->pState = pState;
}

void PQ_BiquadCascadeDf2F32(const pq_biquad_cascade_df2_instance *S, float *pSrc, float *pDst, uint32_t blockSize)
{
    uint32_t stage = S->numStages;
    pq_biquad_state_t *states = S->pState;

    if (pDst != pSrc)
    {
        memcpy(pDst, pSrc, 4 * blockSize);
    }

    if (stage % 2 != 0)
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);

        PQ_VectorBiqaudDf2F32(pSrc, pDst, blockSize);

        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);

        states++;
        stage--;
    }

    do
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 1, states);
        states++;
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);

        PQ_VectorBiqaudCascadeDf2F32(pDst, pDst, blockSize);

        states--;
        PQ_BiquadBackUpInternalState(POWERQUAD, 1, states);
        states++;
        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);

        states++;
        stage -= 2U;

    } while (stage > 0U);
}

void PQ_BiquadCascadeDf2Fixed32(const pq_biquad_cascade_df2_instance *S,
                                int32_t *pSrc,
                                int32_t *pDst,
                                uint32_t blockSize)
{
    uint32_t stage = S->numStages;
    pq_biquad_state_t *states = S->pState;

    if (pDst != pSrc)
    {
        memcpy(pDst, pSrc, 4 * blockSize);
    }

    if (stage % 2 != 0)
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);

        PQ_VectorBiqaudDf2Fixed32(pSrc, pDst, blockSize);

        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);

        states++;
        stage--;
    }

    do
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);
        states++;
        PQ_BiquadRestoreInternalState(POWERQUAD, 1, states);

        PQ_VectorBiqaudCascadeDf2Fixed32(pDst, pDst, blockSize);

        states--;
        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);
        states++;
        PQ_BiquadBackUpInternalState(POWERQUAD, 1, states);

        states++;
        stage -= 2U;
    } while (stage > 0U);
}

void PQ_BiquadCascadeDf2Fixed16(const pq_biquad_cascade_df2_instance *S,
                                int16_t *pSrc,
                                int16_t *pDst,
                                uint32_t blockSize)
{
    uint32_t stage = S->numStages;
    pq_biquad_state_t *states = S->pState;

    if (pDst != pSrc)
    {
        memcpy(pDst, pSrc, 2 * blockSize);
    }

    if (stage % 2 != 0)
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);

        PQ_VectorBiqaudDf2Fixed16(pSrc, pDst, blockSize);

        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);

        states++;
        stage--;
    }

    do
    {
        PQ_BiquadRestoreInternalState(POWERQUAD, 0, states);
        states++;
        PQ_BiquadRestoreInternalState(POWERQUAD, 1, states);

        PQ_VectorBiqaudCascadeDf2Fixed16(pDst, pDst, blockSize);

        states--;
        PQ_BiquadBackUpInternalState(POWERQUAD, 0, states);
        states++;
        PQ_BiquadBackUpInternalState(POWERQUAD, 1, states);

        states++;
        stage -= 2U;
    } while (stage > 0U);
}
