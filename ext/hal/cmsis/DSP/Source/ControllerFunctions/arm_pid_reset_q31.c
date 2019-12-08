/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_pid_reset_q31.c
 * Description:  Q31 PID Control reset function
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
  @addtogroup PID
  @{
 */

/**
  @brief         Reset function for the Q31 PID Control.
  @param[in,out] S  points to an instance of the Q31 PID structure
  @return        none

  @par           Details
                   The function resets the state buffer to zeros.
 */

void arm_pid_reset_q31(
  arm_pid_instance_q31 * S)
{
  /* Reset state to zero, The size will be always 3 samples */
  memset(S->state, 0, 3U * sizeof(q31_t));
}

/**
  @} end of PID group
 */
