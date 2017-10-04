/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/lcd_f.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>

/* Define to ensure that our current MSP432 has the LCD_F module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_LCD_F__

/* Configuration functions */
void LCD_F_initModule(LCD_F_Config *initParams)
{
    BITBAND_PERI(LCD_F->CTL,LCD_F_CTL_ON_OFS) = 0;

    LCD_F->CTL = (LCD_F->CTL
            & ~(LCD_F_CTL_MX_MASK | LCD_F_CTL_SSEL_MASK | LCD_F_CTL_LP
                    | LCD_F_CTL_ON | LCD_F_CTL_DIV_MASK | LCD_F_CTL_PRE_MASK
                    | LCD_F_CTL_SON))
            | (initParams->muxRate | initParams->clockSource
                    | initParams->waveforms | initParams->segments
                    | initParams->clockDivider | initParams->clockPrescaler);
}

void LCD_F_turnOn(void)
{
    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 1;
}

void LCD_F_turnOff(void)
{
    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;
}

/* Memory management functions */
void LCD_F_clearAllMemory(void)
{
    BITBAND_PERI(LCD_F->BMCTL , LCD_F_BMCTL_CLRM_OFS) = 1;
}

void LCD_F_clearAllBlinkingMemory(void)
{
    BITBAND_PERI(LCD_F->BMCTL , LCD_F_BMCTL_CLRBM_OFS) = 1;
}

void LCD_F_selectDisplayMemory(uint_fast16_t displayMemory)
{
    BITBAND_PERI(LCD_F->BMCTL , LCD_F_BMCTL_DISP_OFS) = displayMemory;
}

void LCD_F_setBlinkingControl(uint_fast16_t clockPrescalar,
        uint_fast16_t divider, uint_fast16_t mode)
{
    LCD_F->BMCTL = (LCD_F->BMCTL
            & ~(LCD_F_BMCTL_BLKPRE_MASK | LCD_F_BMCTL_BLKDIV_MASK
                    | LCD_F_BMCTL_BLKMOD_MASK)) | clockPrescalar | mode
            | divider;
}

void LCD_F_setAnimationControl(uint_fast16_t clockPrescalar,
        uint_fast16_t divider, uint_fast16_t frames)
{
    LCD_F->ANMCTL = (LCD_F->ANMCTL
            & ~(LCD_F_ANMCTL_ANMPRE_MASK | LCD_F_ANMCTL_ANMDIV_MASK
                    | LCD_F_ANMCTL_ANMSTP_MASK)) | clockPrescalar | divider
            | frames;
}

void LCD_F_enableAnimation(void)
{
    BITBAND_PERI(LCD_F->ANMCTL, LCD_F_ANMCTL_ANMEN_OFS) = 1;
}

void LCD_F_disableAnimation(void)
{
    BITBAND_PERI(LCD_F->ANMCTL, LCD_F_ANMCTL_ANMEN_OFS) = 0;
}

void LCD_F_clearAllAnimationMemory(void)
{
    BITBAND_PERI(LCD_F->ANMCTL, LCD_F_ANMCTL_ANMCLR_OFS) = 1;
}

/* Pin Configuration Functions */
void LCD_F_setPinAsLCDFunction(uint_fast8_t pin)
{
    uint32_t val = (pin & 0x1F);

    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;

    if((pin >> 5) == 0)
    {
        BITBAND_PERI(LCD_F->PCTL0, val) = 1;
    }
    else
    {
        BITBAND_PERI(LCD_F->PCTL1, val) = 1;
    }
}

void LCD_F_setPinAsPortFunction(uint_fast8_t pin)
{
    uint32_t val = (pin & 0x1F);

    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;

    if((pin >> 5) == 0)
    {
        BITBAND_PERI(LCD_F->PCTL0, val) = 0;
    }
    else
    {
        BITBAND_PERI(LCD_F->PCTL1, val) = 0;
    }
}

void LCD_F_setPinsAsLCDFunction(uint_fast8_t startPin, uint8_t endPin)
{
    uint32_t startIdx = startPin >> 5;
    uint32_t endIdx = endPin >> 5;
    uint32_t startPos = startPin & 0x1F;
    uint32_t endPos = endPin & 0x1F;

    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;

    if (startIdx == endIdx)
    {
        if (startIdx == 0)
        {
            LCD_F->PCTL0 |= (0xFFFFFFFF >> (31 - endPos))
                    & (0xFFFFFFFF << startPos);
        } else
        {
            LCD_F->PCTL1 |= (0xFFFFFFFF >> (31 - endPos))
                    & (0xFFFFFFFF << startPos);
        }
    } else
    {
        LCD_F->PCTL0 |= (0xFFFFFFFF << startPos);
        LCD_F->PCTL1 |= (0xFFFFFFFF >> (31 - endPos));
    }
}

void LCD_F_setPinAsCOM(uint8_t pin, uint_fast8_t com)
{
    uint32_t val = (pin & 0x1F);

    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;

    if((pin >> 5) == 0)
    {
        BITBAND_PERI(LCD_F->CSSEL0, val) = 1;
    }
    else
    {
        BITBAND_PERI(LCD_F->CSSEL1, val) = 1;
    }

    // Setting the relevant COM pin
    HWREG8(LCD_F_BASE + OFS_LCDM0W + pin) |= com;
    HWREG8(LCD_F_BASE + OFS_LCDBM0W + pin) |= com;

}

void LCD_F_setPinAsSEG(uint_fast8_t pin)
{
    uint32_t val = (pin & 0x1F);

    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;

    if((pin >> 5) == 0)
    {
        BITBAND_PERI(LCD_F->CSSEL0, val) = 0;
    }
    else
    {
        BITBAND_PERI(LCD_F->CSSEL1, val) = 0;
    }
}

void LCD_F_selectBias(uint_fast16_t bias)
{
    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;
    LCD_F->VCTL = (LCD_F->VCTL & ~LCD_F_VCTL_LCD2B) | bias;
}

void LCD_F_setVLCDSource(uint_fast16_t v2v3v4Source, uint_fast16_t v5Source)
{
    BITBAND_PERI(LCD_F->CTL, LCD_F_CTL_ON_OFS) = 0;
    LCD_F->VCTL = (LCD_F->VCTL
            & ~(LCD_F_VCTL_REXT | LCD_F_VCTL_EXTBIAS
                    | LCD_F_VCTL_R03EXT)) | v2v3v4Source | v5Source;
}

/* Interrupt Management */
void LCD_F_clearInterrupt(uint32_t mask)
{
    LCD_F->CLRIFG |= mask;
}

uint32_t LCD_F_getInterruptStatus(void)
{
    return LCD_F->IFG;
}

uint32_t LCD_F_getEnabledInterruptStatus(void)
{
    return (LCD_F->IFG & LCD_F->IE);
}

void LCD_F_enableInterrupt(uint32_t mask)
{
    LCD_F->IE |= mask;
}

void LCD_F_disableInterrupt(uint32_t mask)
{
    LCD_F->IE &= ~mask;
}

void LCD_F_registerInterrupt(void (*intHandler)(void))
{
    Interrupt_registerInterrupt(INT_LCD_F, intHandler);
    Interrupt_enableInterrupt(INT_LCD_F);
}

void LCD_F_unregisterInterrupt(void)
{
    Interrupt_disableInterrupt(INT_LCD_F);
    Interrupt_unregisterInterrupt(INT_LCD_F);
}

#endif /* __MCU_HAS_LCD_F__ */
