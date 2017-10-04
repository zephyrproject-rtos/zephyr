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
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>


void RTC_C_startClock(void)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;
    BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_HOLD_OFS) = 0;
    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

void RTC_C_holdClock(void)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;
    BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_HOLD_OFS) = 1;
    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

void RTC_C_setCalibrationFrequency(uint_fast16_t frequencySelect)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;
    RTC_C->CTL13 = (RTC_C->CTL13 & ~(RTC_C_CTL13_CALF_3)) | frequencySelect;
    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

void RTC_C_setCalibrationData(uint_fast8_t offsetDirection,
        uint_fast8_t offsetValue)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;
    RTC_C->OCAL = offsetValue + offsetDirection;
    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

bool RTC_C_setTemperatureCompensation(uint_fast16_t offsetDirection,
        uint_fast8_t offsetValue)
{
    while (!BITBAND_PERI(RTC_C->TCMP, RTC_C_TCMP_TCRDY_OFS))
        ;

    RTC_C->TCMP = offsetValue + offsetDirection;

    if (BITBAND_PERI(RTC_C->TCMP, RTC_C_TCMP_TCOK_OFS))
        return true;
    else
        return false;
}

void RTC_C_initCalendar(const RTC_C_Calendar *calendarTime,
        uint_fast16_t formatSelect)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;

    BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_HOLD_OFS) = 1;

    if (formatSelect)
        BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_BCD_OFS) = 1;
    else
        BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_BCD_OFS) = 0;

    RTC_C->TIM0 = (calendarTime->minutes << RTC_C_TIM0_MIN_OFS)
            | calendarTime->seconds;
    RTC_C->TIM1 = (calendarTime->dayOfWeek << RTC_C_TIM1_DOW_OFS)
            | calendarTime->hours;
    RTC_C->DATE = (calendarTime->month << RTC_C_DATE_MON_OFS)
            | calendarTime->dayOfmonth;
    RTC_C->YEAR = calendarTime->year;

    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}


RTC_C_Calendar RTC_C_getCalendarTime(void)
{
    RTC_C_Calendar tempCal;

    while (!(BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_RDY_OFS)))
        ;

    tempCal.seconds = RTC_C->TIM0
            & (RTC_C_TIM0_SEC_LD_MASK | RTC_C_TIM0_SEC_HD_MASK);
    tempCal.minutes = (RTC_C->TIM0
            & (RTC_C_TIM0_MIN_LD_MASK | RTC_C_TIM0_MIN_HD_MASK))
            >> RTC_C_TIM0_MIN_OFS;
    tempCal.hours = RTC_C->TIM1
            & (RTC_C_TIM1_HOUR_LD_MASK | RTC_C_TIM1_HOUR_HD_MASK);
    tempCal.dayOfWeek = (RTC_C->TIM1
            & (RTC_C_TIM1_DOW_MASK)) >> RTC_C_TIM1_DOW_OFS;
    tempCal.dayOfmonth = RTC_C->DATE
            & (RTC_C_DATE_DAY_LD_MASK | RTC_C_DATE_DAY_HD_MASK);
    tempCal.month = (RTC_C->DATE & (RTC_C_DATE_MON_LD_MASK | RTC_C_DATE_MON_HD))
            >> RTC_C_DATE_MON_OFS;
    tempCal.year = RTC_C->YEAR;

    return (tempCal);
}


void RTC_C_setCalendarAlarm(uint_fast8_t minutesAlarm, uint_fast8_t hoursAlarm,
        uint_fast8_t dayOfWeekAlarm, uint_fast8_t dayOfmonthAlarm)
{
    //Each of these is XORed with 0x80 to turn on if an integer is passed,
    //or turn OFF if RTC_ALARM_OFF (0x80) is passed.
    RTC_C->AMINHR = ((hoursAlarm ^ 0x80) << 8 )| (minutesAlarm ^ 0x80);
    RTC_C->ADOWDAY = ((dayOfmonthAlarm ^ 0x80) << 8 )| (dayOfWeekAlarm ^ 0x80);
}

void RTC_C_setCalendarEvent(uint_fast16_t eventSelect)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;
    RTC_C->CTL13 = (RTC_C->CTL13 & ~(RTC_C_CTL13_TEV_3)) | eventSelect;
    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

void RTC_C_definePrescaleEvent(uint_fast8_t prescaleSelect,
        uint_fast8_t prescaleEventDivider)
{   
    if(prescaleSelect == RTC_C_PRESCALE_0)
    {
        HWREG8(&RTC_C->PS0CTL) &= ~(RTC_C_PS0CTL_RT0IP_7);
        HWREG8(&RTC_C->PS0CTL) |= prescaleEventDivider;
    }
    else if(prescaleSelect == RTC_C_PRESCALE_1)
    {
        HWREG8(&RTC_C->PS1CTL) &= ~(RTC_C_PS0CTL_RT0IP_7);
        HWREG8(&RTC_C->PS1CTL) |= prescaleEventDivider;
    }
}

uint_fast8_t RTC_C_getPrescaleValue(uint_fast8_t prescaleSelect)
{
    if (RTC_C_PRESCALE_0 == prescaleSelect)
    {
        return (RTC_C->PS & RTC_C_PS_RT0PS_MASK);
    } else if (RTC_C_PRESCALE_1 == prescaleSelect)
    {
        return (RTC_C->PS & RTC_C_PS_RT1PS_MASK)>>RTC_C_PS_RT1PS_OFS;
    } else
    {
        return (0);
    }
}

void RTC_C_setPrescaleValue(uint_fast8_t prescaleSelect,
        uint_fast8_t prescaleCounterValue)
{
    RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;

    if (RTC_C_PRESCALE_0 == prescaleSelect)
    {
        RTC_C->PS = (RTC_C->PS & ~RTC_C_PS_RT0PS_MASK) | prescaleCounterValue;
    } else if (RTC_C_PRESCALE_1 == prescaleSelect)
    {
        RTC_C->PS = (RTC_C->PS & ~RTC_C_PS_RT1PS_MASK)
                | (prescaleCounterValue << RTC_C_PS_RT1PS_OFS);
    }

    BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
}

uint16_t RTC_C_convertBCDToBinary(uint16_t valueToConvert)
{
    RTC_C->BCD2BIN = valueToConvert;
    return (RTC_C->BCD2BIN);
}

uint16_t RTC_C_convertBinaryToBCD(uint16_t valueToConvert)
{
    RTC_C->BIN2BCD = valueToConvert;
    return (RTC_C->BIN2BCD);
}

void RTC_C_enableInterrupt(uint8_t interruptMask)
{
    if (interruptMask
            & (RTC_C_CTL0_OFIE + RTC_C_CTL0_TEVIE + RTC_C_CTL0_AIE
                    + RTC_C_CTL0_RDYIE))
    {
        RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK)
                | (RTC_C_KEY
                        | (interruptMask
                                & (RTC_C_CTL0_OFIE + RTC_C_CTL0_TEVIE
                                        + RTC_C_CTL0_AIE + RTC_C_CTL0_RDYIE)));
        BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
    }

    if (interruptMask & RTC_C_PRESCALE_TIMER0_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS0CTL, RTC_C_PS0CTL_RT0PSIE_OFS) = 1;
    }

    if (interruptMask & RTC_C_PRESCALE_TIMER1_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS1CTL,RTC_C_PS1CTL_RT1PSIE_OFS) = 1;
    }
}

void RTC_C_disableInterrupt(uint8_t interruptMask)
{
    uint16_t allIntMask = (RTC_C_CTL0_OFIE + RTC_C_CTL0_TEVIE + RTC_C_CTL0_AIE
            + RTC_C_CTL0_RDYIE);

    if (interruptMask & allIntMask)
    {
        RTC_C->CTL0 = (RTC_C->CTL0
                & ~((interruptMask & allIntMask) | RTC_C_CTL0_KEY_MASK))
                | RTC_C_KEY;

        BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
    }

    if (interruptMask & RTC_C_PRESCALE_TIMER0_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS0CTL, RTC_C_PS0CTL_RT0PSIE_OFS) = 0;
    }

    if (interruptMask & RTC_C_PRESCALE_TIMER1_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS1CTL, RTC_C_PS1CTL_RT1PSIE_OFS) = 0;
    }
}

uint_fast8_t RTC_C_getInterruptStatus(void)
{
    uint_fast8_t tempInterruptFlagMask = 0x00;
    uint_fast8_t interruptFlagMask = RTC_C_TIME_EVENT_INTERRUPT
            | RTC_C_CLOCK_ALARM_INTERRUPT | RTC_C_CLOCK_READ_READY_INTERRUPT
            | RTC_C_PRESCALE_TIMER0_INTERRUPT | RTC_C_PRESCALE_TIMER1_INTERRUPT
            | RTC_C_OSCILLATOR_FAULT_INTERRUPT;

    tempInterruptFlagMask |= (RTC_C->CTL0 & (interruptFlagMask >> 4));

    tempInterruptFlagMask = tempInterruptFlagMask << 4;

    if (interruptFlagMask & RTC_C_PRESCALE_TIMER0_INTERRUPT)
    {
        if (BITBAND_PERI(RTC_C->PS0CTL, RTC_C_PS0CTL_RT0PSIFG_OFS))
        {
            tempInterruptFlagMask |= RTC_C_PRESCALE_TIMER0_INTERRUPT;
        }
    }

    if (interruptFlagMask & RTC_C_PRESCALE_TIMER1_INTERRUPT)
    {
        if (BITBAND_PERI(RTC_C->PS1CTL, RTC_C_PS1CTL_RT1PSIFG_OFS))
        {
            tempInterruptFlagMask |= RTC_C_PRESCALE_TIMER1_INTERRUPT;
        }
    }

    return (tempInterruptFlagMask);
}

uint_fast8_t RTC_C_getEnabledInterruptStatus(void)
{

    uint32_t intStatus = RTC_C_getInterruptStatus();

    if (!BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_OFIE_OFS))
    {
        intStatus &= ~RTC_C_OSCILLATOR_FAULT_INTERRUPT;
    }

    if (!BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_TEVIE_OFS))
    {
        intStatus &= ~RTC_C_TIME_EVENT_INTERRUPT;
    }

    if (!BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_AIE_OFS))
    {
        intStatus &= ~RTC_C_CLOCK_ALARM_INTERRUPT;
    }

    if (!BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_RDYIE_OFS))
    {
        intStatus &= ~RTC_C_CLOCK_READ_READY_INTERRUPT;
    }

    if (!BITBAND_PERI(RTC_C->PS0CTL, RTC_C_PS0CTL_RT0PSIE_OFS))
    {
        intStatus &= ~RTC_C_PRESCALE_TIMER0_INTERRUPT;
    }

    if (!BITBAND_PERI(RTC_C->PS1CTL, RTC_C_PS1CTL_RT1PSIE_OFS))
    {
        intStatus &= ~RTC_C_PRESCALE_TIMER1_INTERRUPT;
    }

    return intStatus;
}

void RTC_C_clearInterruptFlag(uint_fast8_t interruptFlagMask)
{
    if (interruptFlagMask
            & (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT
                    + RTC_C_CLOCK_READ_READY_INTERRUPT
                    + RTC_C_OSCILLATOR_FAULT_INTERRUPT))
    {
        RTC_C->CTL0 = RTC_C_KEY
                | (RTC_C->CTL0 & ~((interruptFlagMask >> 4) | RTC_C_CTL0_KEY_MASK));
        BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
    }

    if (interruptFlagMask & RTC_C_PRESCALE_TIMER0_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS0CTL, RTC_C_PS0CTL_RT0PSIFG_OFS) = 0;
    }

    if (interruptFlagMask & RTC_C_PRESCALE_TIMER1_INTERRUPT)
    {
        BITBAND_PERI(RTC_C->PS1CTL, RTC_C_PS1CTL_RT1PSIFG_OFS) = 0;
    }
}

void RTC_C_registerInterrupt(void (*intHandler)(void))
{
    Interrupt_registerInterrupt(INT_RTC_C, intHandler);
    Interrupt_enableInterrupt(INT_RTC_C);
}

void RTC_C_unregisterInterrupt(void)
{
    Interrupt_disableInterrupt(INT_RTC_C);
    Interrupt_unregisterInterrupt(INT_RTC_C);
}

