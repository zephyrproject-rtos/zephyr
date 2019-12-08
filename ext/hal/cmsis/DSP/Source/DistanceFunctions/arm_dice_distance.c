
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dice_distance.c
 * Description:  Dice distance between two vectors
 *
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
#include <limits.h>
#include <math.h>

extern void arm_boolean_distance_TT_TF_FT(const uint32_t *pA
       , const uint32_t *pB
       , uint32_t numberOfBools
       , uint32_t *cTT
       , uint32_t *cTF
       , uint32_t *cFT
       );


/**
 * @ingroup groupDistance
 * @{
 */

/**
 * @defgroup BoolDist Boolean Distances
 *
 * Distances between two vectors of boolean values.
 *
 * Booleans are packed in 32 bit words.
 * numberOfBooleans argument is the number of booleans and not the
 * number of words.
 *
 * Bits are packed in big-endian mode (because of behavior of numpy packbits in
 * in version < 1.17)
 */

/**
  @addtogroup BoolDist
  @{
 */

/**
 * @brief        Dice distance between two vectors
 *
 * @param[in]    pA              First vector of packed booleans
 * @param[in]    pB              Second vector of packed booleans
 * @param[in]    numberOfBools   Number of booleans
 * @return distance
 *
 */

float32_t arm_dice_distance(const uint32_t *pA, const uint32_t *pB, uint32_t numberOfBools)
{
    uint32_t ctt=0,ctf=0,cft=0;

    arm_boolean_distance_TT_TF_FT(pA, pB, numberOfBools, &ctt, &ctf, &cft);

    return(1.0*(ctf + cft) / (2.0*ctt + cft + ctf));
}


/**
 * @} end of BoolDist group
 */

/**
 * @} end of groupDistance group
 */
