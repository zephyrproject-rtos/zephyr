/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_q31.c
 * Description:  Q31 FIR filter processing function
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2019 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "arm_math.h"

/**
  @ingroup groupFilters
 */

/**
  @addtogroup FIR
  @{
 */

/**
  @brief         Processing function for Q31 FIR filter.
  @param[in]     S          points to an instance of the Q31 FIR filter structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The accumulator has a 2.62 format and maintains full precision of the intermediate multiplication results but provides only a single guard bit.
                   Thus, if the accumulator result overflows it wraps around rather than clip.
                   In order to avoid overflows completely the input signal must be scaled down by log2(numTaps) bits.
                   After all multiply-accumulates are performed, the 2.62 accumulator is right shifted by 31 bits and saturated to 1.31 format to yield the final result.

 @remark
                   Refer to \ref arm_fir_fast_q31() for a faster but less precise implementation of this filter.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"
                                        


static void arm_fir_q31_1_4_mve(const arm_fir_instance_q31 * S, const q31_t * pSrc, q31_t * pDst, uint32_t blockSize)
{
    q31_t    *pState = S->pState;   /* State pointer */
    const q31_t    *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q31_t    *pStateCur;        /* Points to the current sample of the state */
    const q31_t    *pSamples;         /* Temporary pointer to the sample buffer */
    q31_t    *pOutput;          /* Temporary pointer to the output buffer */
    const q31_t    *pTempSrc;         /* Temporary pointer to the source data */
    q31_t    *pTempDest;        /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    q31x4_t vecIn0;

    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    pStateCur = &(pState[(numTaps - 1u)]);
    pTempSrc = pSrc;
    pSamples = pState;
    pOutput = pDst;

    q63_t     acc0, acc1, acc2, acc3;
    /*
     * load 4 coefs
     */
    q31x4_t vecCoeffs = *(q31x4_t *) pCoeffs;

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        const q31_t    *pSamplesTmp = pSamples;

        /*
         * Save 4 input samples in the history buffer
         */
        vst1q(pStateCur, vld1q(pTempSrc));
        pStateCur += 4;
        pTempSrc += 4;

        vecIn0 = vld1q(pSamplesTmp);
        acc0 = vrmlaldavhq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamplesTmp[1]);
        acc1 = vrmlaldavhq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamplesTmp[2]);
        acc2 = vrmlaldavhq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamplesTmp[3]);
        acc3 = vrmlaldavhq(vecIn0, vecCoeffs);

        acc0 = asrl(acc0, 23);
        acc1 = asrl(acc1, 23);
        acc2 = asrl(acc2, 23);
        acc3 = asrl(acc3, 23);

        *pOutput++ = (q31_t) acc0;
        *pOutput++ = (q31_t) acc1;
        *pOutput++ = (q31_t) acc2;
        *pOutput++ = (q31_t) acc3;

        pSamples += 4;
        /*
         * Decrement the sample block loop counter
         */
        blkCnt--;
    }

    uint32_t  residual = blockSize & 3;
    
    switch (residual)
    {
    case 3:
        {
            /*
             * Save 4 input samples in the history buffer
             */

            *(q31x4_t *) pStateCur = *(q31x4_t *) pTempSrc;
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vrmlaldavhq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[2]);
            acc2 = vrmlaldavhq(vecIn0, vecCoeffs);

            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);
            acc2 = asrl(acc2, 23);

            *pOutput++ = (q31_t) acc0;
            *pOutput++ = (q31_t) acc1;
            *pOutput++ = (q31_t) acc2;
        }
        break;

    case 2:
        {
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vrmlaldavhq(vecIn0, vecCoeffs);

            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);

            *pOutput++ = (q31_t) acc0;
            *pOutput++ = (q31_t) acc1;
        }
        break;

    case 1:
        {
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs);

            acc0 = asrl(acc0, 23);

            *pOutput++ = (q31_t) acc0;
        }
        break;
    }


    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;

    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_s32(pTempDest, vld1q(pTempSrc), p0);
    }
}

static void arm_fir_q31_5_8_mve(const arm_fir_instance_q31 * S, const q31_t * pSrc, q31_t * pDst, uint32_t blockSize)
{
    q31_t    *pState = S->pState;   /* State pointer */
    const q31_t    *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q31_t    *pStateCur;        /* Points to the current sample of the state */
    const q31_t    *pSamples;         /* Temporary pointer to the sample buffer */
    q31_t    *pOutput;          /* Temporary pointer to the output buffer */
    const q31_t    *pTempSrc;         /* Temporary pointer to the source data */
    q31_t    *pTempDest;        /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    q31x4_t vecIn0;
    q63_t     acc0, acc1, acc2, acc3;
    q31x4_t vecCoeffs1_4, vecCoeffs5_8;

    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    pStateCur = &(pState[(numTaps - 1u)]);
    pTempSrc = pSrc;
    pSamples = pState;
    pOutput = pDst;


    /*
     * load 8 coefs
     */
    vecCoeffs1_4 = *(q31x4_t *) pCoeffs;
    vecCoeffs5_8 = *(q31x4_t *) (pCoeffs + 4);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        const q31_t    *pSamplesTmp = pSamples;

        /*
         * Save 4 input samples in the history buffer
         */
        vst1q(pStateCur, vld1q(pTempSrc));

        vecIn0 = vld1q(pSamplesTmp);
        acc0 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

        vecIn0 = vld1q(&pSamplesTmp[1]);
        acc1 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

        vecIn0 = vld1q(&pSamplesTmp[2]);
        acc2 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

        vecIn0 = vld1q(&pSamplesTmp[3]);
        acc3 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

        vecIn0 = vld1q(&pSamplesTmp[4]);
        acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs5_8);

        vecIn0 = vld1q(&pSamplesTmp[5]);
        acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs5_8);

        vecIn0 = vld1q(&pSamplesTmp[6]);
        acc2 = vrmlaldavhaq(acc2, vecIn0, vecCoeffs5_8);

        vecIn0 = vld1q(&pSamplesTmp[7]);
        acc3 = vrmlaldavhaq(acc3, vecIn0, vecCoeffs5_8);


        acc0 = asrl(acc0, 23);
        acc1 = asrl(acc1, 23);
        acc2 = asrl(acc2, 23);
        acc3 = asrl(acc3, 23);

        *pOutput++ = (q31_t) acc0;
        *pOutput++ = (q31_t) acc1;
        *pOutput++ = (q31_t) acc2;
        *pOutput++ = (q31_t) acc3;

        pSamples += 4;
        pStateCur += 4;
        pTempSrc += 4;

        /*
         * Decrement the sample block loop counter
         */
        blkCnt--;
    }

    uint32_t  residual = blockSize & 3;
    switch (residual)
    {
    case 3:
        {
            /*
             * Save 4 input samples in the history buffer
             */
            *(q31x4_t *) pStateCur = *(q31x4_t *) pTempSrc;
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[2]);
            acc2 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[4]);
            acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs5_8);

            vecIn0 = vld1q(&pSamples[5]);
            acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs5_8);

            vecIn0 = vld1q(&pSamples[6]);
            acc2 = vrmlaldavhaq(acc2, vecIn0, vecCoeffs5_8);

            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);
            acc2 = asrl(acc2, 23);

            *pOutput++ = (q31_t) acc0;
            *pOutput++ = (q31_t) acc1;
            *pOutput++ = (q31_t) acc2;
        }
        break;

    case 2:
        {
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[4]);
            acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs5_8);

            vecIn0 = vld1q(&pSamples[5]);
            acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs5_8);

            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);

            *pOutput++ = (q31_t) acc0;
            *pOutput++ = (q31_t) acc1;
        }
        break;

    case 1:
        {
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            acc0 = vrmlaldavhq(vecIn0, vecCoeffs1_4);

            vecIn0 = vld1q(&pSamples[4]);
            acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs5_8);

            acc0 = asrl(acc0, 23);

            *pOutput++ = (q31_t) acc0;
        }
        break;
    }

    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;

    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_s32(pTempDest, vld1q(pTempSrc), p0);
    }
}

void arm_fir_q31(
  const arm_fir_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
    q31_t    *pState = S->pState;   /* State pointer */
    const q31_t    *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q31_t    *pStateCur;        /* Points to the current sample of the state */
    const q31_t    *pSamples;         /* Temporary pointer to the sample buffer */
    q31_t    *pOutput;          /* Temporary pointer to the output buffer */
    const q31_t    *pTempSrc;         /* Temporary pointer to the source data */
    q31_t    *pTempDest;        /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    q31x4_t vecIn0;
    uint32_t  tapsBlkCnt = (numTaps + 3) / 4;
    q63_t     acc0, acc1, acc2, acc3;
    q31x4_t vecCoeffs;

    /*
     * [1 to 8 taps] specialized routines
     */
    
    if (blockSize >= 8)
    {
        if (numTaps <= 4)
        {
            arm_fir_q31_1_4_mve(S, pSrc, pDst, blockSize);
            return;
        }
        else if (numTaps <= 8)
        {
            arm_fir_q31_5_8_mve(S, pSrc, pDst, blockSize);
            return;
        }
    }


    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    if (blockSize >= 8)
    {
        pStateCur   = &(pState[(numTaps - 1u)]);
        pSamples    = pState;
        pTempSrc    = pSrc;
        pOutput     = pDst;
        blkCnt      = blockSize >> 2;
        while (blkCnt > 0U)
        {
            const q31_t    *pCoeffsTmp = pCoeffs;
            const q31_t    *pSamplesTmp = pSamples;
    
            acc0 = 0LL;
            acc1 = 0LL;
            acc2 = 0LL;
            acc3 = 0LL;
    
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;
    
            tapsBlkCnt = (numTaps ) / 4;
            uint32_t       i = tapsBlkCnt ;
            while (i > 0U)
            {
                /*
                 * load 4 coefs
                 */
                vecCoeffs = *(q31x4_t *) pCoeffsTmp;
    
                vecIn0 = vld1q(pSamplesTmp);
                acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs);
    
                vecIn0 = vld1q(&pSamplesTmp[1]);
                acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs);
    
                vecIn0 = vld1q(&pSamplesTmp[2]);
                acc2 = vrmlaldavhaq(acc2, vecIn0, vecCoeffs);
    
                vecIn0 = vld1q(&pSamplesTmp[3]);
                acc3 = vrmlaldavhaq(acc3, vecIn0, vecCoeffs);
    
                pSamplesTmp += 4;
                pCoeffsTmp += 4;
                /*
                 * Decrement the taps block loop counter
                 */
                i--;
            }

            tapsBlkCnt = (numTaps ) & 3;
            i = tapsBlkCnt ;
            while (i > 0U)
            {
                /*
                 * load 4 coefs
                 */

                /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
                acc0 += ((q63_t) *pSamplesTmp * *pCoeffsTmp) >> 8;
                acc1 += ((q63_t) pSamplesTmp[1] * *pCoeffsTmp) >> 8;
                acc2 += ((q63_t) pSamplesTmp[2] * *pCoeffsTmp) >> 8;
                acc3 += ((q63_t) pSamplesTmp[3] * *pCoeffsTmp) >> 8;

    
                pSamplesTmp += 1;
                pCoeffsTmp += 1;
                /*
                 * Decrement the taps block loop counter
                 */
                i--;
            }
    
            /* .54-> .31 conversion and store accumulators */
            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);
            acc2 = asrl(acc2, 23);
            acc3 = asrl(acc3, 23);
    
            *pOutput++ = (q31_t) acc0;
            *pOutput++ = (q31_t) acc1;
            *pOutput++ = (q31_t) acc2;
            *pOutput++ = (q31_t) acc3;
    
            pSamples += 4;

            
            /*
             * Decrement the sample block loop counter
             */
            blkCnt--;
        }
    
        uint32_t  residual = blockSize & 3;
        switch (residual)
        {
        case 3:
            {
                const q31_t    *pCoeffsTmp = pCoeffs;
                const q31_t    *pSamplesTmp = pSamples;
    
                acc0 = 0LL;
                acc1 = 0LL;
                acc2 = 0LL;
    
                /*
                 * Save 4 input samples in the history buffer
                 */
              
                *(q31x4_t *) pStateCur = *(q31x4_t *) pTempSrc;
                pStateCur += 4;
                pTempSrc += 4;
    
                tapsBlkCnt = numTaps  / 4;
                uint32_t       i = tapsBlkCnt;
                while (i > 0U)
                {
                    vecCoeffs = *(q31x4_t *) pCoeffsTmp;

                    vecIn0 = vld1q(pSamplesTmp);
                    acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs);
    
                    vecIn0 = vld1q(&pSamplesTmp[1]);
                    acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs);
    
                    vecIn0 = vld1q(&pSamplesTmp[2]);
                    acc2 = vrmlaldavhaq(acc2, vecIn0, vecCoeffs);
    
                    pSamplesTmp += 4;
                    pCoeffsTmp += 4;
                    i--;
                }

                tapsBlkCnt = (numTaps ) & 3;
                
                i = tapsBlkCnt ;
                while (i > 0U)
                {
                   
                    /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
                    acc0 += ((q63_t) *pSamplesTmp * *pCoeffsTmp) >> 8;
                    acc1 += ((q63_t) pSamplesTmp[1] * *pCoeffsTmp) >> 8;
                    acc2 += ((q63_t) pSamplesTmp[2] * *pCoeffsTmp) >> 8;
    
                    pSamplesTmp += 1;
                    pCoeffsTmp += 1;
                    /*
                     * Decrement the taps block loop counter
                     */
                    i--;
                }
    
    
                acc0 = asrl(acc0, 23);
                acc1 = asrl(acc1, 23);
                acc2 = asrl(acc2, 23);
    
                *pOutput++ = (q31_t) acc0;
                *pOutput++ = (q31_t) acc1;
                *pOutput++ = (q31_t) acc2;
            }
            break;
    
        case 2:
            {
                const q31_t    *pCoeffsTmp = pCoeffs;
                const q31_t    *pSamplesTmp = pSamples;
    
                acc0 = 0LL;
                acc1 = 0LL;
    
                /*
                 * Save 4 input samples in the history buffer
                 */
                vst1q(pStateCur, vld1q(pTempSrc));
                pStateCur += 4;
                pTempSrc += 4;
    
                tapsBlkCnt = (numTaps ) / 4;
                uint32_t       i = tapsBlkCnt;
                while (i > 0U)
                {
                    vecCoeffs = *(q31x4_t *) pCoeffsTmp;
    
                    vecIn0 = vld1q(pSamplesTmp);
                    acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs);
    
                    vecIn0 = vld1q(&pSamplesTmp[1]);
                    acc1 = vrmlaldavhaq(acc1, vecIn0, vecCoeffs);
    
                    pSamplesTmp += 4;
                    pCoeffsTmp += 4;
                    i--;
                }

                tapsBlkCnt = (numTaps ) & 3;
                i = tapsBlkCnt ;
                while (i > 0U)
                {
                   

                    /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
                    acc0 += ((q63_t) *pSamplesTmp * *pCoeffsTmp) >> 8;
                    acc1 += ((q63_t) pSamplesTmp[1] * *pCoeffsTmp) >> 8;
    
                    pSamplesTmp += 1;
                    pCoeffsTmp += 1;
                    /*
                     * Decrement the taps block loop counter
                     */
                    i--;
                }
    
                acc0 = asrl(acc0, 23);
                acc1 = asrl(acc1, 23);
    
                *pOutput++ = (q31_t) acc0;
                *pOutput++ = (q31_t) acc1;
            }
            break;
    
        case 1:
            {
                const q31_t    *pCoeffsTmp = pCoeffs;
                const q31_t    *pSamplesTmp = pSamples;
    
                acc0 = 0LL;
    
                /*
                 * Save 4 input samples in the history buffer
                 */
                vst1q(pStateCur, vld1q(pTempSrc));
                pStateCur += 4;
                pTempSrc += 4;
    
                tapsBlkCnt = (numTaps ) / 4;
                uint32_t       i = tapsBlkCnt;
                while (i > 0U)
                {
                    vecCoeffs = *(q31x4_t *) pCoeffsTmp;
    
                    vecIn0 = vld1q(pSamplesTmp);
                    acc0 = vrmlaldavhaq(acc0, vecIn0, vecCoeffs);
    
                    pSamplesTmp += 4;
                    pCoeffsTmp += 4;
                    i--;
                }

                tapsBlkCnt = (numTaps ) & 3;
                i = tapsBlkCnt ;
                while (i > 0U)
                {
                   

                    /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
                    acc0 += ((q63_t) *pSamplesTmp * *pCoeffsTmp) >> 8;
    
                    pSamplesTmp += 1;
                    pCoeffsTmp += 1;
                    /*
                     * Decrement the taps block loop counter
                     */
                    i--;
                }
    
                acc0 = asrl(acc0, 23);
    
                *pOutput++ = (q31_t) acc0;
            }
            break;
        }
    }
    else
    {
         
                q31_t *pStateCurnt;                            /* Points to the current sample of the state */
                q31_t *px;                                     /* Temporary pointer for state buffer */
          const q31_t *pb;                                     /* Temporary pointer for coefficient buffer */
                q63_t acc0;                                    /* Accumulator */
                uint32_t i, blkCnt;                    /* Loop counters */
          pStateCurnt = &(S->pState[(numTaps - 1U)]);
          blkCnt = blockSize;
        
          while (blkCnt > 0U)
          {
            /* Copy one sample at a time into state buffer */
            *pStateCurnt++ = *pSrc++;
        
            /* Set the accumulator to zero */
            acc0 = 0;
        
            /* Initialize state pointer */
            px = pState;
        
            /* Initialize Coefficient pointer */
            pb = pCoeffs;
        
            i = numTaps;
        
            /* Perform the multiply-accumulates */
            do
            {
              /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
              acc0 += (q63_t) *px++ * *pb++;
        
              i--;
            } while (i > 0U);
        
            /* Result is in 2.62 format. Convert to 1.31 and store in destination buffer. */
            *pDst++ = (q31_t) (acc0 >> 31U);
        
            /* Advance state pointer by 1 for the next sample */
            pState = pState + 1U;
        
            /* Decrement loop counter */
            blkCnt--;
        }
    }

    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;

    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_s32(pTempDest, vld1q(pTempSrc), p0);
    }
}

#else
void arm_fir_q31(
  const arm_fir_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
        q31_t *pState = S->pState;                     /* State pointer */
  const q31_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q31_t *pStateCurnt;                            /* Points to the current sample of the state */
        q31_t *px;                                     /* Temporary pointer for state buffer */
  const q31_t *pb;                                     /* Temporary pointer for coefficient buffer */
        q63_t acc0;                                    /* Accumulator */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, tapCnt, blkCnt;                    /* Loop counters */

#if defined (ARM_MATH_LOOPUNROLL)
        q63_t acc1, acc2;                              /* Accumulators */
        q31_t x0, x1, x2;                              /* Temporary variables to hold state values */
        q31_t c0;                                      /* Temporary variable to hold coefficient value */
#endif

  /* S->pState points to state array which contains previous frame (numTaps - 1) samples */
  /* pStateCurnt points to the location where the new input data should be written */
  pStateCurnt = &(S->pState[(numTaps - 1U)]);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 output values simultaneously.
   * The variables acc0 ... acc3 hold output values that are being computed:
   *
   *    acc0 =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0]
   *    acc1 =  b[numTaps-1] * x[n-numTaps]   + b[numTaps-2] * x[n-numTaps-1] + b[numTaps-3] * x[n-numTaps-2] +...+ b[0] * x[1]
   *    acc2 =  b[numTaps-1] * x[n-numTaps+1] + b[numTaps-2] * x[n-numTaps]   + b[numTaps-3] * x[n-numTaps-1] +...+ b[0] * x[2]
   *    acc3 =  b[numTaps-1] * x[n-numTaps+2] + b[numTaps-2] * x[n-numTaps+1] + b[numTaps-3] * x[n-numTaps]   +...+ b[0] * x[3]
   */

  blkCnt = blockSize / 3;

  while (blkCnt > 0U)
  {
    /* Copy 3 new input samples into the state buffer. */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Set all accumulators to zero */
    acc0 = 0;
    acc1 = 0;
    acc2 = 0;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coefficient pointer */
    pb = pCoeffs;

    /* Read the first 2 samples from the state buffer: x[n-numTaps], x[n-numTaps-1] */
    x0 = *px++;
    x1 = *px++;

    /* Loop unrolling: process 3 taps at a time. */
    tapCnt = numTaps / 3;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps] coefficient */
      c0 = *pb;

      /* Read x[n-numTaps-2] sample */
      x2 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += ((q63_t) x0 * c0);
      acc1 += ((q63_t) x1 * c0);
      acc2 += ((q63_t) x2 * c0);

      /* Read the coefficient and state */
      c0 = *(pb + 1U);
      x0 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += ((q63_t) x1 * c0);
      acc1 += ((q63_t) x2 * c0);
      acc2 += ((q63_t) x0 * c0);

      /* Read the coefficient and state */
      c0 = *(pb + 2U);
      x1 = *(px++);

      /* update coefficient pointer */
      pb += 3U;

      /* Perform the multiply-accumulates */
      acc0 += ((q63_t) x2 * c0);
      acc1 += ((q63_t) x0 * c0);
      acc2 += ((q63_t) x1 * c0);

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining outputs */
    tapCnt = numTaps % 0x3U;

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *(pb++);

      /* Fetch 1 state variable */
      x2 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += ((q63_t) x0 * c0);
      acc1 += ((q63_t) x1 * c0);
      acc2 += ((q63_t) x2 * c0);

      /* Reuse the present sample states for next sample */
      x0 = x1;
      x1 = x2;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by 3 to process the next group of 3 samples */
    pState = pState + 3;

    /* The result is in 2.30 format. Convert to 1.31 and store in destination buffer. */
    *pDst++ = (q31_t) (acc0 >> 31U);
    *pDst++ = (q31_t) (acc1 >> 31U);
    *pDst++ = (q31_t) (acc2 >> 31U);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining output samples */
  blkCnt = blockSize % 0x3U;

#else

  /* Initialize blkCnt with number of taps */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy one sample at a time into state buffer */
    *pStateCurnt++ = *pSrc++;

    /* Set the accumulator to zero */
    acc0 = 0;

    /* Initialize state pointer */
    px = pState;

    /* Initialize Coefficient pointer */
    pb = pCoeffs;

    i = numTaps;

    /* Perform the multiply-accumulates */
    do
    {
      /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
      acc0 += (q63_t) *px++ * *pb++;

      i--;
    } while (i > 0U);

    /* Result is in 2.62 format. Convert to 1.31 and store in destination buffer. */
    *pDst++ = (q31_t) (acc0 >> 31U);

    /* Advance state pointer by 1 for the next sample */
    pState = pState + 1U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last numTaps - 1 samples to the start of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCurnt = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time */
  tapCnt = (numTaps - 1U) >> 2U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Calculate remaining number of copies */
  tapCnt = (numTaps - 1U) % 0x4U;

#else

  /* Initialize tapCnt with number of taps */
  tapCnt = (numTaps - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy remaining data */
  while (tapCnt > 0U)
  {
    *pStateCurnt++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of FIR group
 */
