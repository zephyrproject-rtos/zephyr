/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_barycenter_f32.c
 * Description:  Barycenter
 *
 *
 * Target Processor: Cortex-M and Cortex-A cores
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
#include <limits.h>
#include <math.h>


/**
  @ingroup groupSupport
 */


/**
 * @brief Barycenter
 *
 *
 * @param[in]    *in         List of vectors
 * @param[in]    *weights    Weights of the vectors
 * @param[out]   *out        Barycenter
 * @param[in]    nbVectors   Number of vectors
 * @param[in]    vecDim      Dimension of space (vector dimension)
 * @return       None
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_barycenter_f32(const float32_t *in, 
  const float32_t *weights, 
  float32_t *out, 
  uint32_t nbVectors,
  uint32_t vecDim)
{
    const float32_t *pIn, *pW;
    const float32_t *pIn1, *pIn2, *pIn3, *pIn4;
    float32_t      *pOut;
    uint32_t        blkCntVector, blkCntSample;
    float32_t       accum, w;

    blkCntVector = nbVectors;
    blkCntSample = vecDim;

    accum = 0.0f;

    pW = weights;
    pIn = in;


    arm_fill_f32(0.0f, out, vecDim);


    /* Sum */
    pIn1 = pIn;
    pIn2 = pIn1 + vecDim;
    pIn3 = pIn2 + vecDim;
    pIn4 = pIn3 + vecDim;

    blkCntVector = nbVectors >> 2;
    while (blkCntVector > 0) 
    {
        f32x4_t         outV, inV1, inV2, inV3, inV4;
        float32_t       w1, w2, w3, w4;

        pOut = out;
        w1 = *pW++;
        w2 = *pW++;
        w3 = *pW++;
        w4 = *pW++;
        accum += w1 + w2 + w3 + w4;

        blkCntSample = vecDim >> 2;
        while (blkCntSample > 0) {
            outV = vld1q((const float32_t *) pOut);
            inV1 = vld1q(pIn1);
            inV2 = vld1q(pIn2);
            inV3 = vld1q(pIn3);
            inV4 = vld1q(pIn4);
            outV = vfmaq(outV, inV1, w1);
            outV = vfmaq(outV, inV2, w2);
            outV = vfmaq(outV, inV3, w3);
            outV = vfmaq(outV, inV4, w4);
            vst1q(pOut, outV);

            pOut += 4;
            pIn1 += 4;
            pIn2 += 4;
            pIn3 += 4;
            pIn4 += 4;

            blkCntSample--;
        }

        blkCntSample = vecDim & 3;
        while (blkCntSample > 0) {
            *pOut = *pOut + *pIn1++ * w1;
            *pOut = *pOut + *pIn2++ * w2;
            *pOut = *pOut + *pIn3++ * w3;
            *pOut = *pOut + *pIn4++ * w4;
            pOut++;
            blkCntSample--;
        }

        pIn1 += 3 * vecDim;
        pIn2 += 3 * vecDim;
        pIn3 += 3 * vecDim;
        pIn4 += 3 * vecDim;

        blkCntVector--;
    }

    pIn = pIn1;

    blkCntVector = nbVectors & 3;
    while (blkCntVector > 0) 
    {
        f32x4_t         inV, outV;

        pOut = out;
        w = *pW++;
        accum += w;

        blkCntSample = vecDim >> 2;
        while (blkCntSample > 0) 
        {
            outV = vld1q_f32(pOut);
            inV = vld1q_f32(pIn);
            outV = vfmaq(outV, inV, w);
            vst1q_f32(pOut, outV);
            pOut += 4;
            pIn += 4;

            blkCntSample--;
        }

        blkCntSample = vecDim & 3;
        while (blkCntSample > 0) 
        {
            *pOut = *pOut + *pIn++ * w;
            pOut++;
            blkCntSample--;
        }

        blkCntVector--;
    }

    /* Normalize */
    pOut = out;
    accum = 1.0f / accum;

    blkCntSample = vecDim >> 2;
    while (blkCntSample > 0) 
    {
        f32x4_t         tmp;

        tmp = vld1q((const float32_t *) pOut);
        tmp = vmulq(tmp, accum);
        vst1q(pOut, tmp);
        pOut += 4;
        blkCntSample--;
    }

    blkCntSample = vecDim & 3;
    while (blkCntSample > 0) 
    {
        *pOut = *pOut * accum;
        pOut++;
        blkCntSample--;
    }
}
#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"
void arm_barycenter_f32(const float32_t *in, const float32_t *weights, float32_t *out, uint32_t nbVectors,uint32_t vecDim)
{

   const float32_t *pIn,*pW, *pIn1, *pIn2, *pIn3, *pIn4;
   float32_t *pOut;
   uint32_t blkCntVector,blkCntSample;
   float32_t accum, w,w1,w2,w3,w4;

   float32x4_t tmp, inV,outV, inV1, inV2, inV3, inV4;

   blkCntVector = nbVectors;
   blkCntSample = vecDim;

   accum = 0.0f;

   pW = weights;
   pIn = in;

   /* Set counters to 0 */
   tmp = vdupq_n_f32(0.0f);
   pOut = out;

   blkCntSample = vecDim >> 2;
   while(blkCntSample > 0)
   {
         vst1q_f32(pOut, tmp);
         pOut += 4;
         blkCntSample--;
   }

   blkCntSample = vecDim & 3;
   while(blkCntSample > 0)
   {
         *pOut = 0.0f;
         pOut++;
         blkCntSample--;
   }

   /* Sum */
  
   pIn1 = pIn;
   pIn2 = pIn1 + vecDim;
   pIn3 = pIn2 + vecDim;
   pIn4 = pIn3 + vecDim;
   
   blkCntVector = nbVectors >> 2;
   while(blkCntVector > 0)
   {
      pOut = out;
      w1 = *pW++;
      w2 = *pW++;
      w3 = *pW++;
      w4 = *pW++;
      accum += w1 + w2 + w3 + w4;

      blkCntSample = vecDim >> 2;
      while(blkCntSample > 0)
      {
          outV = vld1q_f32(pOut);
          inV1 = vld1q_f32(pIn1);
          inV2 = vld1q_f32(pIn2);
          inV3 = vld1q_f32(pIn3);
          inV4 = vld1q_f32(pIn4);
          outV = vmlaq_n_f32(outV,inV1,w1);
          outV = vmlaq_n_f32(outV,inV2,w2);
          outV = vmlaq_n_f32(outV,inV3,w3);
          outV = vmlaq_n_f32(outV,inV4,w4);
          vst1q_f32(pOut, outV);
          pOut += 4;
          pIn1 += 4;
          pIn2 += 4;
          pIn3 += 4;
          pIn4 += 4;

          blkCntSample--;
      }

      blkCntSample = vecDim & 3;
      while(blkCntSample > 0)
      {
          *pOut = *pOut + *pIn1++ * w1;
          *pOut = *pOut + *pIn2++ * w2;
          *pOut = *pOut + *pIn3++ * w3;
          *pOut = *pOut + *pIn4++ * w4;
          pOut++;
          blkCntSample--;
      }

      pIn1 += 3*vecDim;
      pIn2 += 3*vecDim;
      pIn3 += 3*vecDim;
      pIn4 += 3*vecDim;

      blkCntVector--;
   }

   pIn = pIn1;

   blkCntVector = nbVectors & 3;
   while(blkCntVector > 0)
   {
      pOut = out;
      w = *pW++;
      accum += w;

      blkCntSample = vecDim >> 2;
      while(blkCntSample > 0)
      {
          outV = vld1q_f32(pOut);
          inV = vld1q_f32(pIn);
          outV = vmlaq_n_f32(outV,inV,w);
          vst1q_f32(pOut, outV);
          pOut += 4;
          pIn += 4;

          blkCntSample--;
      }
      
      blkCntSample = vecDim & 3;
      while(blkCntSample > 0)
      {
          *pOut = *pOut + *pIn++ * w;
          pOut++;
          blkCntSample--;
      }

      blkCntVector--;
   }

   /* Normalize */
   pOut = out;
   accum = 1.0f / accum;

   blkCntSample = vecDim >> 2;
   while(blkCntSample > 0)
   {
         tmp = vld1q_f32(pOut);
         tmp = vmulq_n_f32(tmp,accum);
         vst1q_f32(pOut, tmp);
         pOut += 4;
         blkCntSample--;
   }

   blkCntSample = vecDim & 3;
   while(blkCntSample > 0)
   {
         *pOut = *pOut * accum;
         pOut++;
         blkCntSample--;
   }

}
#else
void arm_barycenter_f32(const float32_t *in, const float32_t *weights, float32_t *out, uint32_t nbVectors,uint32_t vecDim)
{

   const float32_t *pIn,*pW;
   float32_t *pOut;
   uint32_t blkCntVector,blkCntSample;
   float32_t accum, w;

   blkCntVector = nbVectors;
   blkCntSample = vecDim;

   accum = 0.0f;

   pW = weights;
   pIn = in;

   /* Set counters to 0 */
   blkCntSample = vecDim;
   pOut = out;

   while(blkCntSample > 0)
   {
         *pOut = 0.0f;
         pOut++;
         blkCntSample--;
   }

   /* Sum */
   while(blkCntVector > 0)
   {
      pOut = out;
      w = *pW++;
      accum += w;

      blkCntSample = vecDim;
      while(blkCntSample > 0)
      {
          *pOut = *pOut + *pIn++ * w;
          pOut++;
          blkCntSample--;
      }

      blkCntVector--;
   }

   /* Normalize */
   blkCntSample = vecDim;
   pOut = out;

   while(blkCntSample > 0)
   {
         *pOut = *pOut / accum;
         pOut++;
         blkCntSample--;
   }

}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of groupSupport group
 */
