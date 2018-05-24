/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fsl_pmu.h"

uint32_t PMU_GetStatusFlags(PMU_Type *base)
{
    uint32_t ret = 0U;

    /* For 1P1. */
    if (PMU_REG_1P1_OK_VDD1P1_MASK == (PMU_REG_1P1_OK_VDD1P1_MASK & base->REG_1P1))
    {
        ret |= kPMU_1P1RegulatorOutputOK;
    }
    if (PMU_REG_1P1_BO_VDD1P1_MASK == (PMU_REG_1P1_BO_VDD1P1_MASK & base->REG_1P1))
    {
        ret |= kPMU_1P1BrownoutOnOutput;
    }

    /* For 3P0. */
    if (PMU_REG_3P0_OK_VDD3P0_MASK == (PMU_REG_3P0_OK_VDD3P0_MASK & base->REG_3P0))
    {
        ret |= kPMU_3P0RegulatorOutputOK;
    }
    if (PMU_REG_3P0_BO_VDD3P0_MASK == (PMU_REG_3P0_BO_VDD3P0_MASK & base->REG_3P0))
    {
        ret |= kPMU_3P0BrownoutOnOutput;
    }

    /* For 2P5. */
    if (PMU_REG_2P5_OK_VDD2P5_MASK == (PMU_REG_2P5_OK_VDD2P5_MASK & base->REG_2P5))
    {
        ret |= kPMU_2P5RegulatorOutputOK;
    }
    if (PMU_REG_2P5_BO_VDD2P5_MASK == (PMU_REG_2P5_BO_VDD2P5_MASK & base->REG_2P5))
    {
        ret |= kPMU_2P5BrownoutOnOutput;
    }

    return ret;
}
