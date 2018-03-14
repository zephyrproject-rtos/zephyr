/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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

#include "ccm_analog_imx7d.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetArmPllFreq
 * Description   : Get ARM PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetArmPllFreq(CCM_ANALOG_Type * base)
{
    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllArmControl))
        return 24000000ul;

    return 12000000ul * (CCM_ANALOG_PLL_ARM & CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetSysPllFreq
 * Description   : Get system PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetSysPllFreq(CCM_ANALOG_Type * base)
{
    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPll480Control))
        return 24000000ul;

    if (CCM_ANALOG_PLL_480 & CCM_ANALOG_PLL_480_DIV_SELECT_MASK)
        return 528000000ul;
    else
        return 480000000ul;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetDdrPllFreq
 * Description   : Get DDR PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetDdrPllFreq(CCM_ANALOG_Type * base)
{
    uint8_t divSelect, divTestSelect;
    float factor;

    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllDdrControl))
        return 24000000ul;

    divSelect = CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG) & CCM_ANALOG_PLL_DDR_DIV_SELECT_MASK;
    divTestSelect = (CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG) & CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT_MASK) >>
                     CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT_SHIFT;

    switch (divTestSelect)
    {
        case 0x0:
            divTestSelect = 2;
            break;
        case 0x1:
            divTestSelect = 1;
            break;
        case 0x2:
        case 0x3:
            divTestSelect = 0;
            break;
    }

    if (CCM_ANALOG_PLL_DDR_SS_REG(base) & CCM_ANALOG_PLL_DDR_SS_ENABLE_MASK)
    {
        factor = ((float)(CCM_ANALOG_PLL_DDR_SS_REG(base) & CCM_ANALOG_PLL_DDR_SS_STEP_MASK)) /
                 ((float)(CCM_ANALOG_PLL_DDR_DENOM_REG(base) & CCM_ANALOG_PLL_DDR_DENOM_B_MASK)) *
                 ((float)(CCM_ANALOG_PLL_DDR_NUM_REG(base) & CCM_ANALOG_PLL_DDR_NUM_A_MASK));
        return (uint32_t)((24000000ul >> divTestSelect) * (divSelect + factor));
    }
    else
    {
        return (24000000ul >> divTestSelect) * divSelect;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetEnetPllFreq
 * Description   : Get Ethernet PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetEnetPllFreq(CCM_ANALOG_Type * base)
{
    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllEnetControl))
        return 24000000ul;

    return 1000000000ul;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetAudioPllFreq
 * Description   : Get Ethernet PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetAudioPllFreq(CCM_ANALOG_Type * base)
{
    uint8_t divSelect, divPostSelect, divTestSelect;
    float factor;

    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllAudioControl))
        return 24000000ul;

    divSelect = CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_AUDIO_DIV_SELECT_MASK;
    divPostSelect = (CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_AUDIO_POST_DIV_SEL_MASK) >>
                    CCM_ANALOG_PLL_AUDIO_POST_DIV_SEL_SHIFT;
    divTestSelect = (CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_AUDIO_TEST_DIV_SELECT_MASK) >>
                    CCM_ANALOG_PLL_AUDIO_TEST_DIV_SELECT_SHIFT;

    switch (divPostSelect)
    {
        case 0x0:
        case 0x2:
            divPostSelect = 0;
            break;
        case 0x1:
            divPostSelect = 1;
            break;
        case 0x3:
            divPostSelect = 2;
            break;
    }

    switch (divTestSelect)
    {
        case 0x0:
            divTestSelect = 2;
            break;
        case 0x1:
            divTestSelect = 1;
            break;
        case 0x2:
        case 0x3:
            divTestSelect = 0;
            break;
    }

    if (CCM_ANALOG_PLL_AUDIO_SS_REG(base) & CCM_ANALOG_PLL_AUDIO_SS_ENABLE_MASK)
    {
        factor = ((float)(CCM_ANALOG_PLL_AUDIO_SS_REG(base) & CCM_ANALOG_PLL_AUDIO_SS_STEP_MASK)) /
                 ((float)(CCM_ANALOG_PLL_AUDIO_DENOM_REG(base) & CCM_ANALOG_PLL_AUDIO_DENOM_B_MASK)) *
                 ((float)(CCM_ANALOG_PLL_AUDIO_NUM_REG(base) & CCM_ANALOG_PLL_AUDIO_NUM_A_MASK));
        return (uint32_t)(((24000000ul >> divTestSelect) >> divPostSelect) * (divSelect + factor));
    }
    else
    {
        return ((24000000ul >> divTestSelect) >> divPostSelect) * divSelect;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetVideoPllFreq
 * Description   : Get Ethernet PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetVideoPllFreq(CCM_ANALOG_Type * base)
{
    uint8_t divSelect, divPostSelect, divTestSelect;
    float factor;

    if (CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllVideoControl))
        return 24000000ul;

    divSelect = CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_VIDEO_DIV_SELECT_MASK;
    divPostSelect = (CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_VIDEO_POST_DIV_SEL_MASK) >>
                    CCM_ANALOG_PLL_VIDEO_POST_DIV_SEL_SHIFT;
    divTestSelect = (CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG) & CCM_ANALOG_PLL_VIDEO_TEST_DIV_SELECT_MASK) >>
                    CCM_ANALOG_PLL_VIDEO_TEST_DIV_SELECT_SHIFT;

    switch (divPostSelect)
    {
        case 0x0:
        case 0x2:
            divPostSelect = 0;
            break;
        case 0x1:
            divPostSelect = 1;
            break;
        case 0x3:
            divPostSelect = 2;
            break;
    }

    switch (divTestSelect)
    {
        case 0x0:
            divTestSelect = 2;
            break;
        case 0x1:
            divTestSelect = 1;
            break;
        case 0x2:
        case 0x3:
            divTestSelect = 0;
            break;
    }

    if (CCM_ANALOG_PLL_VIDEO_SS_REG(base) & CCM_ANALOG_PLL_VIDEO_SS_ENABLE_MASK)
    {
        factor = ((float)(CCM_ANALOG_PLL_VIDEO_SS_REG(base) & CCM_ANALOG_PLL_VIDEO_SS_STEP_MASK)) /
                 ((float)(CCM_ANALOG_PLL_VIDEO_DENOM_REG(base) & CCM_ANALOG_PLL_VIDEO_DENOM_B_MASK)) *
                 ((float)(CCM_ANALOG_PLL_VIDEO_NUM_REG(base) & CCM_ANALOG_PLL_VIDEO_NUM_A_MASK));
        return (uint32_t)(((24000000ul >> divTestSelect) >> divPostSelect) * (divSelect + factor));
    }
    else
    {
        return ((24000000ul >> divTestSelect) >> divPostSelect) * divSelect;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetPfdFreq
 * Description   : Get PFD frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetPfdFreq(CCM_ANALOG_Type * base, uint32_t pfdFrac)
{
    uint32_t main, frac;

    /* PFD should work with system PLL without bypass */
    assert(!CCM_ANALOG_IsPllBypassed(base, ccmAnalogPll480Control));

    main = CCM_ANALOG_GetSysPllFreq(base);
    frac = CCM_ANALOG_GetPfdFrac(base, pfdFrac);

    return main / frac * 18;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
