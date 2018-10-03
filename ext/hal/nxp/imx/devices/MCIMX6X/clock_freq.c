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

#include "clock_freq.h"
#include "ccm_imx6sx.h"
#include "ccm_analog_imx6sx.h"

/*FUNCTION**********************************************************************
 *
 * Function Name : get_epit_clock_freq
 * Description   : Get clock frequency applys to the EPIIT module
 *
 *END**************************************************************************/
uint32_t get_epit_clock_freq(EPIT_Type* base)
{
    uint32_t root;
    uint32_t hz;
    uint32_t divPerclkPodf, divIpgPodf, divAhbPodf, divPeriphClk2Podf;

    /* Different instance has the same clock root, it's different from i.mx7d. */
    /* Get the clock root according to the mux node of clock tree. */
    if(CCM_GetRootMux(CCM, ccmRootPerclkClkSel) == ccmRootmuxPerclkClkOsc24m)
    {
        root = ccmRootmuxPerclkClkOsc24m;
        hz = 24000000;
        divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
        divIpgPodf = 0;
        divAhbPodf = 0;
        divPeriphClk2Podf = 0;
    }
    else if(CCM_GetRootMux(CCM, ccmRootPeriphClkSel) == ccmRootmuxPeriphClkPrePeriphClkSel)
    {
        root = CCM_GetRootMux(CCM, ccmRootPrePeriphClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL2_PFD2, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPrePeriphClkPll2:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllSysControl);
                divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
                divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
                divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
                divPeriphClk2Podf = 0;
                break;
            default:
                return 0;
        }
    }
    else if(CCM_GetRootMux(CCM, ccmRootPeriphClk2Sel) == ccmRootmuxPeriphClk2OSC24m)
    {
        root = ccmRootmuxPeriphClk2OSC24m;
        hz = 24000000;
        divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
        divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
        divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
        divPeriphClk2Podf = CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
    }
    else
    {
        root = CCM_GetRootMux(CCM, ccmRootPll3SwClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL3_BYP, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPll3SwClkPll3:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllUsb1Control);
                divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
                divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
                divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
                divPeriphClk2Podf = CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
                break;
            default:
                return 0;
        }
    }

    return hz / (divPerclkPodf + 1) / (divIpgPodf + 1) / (divAhbPodf + 1) / (divPeriphClk2Podf + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : get_I2C_clock_freq
 * Description   : Get clock frequency applys to the I2C module
 *
 *END**************************************************************************/
uint32_t get_i2c_clock_freq(I2C_Type* base)
{
    uint32_t root;
    uint32_t hz;
    uint32_t divPerclkPodf, divIpgPodf, divAhbPodf, divPeriphClk2Podf;

    /* Different instance has the same clock root, it's different from i.mx7d. */
    /* Get the clock root according to the mux node of clock tree. */
    if(CCM_GetRootMux(CCM, ccmRootPerclkClkSel) == ccmRootmuxPerclkClkOsc24m)
    {
        root = ccmRootmuxPerclkClkOsc24m;
        hz = 24000000;
        divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
        divIpgPodf = 0;
        divAhbPodf = 0;
        divPeriphClk2Podf = 0;
    }
    else if(CCM_GetRootMux(CCM, ccmRootPeriphClkSel) == ccmRootmuxPeriphClkPrePeriphClkSel)
    {
        root = CCM_GetRootMux(CCM, ccmRootPrePeriphClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL2_PFD2, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPrePeriphClkPll2:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllSysControl);
                divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
                divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
                divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
                divPeriphClk2Podf = 0;
                break;
            default:
                return 0;
        }
    }
    else if(CCM_GetRootMux(CCM, ccmRootPeriphClk2Sel) == ccmRootmuxPeriphClk2OSC24m)
    {
        root = ccmRootmuxPeriphClk2OSC24m;
        hz = 24000000;
        divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
        divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
        divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
        divPeriphClk2Podf = CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
    }
    else
    {
        root = CCM_GetRootMux(CCM, ccmRootPll3SwClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL3_BYP, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPll3SwClkPll3:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllUsb1Control);
                divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
                divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
                divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
                divPeriphClk2Podf = CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
                break;
            default:
                return 0;
        }
    }

    return hz / (divPerclkPodf + 1) / (divIpgPodf + 1) / (divAhbPodf + 1) / (divPeriphClk2Podf + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : get_ecspi_clock_freq
 * Description   : Get clock frequency applys to the ECSPI module
 *
 *END**************************************************************************/
uint32_t get_ecspi_clock_freq(ECSPI_Type* base)
{
    uint32_t root;
    uint32_t hz;
    uint32_t divEcspiClkPodf, divStatic;

    if(CCM_GetRootMux(CCM, ccmRootEcspiClkSel) == ccmRootmuxEcspiClkOsc24m)
    {
        root = ccmRootmuxEcspiClkOsc24m;
        hz = 24000000;
        divEcspiClkPodf = CCM_GetRootDivider(CCM, ccmRootEcspiClkPodf);
        divStatic = 0;
    }
    else
    {
        root = CCM_GetRootMux(CCM, ccmRootPll3SwClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL3_BYP, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPll3SwClkPll3:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllUsb1Control);
                divEcspiClkPodf = CCM_GetRootDivider(CCM, ccmRootEcspiClkPodf);
                divStatic = 7;
                break;
            default:
                return 0;
        }
    }

    return hz / (divEcspiClkPodf + 1) / (divStatic + 1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : get_uart_clock_freq
 * Description   : Get clock frequency applys to the UART module
 *
 *END**************************************************************************/
uint32_t get_uart_clock_freq(UART_Type* base)
{
    uint32_t root;
    uint32_t hz;
    uint32_t divUartClkPodf, divStatic;

    if(CCM_GetRootMux(CCM, ccmRootUartClkSel) == ccmRootmuxUartClkOsc24m)
    {
        root = ccmRootmuxUartClkOsc24m;
        hz = 24000000;
        divUartClkPodf = CCM_GetRootDivider(CCM, ccmRootUartClkPodf);
        divStatic = 0;
    }
    else
    {
        root = CCM_GetRootMux(CCM, ccmRootPll3SwClkSel);
        /* Here do not show all the clock root source,
           if user use other clock root source, such as PLL3_BYP, please
           add it as follows according to the clock tree of CCM in reference manual. */
        switch(root)
        {
            case ccmRootmuxPll3SwClkPll3:
                hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllUsb1Control);
                divUartClkPodf = CCM_GetRootDivider(CCM, ccmRootUartClkPodf);
                divStatic = 5;
                break;
            default:
                return 0;
        }
    }

    return hz / (divUartClkPodf + 1) / (divStatic + 1);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
