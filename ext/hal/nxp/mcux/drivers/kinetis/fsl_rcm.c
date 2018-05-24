/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
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

#include "fsl_rcm.h"

void RCM_ConfigureResetPinFilter(RCM_Type *base, const rcm_reset_pin_filter_config_t *config)
{
    assert(config);

#if (defined(FSL_FEATURE_RCM_REG_WIDTH) && (FSL_FEATURE_RCM_REG_WIDTH == 32))
    uint32_t reg;

    reg = (((uint32_t)config->enableFilterInStop << RCM_RPC_RSTFLTSS_SHIFT) | (uint32_t)config->filterInRunWait);
    if (config->filterInRunWait == kRCM_FilterBusClock)
    {
        reg |= ((uint32_t)config->busClockFilterCount << RCM_RPC_RSTFLTSEL_SHIFT);
    }
    base->RPC = reg;
#else
    base->RPFC = ((uint8_t)(config->enableFilterInStop << RCM_RPFC_RSTFLTSS_SHIFT) | (uint8_t)config->filterInRunWait);
    if (config->filterInRunWait == kRCM_FilterBusClock)
    {
        base->RPFW = config->busClockFilterCount;
    }
#endif /* FSL_FEATURE_RCM_REG_WIDTH */
}

#if (defined(FSL_FEATURE_RCM_HAS_BOOTROM) && FSL_FEATURE_RCM_HAS_BOOTROM)
void RCM_SetForceBootRomSource(RCM_Type *base, rcm_boot_rom_config_t config)
{
    uint32_t reg;

    reg = base->FM;
    reg &= ~RCM_FM_FORCEROM_MASK;
    reg |= ((uint32_t)config << RCM_FM_FORCEROM_SHIFT);
    base->FM = reg;
}
#endif /* #if FSL_FEATURE_RCM_HAS_BOOTROM */
