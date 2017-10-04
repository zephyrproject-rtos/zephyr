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
#include <ti/devices/msp432p4xx/driverlib/timer_a.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

static void privateTimer_AProcessClockSourceDivider(uint32_t timer,
        uint16_t clockSourceDivider)
{
    TIMER_A_CMSIS(timer)->CTL &= ~TIMER_A_CTL_ID__8;
    TIMER_A_CMSIS(timer)->EX0 &= ~TIMER_A_EX0_IDEX_MASK;

    switch (clockSourceDivider)
    {
    case TIMER_A_CLOCKSOURCE_DIVIDER_1:
    case TIMER_A_CLOCKSOURCE_DIVIDER_2:
        TIMER_A_CMSIS(timer)->CTL |= ((clockSourceDivider - 1) << 6);
        TIMER_A_CMSIS(timer)->EX0 = TIMER_A_EX0_TAIDEX_0;
        break;
    case TIMER_A_CLOCKSOURCE_DIVIDER_4:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__4;
        TIMER_A_CMSIS(timer)->EX0 = TIMER_A_EX0_TAIDEX_0;
        break;
    case TIMER_A_CLOCKSOURCE_DIVIDER_8:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__8;
        TIMER_A_CMSIS(timer)->EX0 = TIMER_A_EX0_TAIDEX_0;
        break;
    case TIMER_A_CLOCKSOURCE_DIVIDER_3:
    case TIMER_A_CLOCKSOURCE_DIVIDER_5:
    case TIMER_A_CLOCKSOURCE_DIVIDER_6:
    case TIMER_A_CLOCKSOURCE_DIVIDER_7:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__1;
        TIMER_A_CMSIS(timer)->EX0 = (clockSourceDivider - 1);
        break;

    case TIMER_A_CLOCKSOURCE_DIVIDER_10:
    case TIMER_A_CLOCKSOURCE_DIVIDER_12:
    case TIMER_A_CLOCKSOURCE_DIVIDER_14:
    case TIMER_A_CLOCKSOURCE_DIVIDER_16:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__2;
        TIMER_A_CMSIS(timer)->EX0 = (clockSourceDivider / 2 - 1);
        break;

    case TIMER_A_CLOCKSOURCE_DIVIDER_20:
    case TIMER_A_CLOCKSOURCE_DIVIDER_24:
    case TIMER_A_CLOCKSOURCE_DIVIDER_28:
    case TIMER_A_CLOCKSOURCE_DIVIDER_32:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__4;
        TIMER_A_CMSIS(timer)->EX0 = (clockSourceDivider / 4 - 1);
        break;
    case TIMER_A_CLOCKSOURCE_DIVIDER_40:
    case TIMER_A_CLOCKSOURCE_DIVIDER_48:
    case TIMER_A_CLOCKSOURCE_DIVIDER_56:
    case TIMER_A_CLOCKSOURCE_DIVIDER_64:
        TIMER_A_CMSIS(timer)->CTL |= TIMER_A_CTL_ID__8;
        TIMER_A_CMSIS(timer)->EX0 = (clockSourceDivider / 8 - 1);
        break;
    }
}

void Timer_A_startCounter(uint32_t timer, uint_fast16_t timerMode)
{
    ASSERT(
            (TIMER_A_UPDOWN_MODE == timerMode)
            || (TIMER_A_CONTINUOUS_MODE == timerMode)
            || (TIMER_A_UP_MODE == timerMode));

    TIMER_A_CMSIS(timer)->CTL |= timerMode;
}

void Timer_A_configureContinuousMode(uint32_t timer,
        const Timer_A_ContinuousModeConfig *config)
{
    ASSERT(
            (TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_ACLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_SMCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
                    == config->clockSource));

    ASSERT(
            (TIMER_A_DO_CLEAR == config->timerClear)
            || (TIMER_A_SKIP_CLEAR == config->timerClear));

    ASSERT(
            (TIMER_A_TAIE_INTERRUPT_ENABLE == config->timerInterruptEnable_TAIE)
            || (TIMER_A_TAIE_INTERRUPT_DISABLE
                    == config->timerInterruptEnable_TAIE));

    ASSERT(
            (TIMER_A_CLOCKSOURCE_DIVIDER_1 == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_2
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_4
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_8
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_3
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_5
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_6
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_7
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_10
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_12
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_14
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_16
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_20
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_24
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_28
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_32
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_40
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_48
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_56
                    == config->clockSourceDivider)
            || (TIMER_A_CLOCKSOURCE_DIVIDER_64
                    == config->clockSourceDivider));

    privateTimer_AProcessClockSourceDivider(timer, config->clockSourceDivider);

    TIMER_A_CMSIS(timer)->CTL = (TIMER_A_CMSIS(timer)->CTL
            & ~(TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
                    + TIMER_A_UPDOWN_MODE + TIMER_A_DO_CLEAR
                    + TIMER_A_TAIE_INTERRUPT_ENABLE))
            | (config->clockSource + config->timerClear
                    + config->timerInterruptEnable_TAIE);
}

void Timer_A_configureUpMode(uint32_t timer, const Timer_A_UpModeConfig *config)
{
    ASSERT(
            (TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_ACLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_SMCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
                    == config->clockSource));

    ASSERT(
            (TIMER_A_DO_CLEAR == config->timerClear)
            || (TIMER_A_SKIP_CLEAR == config->timerClear));

    ASSERT(
            (TIMER_A_DO_CLEAR == config->timerClear)
            || (TIMER_A_SKIP_CLEAR == config->timerClear));

    privateTimer_AProcessClockSourceDivider(timer, config->clockSourceDivider);

    TIMER_A_CMSIS(timer)->CTL &=
            ~(TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK + TIMER_A_UPDOWN_MODE
                    + TIMER_A_DO_CLEAR + TIMER_A_TAIE_INTERRUPT_ENABLE);

    TIMER_A_CMSIS(timer)->CTL |= (config->clockSource + config->timerClear
            + config->timerInterruptEnable_TAIE);

    if (TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE
            == config->captureCompareInterruptEnable_CCR0_CCIE)
        BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[0],TIMER_A_CCTLN_CCIE_OFS) = 1;
    else
        BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[0],TIMER_A_CCTLN_CCIE_OFS) = 0;

    TIMER_A_CMSIS(timer)->CCR[0] = config->timerPeriod;
}

void Timer_A_configureUpDownMode(uint32_t timer,
        const Timer_A_UpDownModeConfig *config)
{
    ASSERT(
            (TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_ACLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_SMCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
                    == config->clockSource));

    ASSERT(
            (TIMER_A_DO_CLEAR == config->timerClear)
            || (TIMER_A_SKIP_CLEAR == config->timerClear));

    ASSERT(
            (TIMER_A_DO_CLEAR == config->timerClear)
            || (TIMER_A_SKIP_CLEAR == config->timerClear));

    privateTimer_AProcessClockSourceDivider(timer, config->clockSourceDivider);

    TIMER_A_CMSIS(timer)->CTL &=
            ~(TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK + TIMER_A_UPDOWN_MODE
                    + TIMER_A_DO_CLEAR + TIMER_A_TAIE_INTERRUPT_ENABLE);

    TIMER_A_CMSIS(timer)->CTL |= (config->clockSource + TIMER_A_STOP_MODE
            + config->timerClear + config->timerInterruptEnable_TAIE);
    if (TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE
            == config->captureCompareInterruptEnable_CCR0_CCIE)
        BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[0],TIMER_A_CCTLN_CCIE_OFS) = 1;
    else
        BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[0],TIMER_A_CCTLN_CCIE_OFS) = 0;

    TIMER_A_CMSIS(timer)->CCR[0] = config->timerPeriod;
}

void Timer_A_initCapture(uint32_t timer,
        const Timer_A_CaptureModeConfig *config)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == config->captureRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == config->captureRegister));

    ASSERT(
            (TIMER_A_CAPTUREMODE_NO_CAPTURE == config->captureMode)
            || (TIMER_A_CAPTUREMODE_RISING_EDGE == config->captureMode)
            || (TIMER_A_CAPTUREMODE_FALLING_EDGE == config->captureMode)
            || (TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE
                    == config->captureMode));

    ASSERT(
            (TIMER_A_CAPTURE_INPUTSELECT_CCIxA == config->captureInputSelect)
            || (TIMER_A_CAPTURE_INPUTSELECT_CCIxB
                    == config->captureInputSelect)
            || (TIMER_A_CAPTURE_INPUTSELECT_GND
                    == config->captureInputSelect)
            || (TIMER_A_CAPTURE_INPUTSELECT_Vcc
                    == config->captureInputSelect));

    ASSERT(
            (TIMER_A_CAPTURE_ASYNCHRONOUS == config->synchronizeCaptureSource)
            || (TIMER_A_CAPTURE_SYNCHRONOUS
                    == config->synchronizeCaptureSource));

    ASSERT(
            (TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE
                    == config->captureInterruptEnable)
            || (TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE
                    == config->captureInterruptEnable));

    ASSERT(
            (TIMER_A_OUTPUTMODE_OUTBITVALUE == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_SET == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_RESET
                    == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_SET_RESET
                    == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_SET
                    == config->captureOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET_SET
                    == config->captureOutputMode));

    if (TIMER_A_CAPTURECOMPARE_REGISTER_0 == config->captureRegister)
    {
        //CaptureCompare register 0 only supports certain modes
        ASSERT(
                (TIMER_A_OUTPUTMODE_OUTBITVALUE == config->captureOutputMode)
                || (TIMER_A_OUTPUTMODE_SET == config->captureOutputMode)
                || (TIMER_A_OUTPUTMODE_TOGGLE
                        == config->captureOutputMode)
                || (TIMER_A_OUTPUTMODE_RESET
                        == config->captureOutputMode));
    }
    uint8_t idx = (config->captureRegister>>1)-1;
    TIMER_A_CMSIS(timer)->CCTL[idx] =
            (TIMER_A_CMSIS(timer)->CCTL[idx]
                    & ~(TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE
                            | TIMER_A_CAPTURE_INPUTSELECT_Vcc
                            | TIMER_A_CAPTURE_SYNCHRONOUS | TIMER_A_DO_CLEAR
                            | TIMER_A_TAIE_INTERRUPT_ENABLE | TIMER_A_CCTLN_CM_3))
                    | (config->captureMode | config->captureInputSelect
                            | config->synchronizeCaptureSource
                            | config->captureInterruptEnable
                            | config->captureOutputMode | TIMER_A_CCTLN_CAP);

}

void Timer_A_initCompare(uint32_t timer,
        const Timer_A_CompareModeConfig *config)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == config->compareRegister));

    ASSERT(
            (TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE
                    == config->compareInterruptEnable)
            || (TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE
                    == config->compareInterruptEnable));

    ASSERT(
            (TIMER_A_OUTPUTMODE_OUTBITVALUE == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_SET == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_RESET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_SET_RESET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_SET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET_SET
                    == config->compareOutputMode));

    if (TIMER_A_CAPTURECOMPARE_REGISTER_0 == config->compareRegister)
    {
        //CaptureCompare register 0 only supports certain modes
        ASSERT(
                (TIMER_A_OUTPUTMODE_OUTBITVALUE == config->compareOutputMode)
                || (TIMER_A_OUTPUTMODE_SET == config->compareOutputMode)
                || (TIMER_A_OUTPUTMODE_TOGGLE
                        == config->compareOutputMode)
                || (TIMER_A_OUTPUTMODE_RESET
                        == config->compareOutputMode));
    }

    uint8_t idx = (config->compareRegister>>1)-1;
    TIMER_A_CMSIS(timer)->CCTL[idx] =
            (TIMER_A_CMSIS(timer)->CCTL[idx]
             & ~(TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE
             | TIMER_A_OUTPUTMODE_RESET_SET | TIMER_A_CCTLN_CAP))
             | (config->compareInterruptEnable + config->compareOutputMode);

   TIMER_A_CMSIS(timer)->CCR[idx] = config->compareValue;

}

uint16_t Timer_A_getCounterValue(uint32_t timer)
{
    uint_fast16_t voteOne, voteTwo, res;

    voteTwo = TIMER_A_CMSIS(timer)->R;

    do
    {
        voteOne = voteTwo;
        voteTwo = TIMER_A_CMSIS(timer)->R;

        if (voteTwo > voteOne)
            res = voteTwo - voteOne;
        else if (voteOne > voteTwo)
            res = voteOne - voteTwo;
        else
            res = 0;

    } while (res > TIMER_A_THRESHOLD);

    return voteTwo;

}

void Timer_A_clearTimer(uint32_t timer)
{
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CTL , TIMER_A_CTL_CLR_OFS) = 1;
}

uint_fast8_t Timer_A_getSynchronizedCaptureCompareInput(uint32_t timer,
        uint_fast16_t captureCompareRegister, uint_fast16_t synchronizedSetting)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    ASSERT(
            (TIMER_A_READ_CAPTURE_COMPARE_INPUT == synchronizedSetting)
            || (TIMER_A_READ_SYNCHRONIZED_CAPTURECOMPAREINPUT
                    == synchronizedSetting));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    if (TIMER_A_CMSIS(timer)->CCTL[idx] & synchronizedSetting)
        return TIMER_A_CAPTURECOMPARE_INPUT_HIGH;
    else
        return TIMER_A_CAPTURECOMPARE_INPUT_LOW;
}

uint_fast8_t Timer_A_getOutputForOutputModeOutBitValue(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    if (BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[idx],TIMER_A_CCTLN_OUT_OFS))
        return TIMER_A_OUTPUTMODE_OUTBITVALUE_HIGH;
    else
        return TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW;
}

uint_fast16_t Timer_A_getCaptureCompareCount(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    return (TIMER_A_CMSIS(timer)->CCR[idx]);
}

void Timer_A_setOutputForOutputModeOutBitValue(uint32_t timer,
        uint_fast16_t captureCompareRegister,
        uint_fast8_t outputModeOutBitValue)
{
    uint8_t idx = (captureCompareRegister>>1) - 1;
    TIMER_A_CMSIS(timer)->CCTL[idx] =
            ((TIMER_A_CMSIS(timer)->CCTL[idx])
                    & ~(TIMER_A_OUTPUTMODE_RESET_SET))
                    | (outputModeOutBitValue);
}

void Timer_A_generatePWM(uint32_t timer, const Timer_A_PWMConfig *config)
{
    ASSERT(
            (TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_ACLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_SMCLK == config->clockSource)
            || (TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
                    == config->clockSource));

    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == config->compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == config->compareRegister));

    ASSERT(
            (TIMER_A_OUTPUTMODE_OUTBITVALUE == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_SET == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_RESET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_SET_RESET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_TOGGLE_SET
                    == config->compareOutputMode)
            || (TIMER_A_OUTPUTMODE_RESET_SET
                    == config->compareOutputMode));

    privateTimer_AProcessClockSourceDivider(timer, config->clockSourceDivider);

    TIMER_A_CMSIS(timer)->CTL &=
            ~(TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK + TIMER_A_UPDOWN_MODE
                    + TIMER_A_DO_CLEAR + TIMER_A_TAIE_INTERRUPT_ENABLE);

    TIMER_A_CMSIS(timer)->CTL |= (config->clockSource + TIMER_A_UP_MODE
            + TIMER_A_DO_CLEAR);

    TIMER_A_CMSIS(timer)->CCR[0] = config->timerPeriod;

    TIMER_A_CMSIS(timer)->CCTL[0] &= ~(TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE
            + TIMER_A_OUTPUTMODE_RESET_SET);

    uint8_t idx = (config->compareRegister>>1) - 1;
    TIMER_A_CMSIS(timer)->CCTL[idx] |= config->compareOutputMode;
    TIMER_A_CMSIS(timer)->CCR[idx] = config->dutyCycle;
}

void Timer_A_stopTimer(uint32_t timer)
{
    TIMER_A_CMSIS(timer)->CTL &= ~TIMER_A_CTL_MC_3;
}

void Timer_A_setCompareValue(uint32_t timer, uint_fast16_t compareRegister,
        uint_fast16_t compareValue)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5 == compareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6 == compareRegister));

    uint8_t idx = (compareRegister>>1) - 1;
    TIMER_A_CMSIS(timer)->CCR[idx] = compareValue;
}

void Timer_A_clearInterruptFlag(uint32_t timer)
{
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CTL,TIMER_A_CTL_IFG_OFS) = 0;
}

void Timer_A_clearCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[idx],TIMER_A_CCTLN_CCIFG_OFS) = 0;
}

void Timer_A_enableInterrupt(uint32_t timer)
{
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CTL,TIMER_A_CTL_IE_OFS) = 1;
}

void Timer_A_disableInterrupt(uint32_t timer)
{
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CTL,TIMER_A_CTL_IE_OFS) = 0;
}

uint32_t Timer_A_getInterruptStatus(uint32_t timer)
{
    return (TIMER_A_CMSIS(timer)->CTL) & TIMER_A_CTL_IFG;
}

void Timer_A_enableCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[idx],TIMER_A_CCTLN_CCIE_OFS) = 1;
}

void Timer_A_disableCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    ASSERT(
            (TIMER_A_CAPTURECOMPARE_REGISTER_0 == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_1
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_2
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_3
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_4
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_5
                    == captureCompareRegister)
            || (TIMER_A_CAPTURECOMPARE_REGISTER_6
                    == captureCompareRegister));

    uint8_t idx = (captureCompareRegister>>1) - 1;
    BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[idx],TIMER_A_CCTLN_CCIE_OFS) = 0;

}

uint32_t Timer_A_getCaptureCompareInterruptStatus(uint32_t timer,
        uint_fast16_t captureCompareRegister, uint_fast16_t mask)
{
    uint8_t idx = (captureCompareRegister>>1) - 1;
    return (TIMER_A_CMSIS(timer)->CCTL[idx]) & mask;
}

uint32_t Timer_A_getEnabledInterruptStatus(uint32_t timer)
{
    if (TIMER_A_CMSIS(timer)->CTL & TIMER_A_CTL_IE)
    {
        return Timer_A_getInterruptStatus(timer);
    } else
    {
        return 0;
    }

}

uint32_t Timer_A_getCaptureCompareEnabledInterruptStatus(uint32_t timer,
        uint_fast16_t captureCompareRegister)
{
    uint8_t idx = (captureCompareRegister>>1) - 1;
    if (BITBAND_PERI(TIMER_A_CMSIS(timer)->CCTL[idx],TIMER_A_CCTLN_CCIE_OFS))
        return Timer_A_getCaptureCompareInterruptStatus(timer,
                captureCompareRegister,
                TIMER_A_CAPTURE_OVERFLOW |
                TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
    else
        return 0;
}

void Timer_A_registerInterrupt(uint32_t timer, uint_fast8_t interruptSelect,
        void (*intHandler)(void))
{
    if (interruptSelect == TIMER_A_CCR0_INTERRUPT)
    {
        switch (timer)
        {
        case TIMER_A0_BASE:
            Interrupt_registerInterrupt(INT_TA0_0, intHandler);
            Interrupt_enableInterrupt(INT_TA0_0);
            break;
        case TIMER_A1_BASE:
            Interrupt_registerInterrupt(INT_TA1_0, intHandler);
            Interrupt_enableInterrupt(INT_TA1_0);
            break;
        case TIMER_A2_BASE:
            Interrupt_registerInterrupt(INT_TA2_0, intHandler);
            Interrupt_enableInterrupt(INT_TA2_0);
            break;
        case TIMER_A3_BASE:
            Interrupt_registerInterrupt(INT_TA3_0, intHandler);
            Interrupt_enableInterrupt(INT_TA3_0);
            break;
        default:
            ASSERT(false);
        }
    } else if (interruptSelect == TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT)
    {
        switch (timer)
        {
        case TIMER_A0_BASE:
            Interrupt_registerInterrupt(INT_TA0_N, intHandler);
            Interrupt_enableInterrupt(INT_TA0_N);
            break;
        case TIMER_A1_BASE:
            Interrupt_registerInterrupt(INT_TA1_N, intHandler);
            Interrupt_enableInterrupt(INT_TA1_N);
            break;
        case TIMER_A2_BASE:
            Interrupt_registerInterrupt(INT_TA2_N, intHandler);
            Interrupt_enableInterrupt(INT_TA2_N);
            break;
        case TIMER_A3_BASE:
            Interrupt_registerInterrupt(INT_TA3_N, intHandler);
            Interrupt_enableInterrupt(INT_TA3_N);
            break;
        default:
            ASSERT(false);
        }
    } else
    {
        ASSERT(false);
    }
}

void Timer_A_unregisterInterrupt(uint32_t timer, uint_fast8_t interruptSelect)
{
    if (interruptSelect == TIMER_A_CCR0_INTERRUPT)
    {
        switch (timer)
        {
        case TIMER_A0_BASE:
            Interrupt_disableInterrupt(INT_TA0_0);
            Interrupt_unregisterInterrupt(INT_TA0_0);
            break;
        case TIMER_A1_BASE:
            Interrupt_disableInterrupt(INT_TA1_0);
            Interrupt_unregisterInterrupt(INT_TA1_0);
            break;
        case TIMER_A2_BASE:
            Interrupt_disableInterrupt(INT_TA2_0);
            Interrupt_unregisterInterrupt(INT_TA2_0);
            break;
        case TIMER_A3_BASE:
            Interrupt_disableInterrupt(INT_TA3_0);
            Interrupt_unregisterInterrupt(INT_TA3_0);
            break;
        default:
            ASSERT(false);
        }
    } else if (interruptSelect == TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT)
    {
        switch (timer)
        {
        case TIMER_A0_BASE:
            Interrupt_disableInterrupt(INT_TA0_N);
            Interrupt_unregisterInterrupt(INT_TA0_N);
            break;
        case TIMER_A1_BASE:
            Interrupt_disableInterrupt(INT_TA1_N);
            Interrupt_unregisterInterrupt(INT_TA1_N);
            break;
        case TIMER_A2_BASE:
            Interrupt_disableInterrupt(INT_TA2_N);
            Interrupt_unregisterInterrupt(INT_TA2_N);
            break;
        case TIMER_A3_BASE:
            Interrupt_disableInterrupt(INT_TA3_N);
            Interrupt_unregisterInterrupt(INT_TA3_N);
            break;
        default:
            ASSERT(false);
        }
    } else
    {
        ASSERT(false);
    }
}

