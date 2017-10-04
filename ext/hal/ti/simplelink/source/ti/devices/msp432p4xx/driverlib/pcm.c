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
/* Standard Includes */
#include <stdint.h>

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/pcm.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <ti/devices/msp432p4xx/driverlib/cpu.h>

static bool __PCM_setCoreVoltageLevelAdvanced(uint_fast8_t voltageLevel,
        uint32_t timeOut, bool blocking)
{
    uint8_t powerMode, bCurrentVoltageLevel;
    uint32_t regValue;
    bool boolTimeout;

    ASSERT(voltageLevel == PCM_VCORE0 || voltageLevel == PCM_VCORE1);

    /* Getting current power mode and level */
    powerMode = PCM_getPowerMode();
    bCurrentVoltageLevel = PCM_getCoreVoltageLevel();

    boolTimeout = timeOut > 0 ? true : false;

    /* If we are already at the power mode they requested, return */
    if (bCurrentVoltageLevel == voltageLevel)
        return true;

    while (bCurrentVoltageLevel != voltageLevel)
    {
        regValue = PCM->CTL0;

        switch (PCM_getPowerState())
        {
        case PCM_AM_LF_VCORE1:
        case PCM_AM_DCDC_VCORE1:
        case PCM_AM_LDO_VCORE0:
            PCM->CTL0 = (PCM_KEY | (PCM_AM_LDO_VCORE1)
                    | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            break;
        case PCM_AM_LF_VCORE0:
        case PCM_AM_DCDC_VCORE0:
        case PCM_AM_LDO_VCORE1:
            PCM->CTL0 = (PCM_KEY | (PCM_AM_LDO_VCORE0)
                    | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            break;
        default:
            ASSERT(false);
        }

        if(blocking)
        {
            while (BITBAND_PERI(PCM->CTL1, PCM_CTL1_PMR_BUSY_OFS))
            {
                if (boolTimeout && !(--timeOut))
                    return false;

            }
        }
        else
        {
            return true;
        }

        bCurrentVoltageLevel = PCM_getCoreVoltageLevel();
    }

    /* Changing the power mode if we are stuck in LDO mode */
    if (powerMode != PCM_getPowerMode())
    {
        if (powerMode == PCM_DCDC_MODE)
            return PCM_setPowerMode(PCM_DCDC_MODE);
        else
            return PCM_setPowerMode(PCM_LF_MODE);
    }

    return true;

}


bool PCM_setCoreVoltageLevel(uint_fast8_t voltageLevel)
{
    return __PCM_setCoreVoltageLevelAdvanced(voltageLevel, 0, true);
}

bool PCM_setCoreVoltageLevelWithTimeout(uint_fast8_t voltageLevel,
        uint32_t timeOut)
{
    return __PCM_setCoreVoltageLevelAdvanced(voltageLevel, timeOut, true);
}

bool PCM_setCoreVoltageLevelNonBlocking(uint_fast8_t voltageLevel)
{
    return __PCM_setCoreVoltageLevelAdvanced(voltageLevel, 0, false);
}

uint8_t PCM_getPowerMode(void)
{
    uint8_t currentPowerState;

    currentPowerState = PCM_getPowerState();

    switch (currentPowerState)
    {
    case PCM_AM_LDO_VCORE0:
    case PCM_AM_LDO_VCORE1:
    case PCM_LPM0_LDO_VCORE0:
    case PCM_LPM0_LDO_VCORE1:
        return PCM_LDO_MODE;
    case PCM_AM_DCDC_VCORE0:
    case PCM_AM_DCDC_VCORE1:
    case PCM_LPM0_DCDC_VCORE0:
    case PCM_LPM0_DCDC_VCORE1:
        return PCM_DCDC_MODE;
    case PCM_LPM0_LF_VCORE0:
    case PCM_LPM0_LF_VCORE1:
    case PCM_AM_LF_VCORE1:
    case PCM_AM_LF_VCORE0:
        return PCM_LF_MODE;
    default:
        ASSERT(false);
        return false;

    }
}

uint8_t PCM_getCoreVoltageLevel(void)
{
    uint8_t currentPowerState = PCM_getPowerState();

    switch (currentPowerState)
    {
    case PCM_AM_LDO_VCORE0:
    case PCM_AM_DCDC_VCORE0:
    case PCM_AM_LF_VCORE0:
    case PCM_LPM0_LDO_VCORE0:
    case PCM_LPM0_DCDC_VCORE0:
    case PCM_LPM0_LF_VCORE0:
        return PCM_VCORE0;
    case PCM_AM_LDO_VCORE1:
    case PCM_AM_DCDC_VCORE1:
    case PCM_AM_LF_VCORE1:
    case PCM_LPM0_LDO_VCORE1:
    case PCM_LPM0_DCDC_VCORE1:
    case PCM_LPM0_LF_VCORE1:
        return PCM_VCORE1;
    case PCM_LPM3:
        return PCM_VCORELPM3;
    default:
        ASSERT(false);
        return false;

    }
}

static bool __PCM_setPowerModeAdvanced(uint_fast8_t powerMode, uint32_t timeOut,
bool blocking)
{
    uint8_t bCurrentPowerMode, bCurrentPowerState;
    uint32_t regValue;
    bool boolTimeout;

    ASSERT(
            powerMode == PCM_LDO_MODE || powerMode == PCM_DCDC_MODE
            || powerMode == PCM_LF_MODE);

    /* Getting Current Power Mode */
    bCurrentPowerMode = PCM_getPowerMode();

    /* If the power mode being set it the same as the current mode, return */
    if (powerMode == bCurrentPowerMode)
        return true;

    bCurrentPowerState = PCM_getPowerState();

    boolTimeout = timeOut > 0 ? true : false;

    /* Go through the while loop while we haven't achieved the power mode */
    while (bCurrentPowerMode != powerMode)
    {
        regValue = PCM->CTL0;

        switch (bCurrentPowerState)
        {
        case PCM_AM_DCDC_VCORE0:
        case PCM_AM_LF_VCORE0:
            PCM->CTL0 = (PCM_KEY | PCM_AM_LDO_VCORE0
                    | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            break;
        case PCM_AM_LF_VCORE1:
        case PCM_AM_DCDC_VCORE1:
            PCM->CTL0 = (PCM_KEY | PCM_AM_LDO_VCORE1
                    | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            break;
        case PCM_AM_LDO_VCORE1:
        {
            if (powerMode == PCM_DCDC_MODE)
            {
                PCM->CTL0 = (PCM_KEY | PCM_AM_DCDC_VCORE1
                        | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            } else if (powerMode == PCM_LF_MODE)
            {
                PCM->CTL0 = (PCM_KEY | PCM_AM_LF_VCORE1
                        | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            } else
                ASSERT(false);

            break;
        }
        case PCM_AM_LDO_VCORE0:
        {
            if (powerMode == PCM_DCDC_MODE)
            {
                PCM->CTL0 = (PCM_KEY | PCM_AM_DCDC_VCORE0
                        | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            } else if (powerMode == PCM_LF_MODE)
            {
                PCM->CTL0 = (PCM_KEY | PCM_AM_LF_VCORE0
                        | (regValue & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_AMR_MASK)));
            } else
                ASSERT(false);

            break;
        }
        default:
            ASSERT(false);
        }

        if (blocking)
        {
            while (BITBAND_PERI(PCM->CTL1, PCM_CTL1_PMR_BUSY_OFS))
            {
                if (boolTimeout && !(--timeOut))
                    return false;

            }
        } else
            return true;

        bCurrentPowerMode = PCM_getPowerMode();
        bCurrentPowerState = PCM_getPowerState();
    }

    return true;

}

bool PCM_setPowerMode(uint_fast8_t powerMode)
{
    return __PCM_setPowerModeAdvanced(powerMode, 0, true);
}

bool PCM_setPowerModeNonBlocking(uint_fast8_t powerMode)
{
    return __PCM_setPowerModeAdvanced(powerMode, 0, false);
}

bool PCM_setPowerModeWithTimeout(uint_fast8_t powerMode, uint32_t timeOut)
{
    return __PCM_setPowerModeAdvanced(powerMode, timeOut, true);
}

static bool __PCM_setPowerStateAdvanced(uint_fast8_t powerState,
        uint32_t timeout,
        bool blocking)
{
    uint8_t bCurrentPowerState;
    bCurrentPowerState = PCM_getPowerState();

    ASSERT(
            powerState == PCM_AM_LDO_VCORE0 || powerState == PCM_AM_LDO_VCORE1
            || powerState == PCM_AM_DCDC_VCORE0 || powerState == PCM_AM_DCDC_VCORE1
            || powerState == PCM_AM_LF_VCORE0 || powerState == PCM_AM_LF_VCORE1
            || powerState == PCM_LPM0_LDO_VCORE0 || powerState == PCM_LPM0_LDO_VCORE1
            || powerState == PCM_LPM0_DCDC_VCORE0 || powerState == PCM_LPM0_DCDC_VCORE1
            || powerState == PCM_LPM3 || powerState == PCM_LPM35_VCORE0
            || powerState == PCM_LPM45 || powerState == PCM_LPM4);

    if (bCurrentPowerState == powerState)
        return true;

    switch (powerState)
    {
    case PCM_AM_LDO_VCORE0:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_LDO_MODE, timeout, blocking));
    case PCM_AM_LDO_VCORE1:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_LDO_MODE, timeout, blocking));
    case PCM_AM_DCDC_VCORE0:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_DCDC_MODE, timeout, blocking));
    case PCM_AM_DCDC_VCORE1:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_DCDC_MODE, timeout, blocking));
    case PCM_AM_LF_VCORE0:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_LF_MODE, timeout, blocking));
    case PCM_AM_LF_VCORE1:
        return (__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                && __PCM_setPowerModeAdvanced(PCM_LF_MODE, timeout, blocking));
    case PCM_LPM0_LDO_VCORE0:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_LDO_MODE, timeout, blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM0_LDO_VCORE1:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_LDO_MODE, timeout, blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM0_DCDC_VCORE0:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_DCDC_MODE, timeout,
                        blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM0_DCDC_VCORE1:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_DCDC_MODE, timeout,
                        blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM0_LF_VCORE0:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE0, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_LF_MODE, timeout, blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM0_LF_VCORE1:
        if (!__PCM_setCoreVoltageLevelAdvanced(PCM_VCORE1, timeout, blocking)
                || !__PCM_setPowerModeAdvanced(PCM_LF_MODE, timeout, blocking))
            break;
        return PCM_gotoLPM0();
    case PCM_LPM3:
        return PCM_gotoLPM3();
    case PCM_LPM4:
        return PCM_gotoLPM4();
    case PCM_LPM45:
        return PCM_shutdownDevice(PCM_LPM45);
    case PCM_LPM35_VCORE0:
        return PCM_shutdownDevice(PCM_LPM35_VCORE0);
    default:
        ASSERT(false);
        return false;
    }

    return false;

}

bool PCM_setPowerState(uint_fast8_t powerState)
{
    return __PCM_setPowerStateAdvanced(powerState, 0, true);
}

bool PCM_setPowerStateWithTimeout(uint_fast8_t powerState, uint32_t timeout)
{
    return __PCM_setPowerStateAdvanced(powerState, timeout, true);
}

bool PCM_setPowerStateNonBlocking(uint_fast8_t powerState)
{
    return __PCM_setPowerStateAdvanced(powerState, 0, false);
}

bool PCM_shutdownDevice(uint32_t shutdownMode)
{
    uint32_t shutdownModeBits = (shutdownMode == PCM_LPM45) ?
            PCM_CTL0_LPMR_12 : PCM_CTL0_LPMR_10;

    ASSERT(
            shutdownMode == PCM_SHUTDOWN_PARTIAL
            || shutdownMode == PCM_SHUTDOWN_COMPLETE);

    /* If a power transition is occuring, return false */
    if (BITBAND_PERI(PCM->CTL1, PCM_CTL1_PMR_BUSY_OFS))
        return false;

    /* Initiating the shutdown */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    PCM->CTL0 = (PCM_KEY | shutdownModeBits
            | (PCM->CTL0 & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_LPMR_MASK)));

    CPU_wfi();

    return true;
}

bool PCM_gotoLPM4(void)
{
    /* Disabling RTC_C and WDT_A */
    WDT_A_holdTimer();
    RTC_C_holdClock();

    /* LPM4 is just LPM3 with WDT_A/RTC_C disabled... */
    return PCM_gotoLPM3();
}

bool PCM_gotoLPM4InterruptSafe(void)
{
    bool slHappenedCorrect;

    /* Disabling master interrupts. In Cortex M, if an interrupt is enabled but
     master interrupts are disabled and a WFI happens the WFI will
     immediately exit. */
    Interrupt_disableMaster();

    slHappenedCorrect = PCM_gotoLPM4();

    /* Enabling and Disabling Interrupts very quickly so that the
     processor catches any pending interrupts */
    Interrupt_enableMaster();
    Interrupt_disableMaster();

    return slHappenedCorrect;
}

bool PCM_gotoLPM0(void)
{
    /* If we are in the middle of a state transition, return false */
    if (BITBAND_PERI(PCM->CTL1, PCM_CTL1_PMR_BUSY_OFS))
        return false;

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    CPU_wfi();

    return true;
}

bool PCM_gotoLPM0InterruptSafe(void)
{
    bool slHappenedCorrect;

    /* Disabling master interrupts. In Cortex M, if an interrupt is enabled but
     master interrupts are disabled and a WFI happens the WFI will
     immediately exit. */
    Interrupt_disableMaster();

    slHappenedCorrect = PCM_gotoLPM0();

    /* Enabling and Disabling Interrupts very quickly so that the
     processor catches any pending interrupts */
    Interrupt_enableMaster();
    Interrupt_disableMaster();

    return slHappenedCorrect;
}

bool PCM_gotoLPM3(void)
{
    uint_fast8_t bCurrentPowerState;
    uint_fast8_t currentPowerMode;

    /* If we are in the middle of a state transition, return false */
    if (BITBAND_PERI(PCM->CTL1, PCM_CTL1_PMR_BUSY_OFS))
        return false;

    /* If we are in the middle of a shutdown, return false */
    if ((PCM->CTL0 & PCM_CTL0_LPMR_MASK) == PCM_CTL0_LPMR_10
            || (PCM->CTL0 & PCM_CTL0_LPMR_MASK) == PCM_CTL0_LPMR_12)
        return false;

    currentPowerMode = PCM_getPowerMode();
    bCurrentPowerState = PCM_getPowerState();

    if (currentPowerMode == PCM_DCDC_MODE)
        PCM_setPowerMode(PCM_LDO_MODE);

    /* Clearing the SDR */
    PCM->CTL0 = (PCM->CTL0 & ~(PCM_CTL0_KEY_MASK | PCM_CTL0_LPMR_MASK)) | PCM_KEY;

    /* Setting the sleep deep bit */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    CPU_wfi();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    return PCM_setPowerState(bCurrentPowerState);
}

bool PCM_gotoLPM3InterruptSafe(void)
{
    bool lpmHappenedCorrect;

    /* Disabling master interrupts. In Cortex M, if an interrupt is enabled but
     master interrupts are disabled and a WFI happens the WFI will
     immediately exit. */
    Interrupt_disableMaster();

    lpmHappenedCorrect = PCM_gotoLPM3();

    /* Enabling and Disabling Interrupts very quickly so that the
     processor catches any pending interrupts */
    Interrupt_enableMaster();
    Interrupt_disableMaster();

    return lpmHappenedCorrect;
}

uint8_t PCM_getPowerState(void)
{
    return (PCM->CTL0 & PCM_CTL0_CPM_MASK) >> PCM_CTL0_CPM_OFS;
}

void PCM_enableRudeMode(void)
{

    PCM->CTL1 = (PCM->CTL1 & ~(PCM_CTL0_KEY_MASK)) | PCM_KEY
            | PCM_CTL1_FORCE_LPM_ENTRY;
}

void PCM_disableRudeMode(void)
{
    PCM->CTL1 = (PCM->CTL1 & ~(PCM_CTL0_KEY_MASK | PCM_CTL1_FORCE_LPM_ENTRY))
            | PCM_KEY;
}

void PCM_enableInterrupt(uint32_t flags)
{
    PCM->IE |= flags;
}

void PCM_disableInterrupt(uint32_t flags)
{
    PCM->IE &= ~flags;
}

uint32_t PCM_getInterruptStatus(void)
{
    return PCM->IFG;
}

uint32_t PCM_getEnabledInterruptStatus(void)
{
    return PCM_getInterruptStatus() & PCM->IE;
}

void PCM_clearInterruptFlag(uint32_t flags)
{
    PCM->CLRIFG |= flags;
}

void PCM_registerInterrupt(void (*intHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(INT_PCM, intHandler);

    //
    // Enable the system control interrupt.
    //
    Interrupt_enableInterrupt(INT_PCM);
}

void PCM_unregisterInterrupt(void)
{
    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(INT_PCM);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(INT_PCM);
}
