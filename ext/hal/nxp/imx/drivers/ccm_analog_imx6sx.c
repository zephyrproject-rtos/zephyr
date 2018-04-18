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

#include "ccm_analog_imx6sx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_PowerUpPll
 * Description   : Power up PLL.
 *
 *END**************************************************************************/
void CCM_ANALOG_PowerUpPll(CCM_ANALOG_Type * base, uint32_t pllControl)
{
    /* Judge PLL_USB1 and PLL_USB2 according to register offset value. 
	   Because the definition of power control bit is different from the other PLL.*/
    if((CCM_ANALOG_TUPLE_OFFSET(pllControl) == 0x10) || (CCM_ANALOG_TUPLE_OFFSET(pllControl) == 0x20))
        CCM_ANALOG_TUPLE_REG_SET(base, pllControl) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllControl);
    else
        CCM_ANALOG_TUPLE_REG_CLR(base, pllControl) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllControl);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_PowerDownPll
 * Description   : Power down PLL.
 *
 *END**************************************************************************/
void CCM_ANALOG_PowerDownPll(CCM_ANALOG_Type * base, uint32_t pllControl)
{
    /* Judge PLL_USB1 and PLL_USB2 according to register offset value. 
	   Because the definition of power control bit is different from the other PLL.*/
    if((CCM_ANALOG_TUPLE_OFFSET(pllControl) == 0x10) || (CCM_ANALOG_TUPLE_OFFSET(pllControl) == 0x20))
        CCM_ANALOG_TUPLE_REG_CLR(base, pllControl) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllControl);
    else
        CCM_ANALOG_TUPLE_REG_SET(base, pllControl) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllControl);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_SetPllBypass
 * Description   : PLL bypass setting.
 *
 *END**************************************************************************/
void CCM_ANALOG_SetPllBypass(CCM_ANALOG_Type * base, uint32_t pllControl, bool bypass)
{
    if(bypass)
        CCM_ANALOG_TUPLE_REG_SET(base, pllControl) = CCM_ANALOG_PLL_ARM_BYPASS_MASK;
    else
        CCM_ANALOG_TUPLE_REG_CLR(base, pllControl) = CCM_ANALOG_PLL_ARM_BYPASS_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_ANALOG_GetPllFreq
 * Description   : Get PLL frequency
 *
 *END**************************************************************************/
uint32_t CCM_ANALOG_GetPllFreq(CCM_ANALOG_Type * base, uint32_t pllControl)
{
    uint8_t divSelect;
    float numerator, denomitor;
    uint32_t hz = 0;

    if (CCM_ANALOG_IsPllBypassed(base, pllControl))
        return 24000000;

    switch(CCM_ANALOG_TUPLE_OFFSET(pllControl))
    {
        /* Get PLL_ARM frequency. */
        case 0x0:
        {
            divSelect = CCM_ANALOG_PLL_ARM & CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK;
            hz = 24000000 * divSelect / 2;
            break;
        }
        /* Get PLL_USB1(PLL3) frequency. */
        case 0x10:
        {
            divSelect = CCM_ANALOG_PLL_USB1 & CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK;
            if(divSelect == 0)
                hz = 480000000;
            else if(divSelect == 1)
                hz = 528000000;
            break;
        }
        /* Get PLL_USB2 frequency. */
        case 0x20:
        {
            divSelect = CCM_ANALOG_PLL_USB2 & CCM_ANALOG_PLL_USB2_DIV_SELECT_MASK;
            if(divSelect == 0)
                hz = 480000000;
            else if(divSelect == 1)
                hz = 528000000;
            break;
        }
        /* Get PLL_SYS(PLL2) frequency. */
        case 0x30:
        {
            divSelect = CCM_ANALOG_PLL_SYS & CCM_ANALOG_PLL_SYS_DIV_SELECT_MASK;
            if(divSelect == 0)
                hz = 480000000;
            else
                hz = 528000000;
            break;
        }
        /* Get PLL_AUDIO frequency. */
        case 0x70:
        {
            divSelect = CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_DIV_SELECT_MASK;
            numerator = CCM_ANALOG_PLL_AUDIO_NUM & CCM_ANALOG_PLL_AUDIO_NUM_A_MASK;
            denomitor = CCM_ANALOG_PLL_AUDIO_DENOM & CCM_ANALOG_PLL_AUDIO_DENOM_B_MASK;
            hz = (uint32_t)(24000000 * (divSelect + (numerator / denomitor)));
            break;
        }
        /* Get PLL_VIDEO frequency. */
        case 0xA0:
        {
            divSelect = CCM_ANALOG_PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_DIV_SELECT_MASK;
            numerator = CCM_ANALOG_PLL_VIDEO_NUM & CCM_ANALOG_PLL_VIDEO_NUM_A_MASK;
            denomitor = CCM_ANALOG_PLL_VIDEO_DENOM & CCM_ANALOG_PLL_VIDEO_DENOM_B_MASK;
            hz = (uint32_t)(24000000 * (divSelect + (numerator / denomitor)));
            break;
        }
    }
    return hz;
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

    /* Judge whether pfdFrac is PLL2 PFD or not. */
    if(CCM_ANALOG_TUPLE_OFFSET(pfdFrac) == 0x100)
    {
        /* PFD should work with PLL2 without bypass */
        assert(!CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllSysControl));
        main = CCM_ANALOG_GetPllFreq(base, ccmAnalogPllSysControl);
    }
    else if(CCM_ANALOG_TUPLE_OFFSET(pfdFrac) == 0xF0)
    {
        /* PFD should work with PLL3 without bypass */
        assert(!CCM_ANALOG_IsPllBypassed(base, ccmAnalogPllUsb1Control));
        main = CCM_ANALOG_GetPllFreq(base, ccmAnalogPllUsb1Control);
    }
    else
        main = 0;

    frac = CCM_ANALOG_GetPfdFrac(base, pfdFrac);

    return main / frac * 18; 
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
