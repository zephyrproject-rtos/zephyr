/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
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
 */

/*
 * By default disable both asserts and log for this module.
 * This must be done before DebugP.h is included.
 */
#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif
#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED 0
#endif

#include <stdbool.h>

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/pwm/PWMTimerCC32XX.h>
#include <ti/drivers/timer/TimerCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_apps_config.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_timer.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/gpio.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/timer.h>

#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_STATE 0xC61

void PWMTimerCC32XX_close(PWM_Handle handle);
int_fast16_t PWMTimerCC32XX_control(PWM_Handle handle, uint_fast16_t cmd,
        void *arg);
void PWMTimerCC32XX_init(PWM_Handle handle);
PWM_Handle PWMTimerCC32XX_open(PWM_Handle handle, PWM_Params *params);
int_fast16_t PWMTimerCC32XX_setDuty(PWM_Handle handle, uint32_t dutyValue);
int_fast16_t PWMTimerCC32XX_setPeriod(PWM_Handle handle, uint32_t periodValue);
void PWMTimerCC32XX_start(PWM_Handle handle);
void PWMTimerCC32XX_stop(PWM_Handle handle);

/* PWM function table for PWMTimerCC32XX implementation */
const PWM_FxnTable PWMTimerCC32XX_fxnTable = {
    PWMTimerCC32XX_close,
    PWMTimerCC32XX_control,
    PWMTimerCC32XX_init,
    PWMTimerCC32XX_open,
    PWMTimerCC32XX_setDuty,
    PWMTimerCC32XX_setPeriod,
    PWMTimerCC32XX_start,
    PWMTimerCC32XX_stop
};

/*
 * Internal value to notify an error has occurred while calculating a duty
 * or period.
 */
static const uint32_t PWM_INVALID_VALUE = (~0);

/*
 * GPT peripheral load & match registers are 16 bits wide.  Max value which
 * can be set is 65535.
 */
static const uint16_t PWM_MAX_MATCH_REG_VALUE = (~0);

/*
 * GPT peripherals have 24 bit resolution.  The max period value which be
 * set is 16777215.
 */
static const uint32_t PWM_MAX_PERIOD_COUNT = (0xFFFFFF);

/*
 *  The following fields are used by CC32XX driverlib APIs and therefore
 *  must be populated by driverlib macro definitions. For CC32XX driverlib
 *  these definitions are found in:
 *      - inc/hw_memmap.h
 *      - driverlib/gpio.h
 *      - driverlib/pin.h
 *      - driverlib/timer.h
 */
static const uint32_t timerBaseAddresses[4] = {
    TIMERA0_BASE,
    TIMERA1_BASE,
    TIMERA2_BASE,
    TIMERA3_BASE,
};

static const uint32_t timerHalves[2] = {
    TIMER_A,
    TIMER_B,
};

static const uint32_t gpioBaseAddresses[4] = {
    GPIOA0_BASE,
    GPIOA1_BASE,
    GPIOA2_BASE,
    GPIOA3_BASE,
};

static const uint32_t gpioPinIndexes[8] = {
    GPIO_PIN_0,
    GPIO_PIN_1,
    GPIO_PIN_2,
    GPIO_PIN_3,
    GPIO_PIN_4,
    GPIO_PIN_5,
    GPIO_PIN_6,
    GPIO_PIN_7,
};

#define PinConfigTimerPort(config)     (((config) >> 28) & 0xF)
#define PinConfigTimerHalf(config)     (((config) >> 24) & 0xF)
#define PinConfigGPIOPort(config)      (((config) >> 20) & 0xF)
#define PinConfigGPIOPinIndex(config)  (((config) >> 16) & 0xF)
#define PinConfigPinMode(config)       (((config) >> 8) & 0xF)
#define PinConfigPin(config)           (((config) >> 0) & 0x3F)

/*
 *  ======== getDutyCounts ========
 */
static uint32_t getDutyCounts(PWM_Duty_Units dutyUnits, uint32_t dutyValue,
    uint32_t periodCounts)
{
    uint32_t      duty = 0;
    ClockP_FreqHz freq;

    ClockP_getCpuFreq(&freq);

    switch (dutyUnits) {
        case PWM_DUTY_COUNTS:
            duty = dutyValue;
            break;

        case PWM_DUTY_FRACTION:
            duty = (((uint64_t) dutyValue) * ((uint64_t) periodCounts)) /
                PWM_DUTY_FRACTION_MAX;
            break;

        case PWM_DUTY_US:
            duty = dutyValue * (freq.lo/1000000);
            break;

        default:
            /* Unsupported duty units return an invalid duty */
            duty = PWM_INVALID_VALUE;
    }

    return (duty);
}

/*
 *  ======== getPeriodCounts ========
 */
static uint32_t getPeriodCounts(PWM_Period_Units periodUnits,
    uint32_t periodValue)
{
    uint32_t      period = 0;
    ClockP_FreqHz freq;

    ClockP_getCpuFreq(&freq);

    switch (periodUnits) {
        case PWM_PERIOD_COUNTS:
            period = periodValue;
            break;

        case PWM_PERIOD_HZ:
            if (periodValue && periodValue <= freq.lo) {
                period = freq.lo / periodValue;
            }
            break;

        case PWM_PERIOD_US:
            period = periodValue * (freq.lo/1000000);
            break;

        default:
            /* Unsupported period units return an invalid period */
            period = PWM_INVALID_VALUE;
    }

    return (period);
}

/*
 *  ======== getPowerMgrId ========
 */
static uint_fast16_t getPowerMgrId(uint32_t baseAddr)
{
    switch (baseAddr) {
        case GPIOA0_BASE:
            return (PowerCC32XX_PERIPH_GPIOA0);
        case GPIOA1_BASE:
            return (PowerCC32XX_PERIPH_GPIOA1);
        case GPIOA2_BASE:
            return (PowerCC32XX_PERIPH_GPIOA2);
        case GPIOA3_BASE:
            return (PowerCC32XX_PERIPH_GPIOA3);
        case GPIOA4_BASE:
            return (PowerCC32XX_PERIPH_GPIOA4);
        default:
            /* Should never get here */
            return ((unsigned int) -1);
    }
}

/*
 *  ======== initHw ========
 */
static int initHw(PWM_Handle handle, uint32_t period, uint32_t duty)
{
    uintptr_t                       key;
    int32_t                         result;
    uint32_t                        timerConfigVal;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    uint32_t timerBaseAddr;
    uint16_t halfTimer;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    halfTimer = timerHalves[PinConfigTimerHalf(hwAttrs->pwmPin)];

    key = HwiP_disable();

    MAP_TimerDisable(timerBaseAddr, halfTimer);

    /*
     * The CC32XX SDK TimerConfigure API halts both timers when it is
     * used to configure a single half timer.  The code below performs
     * the register operations necessary to configure each half timer
     * individually.
     */
    /* Enable CCP to IO path */
    HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_GPT_TRIG_SEL) = 0xFF;

    /* Split the timer and configure it as a PWM */
    timerConfigVal = ((halfTimer & (TIMER_CFG_A_PWM | TIMER_CFG_B_PWM)) |
        TIMER_CFG_SPLIT_PAIR);
    HWREG(timerBaseAddr + TIMER_O_CFG) |= (timerConfigVal >> 24);
    if (halfTimer & TIMER_A) {
        HWREG(timerBaseAddr + TIMER_O_TAMR) = timerConfigVal & 255;
    }
    else {
        HWREG(timerBaseAddr + TIMER_O_TBMR) = (timerConfigVal >> 8) & 255;
    }

    /* Set the peripheral output to active-high */
    MAP_TimerControlLevel(timerBaseAddr, halfTimer, true);

    HwiP_restore(key);

    result = PWMTimerCC32XX_setPeriod(handle, period);
    if (result != PWM_STATUS_SUCCESS) {
        return (result);
    }

    result = PWMTimerCC32XX_setDuty(handle, duty);
    if (result != PWM_STATUS_SUCCESS) {
        return (result);
    }

    return (PWM_STATUS_SUCCESS);
}

/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up from LPDS.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    PWM_Handle             handle = (PWM_Handle) clientArg;
    PWMTimerCC32XX_Object *object = handle->object;

    initHw(handle, object->period, object->duty);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== PWMTimerCC32XX_close ========
 *  @pre    Function assumes that the handle is not NULL
 */
void PWMTimerCC32XX_close(PWM_Handle handle)
{
    PWMTimerCC32XX_Object          *object  = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_SubTimer            subTimer;
    uint32_t                        timerBaseAddr;
    uint32_t                        gpioBaseAddr;
    uint32_t                        padRegister;
    uintptr_t                       key;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];

    subTimer = (TimerCC32XX_SubTimer) (TimerCC32XX_timer16A +
        PinConfigTimerHalf(hwAttrs->pwmPin));

    /*
     * Some PWM pins may not have GPIO capability; in these cases gpioBaseAddr
     * is set to 0 & the GPIO power dependencies are not released.
     */
    gpioBaseAddr = (PinConfigGPIOPort(hwAttrs->pwmPin) == 0xF) ?
        0 : gpioBaseAddresses[PinConfigGPIOPort(hwAttrs->pwmPin)];

    PWMTimerCC32XX_stop(handle);

    key = HwiP_disable();

    TimerCC32XX_freeTimerResource(timerBaseAddr, subTimer);

    /* Remove GPIO power dependency if pin is GPIO capable */
    if (gpioBaseAddr) {
        Power_releaseDependency(getPowerMgrId(gpioBaseAddr));
    }

    Power_unregisterNotify(&object->postNotify);

    padRegister = (PinToPadGet((hwAttrs->pwmPin) & 0x3f)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;

    object->isOpen = false;

    HwiP_restore(key);

    DebugP_log1("PWM:(%p) is closed", (uintptr_t) handle);
}

/*
 *  ======== PWMTimerCC32XX_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t PWMTimerCC32XX_control(PWM_Handle handle, uint_fast16_t cmd,
    void *arg)
{
    /* No implementation yet */
    return (PWM_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== PWMTimerCC32XX_init ========
 *  @pre    Function assumes that the handle is not NULL
 */
void PWMTimerCC32XX_init(PWM_Handle handle)
{
}

/*
 *  ======== PWMTimerCC32XX_open ========
 *  @pre    Function assumes that the handle is not NULL
 */
PWM_Handle PWMTimerCC32XX_open(PWM_Handle handle, PWM_Params *params)
{
    uintptr_t                       key;
    PWMTimerCC32XX_Object          *object = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_SubTimer            subTimer;
    uint32_t                        timerBaseAddr;
    uint32_t                        gpioBaseAddr;
    uint16_t                        pin;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    pin = PinConfigPin(hwAttrs->pwmPin);

    subTimer = (TimerCC32XX_SubTimer) (TimerCC32XX_timer16A +
        PinConfigTimerHalf(hwAttrs->pwmPin));

    key = HwiP_disable();

    if (object->isOpen) {
        HwiP_restore(key);

        DebugP_log1("PWM:(%p) already opened.", (uintptr_t) handle);

        return (NULL);
    }

    if (!TimerCC32XX_allocateTimerResource(timerBaseAddr, subTimer)) {
        HwiP_restore(key);

        DebugP_log1("Timer: 0x%X unavailable.", timerBaseAddr);

        return (NULL);
    }

    object->isOpen = true;

    HwiP_restore(key);

    /*
     * Some PWM pins may not have GPIO capability; in these cases gpioBaseAddr
     * is set to 0 & the GPIO power dependencies are not set.
     */
    gpioBaseAddr = (PinConfigGPIOPort(hwAttrs->pwmPin) == 0xF) ?
        0 : gpioBaseAddresses[PinConfigGPIOPort(hwAttrs->pwmPin)];

    /* Set GPIO power dependency if pin is GPIO capable */
    if (gpioBaseAddr) {
        /* Check GPIO power resource Id */
        if (getPowerMgrId(gpioBaseAddr) == ((unsigned int) -1)) {
            TimerCC32XX_freeTimerResource(timerBaseAddr, subTimer);

            object->isOpen = false;

            DebugP_log1("PWM:(%p) Failed to determine GPIO power resource ID.",
                (uintptr_t) handle);

            return (NULL);
        }

        /* Register power dependency for GPIO port */
        Power_setDependency(getPowerMgrId(gpioBaseAddr));
    }

    Power_registerNotify(&object->postNotify, PowerCC32XX_AWAKE_LPDS,
        postNotifyFxn, (uintptr_t) handle);

    /*
     * Set PWM duty to initial value (not 0) - required when inverting
     * output polarity to generate a duty equal to 0 or period.  See comments in
     * PWMTimerCC32XX_setDuty for more information.
     */
    object->duty = 0;
    object->period = 0;
    object->dutyUnits = params->dutyUnits;
    object->idleLevel = params->idleLevel;
    object->periodUnits = params->periodUnits;
    object->pwmStarted = 0;

    /* Initialize the peripheral & set the period & duty */
    if (initHw(handle, params->periodValue, params->dutyValue) !=
        PWM_STATUS_SUCCESS) {
        PWMTimerCC32XX_close(handle);

        DebugP_log1("PWM:(%p) Failed set initial PWM configuration.",
            (uintptr_t) handle);

        return (NULL);
    }

    /* Configure the Power_pinParkState based on idleLevel param */
    PowerCC32XX_setParkState((PowerCC32XX_Pin) pin,
        (object->idleLevel == PWM_IDLE_HIGH));

    /* Called to set the initial idleLevel */
    PWMTimerCC32XX_stop(handle);

    DebugP_log3("PWM:(%p) opened; period set to: %d; duty set to: %d",
        (uintptr_t) handle, params->periodValue, params->dutyValue);

    return (handle);
}

/*
 *  ======== PWMTimerCC32XX_setDuty ========
 *  @pre    Function assumes that handle is not NULL
 */
int_fast16_t PWMTimerCC32XX_setDuty(PWM_Handle handle, uint32_t dutyValue)
{
    uintptr_t                       key;
    uint32_t                        duty;
    uint32_t                        period;
    PWMTimerCC32XX_Object          *object = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    uint32_t timerBaseAddr;
    uint16_t halfTimer;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    halfTimer = timerHalves[PinConfigTimerHalf(hwAttrs->pwmPin)];

    key = HwiP_disable();

    period = object->period;
    duty = getDutyCounts(object->dutyUnits, dutyValue, period);

    if (duty == PWM_INVALID_VALUE) {
        HwiP_restore(key);

        DebugP_log1("PWM:(%p) duty units could not be determined.",
            (uintptr_t) handle);

        return (PWM_STATUS_ERROR);
    }

    if (duty > period) {
        HwiP_restore(key);

        DebugP_log1("PWM:(%p) duty is out of range.", (uintptr_t) handle);

        return (PWM_STATUS_INVALID_DUTY);
    }

    /*
     * The timer peripheral cannot generate a duty equal to the period when
     * the timer is counting down.  In these cases the PWM duty is set to the
     * period value (output remains low) and output polarity is inverted.
     * Additionally, if the output is changed from the period the PWM output
     * polarity must be inverted again.
     *
     * The code below uses the previous duty (object->duty) and the new duty to
     * determine if the polarity should be inverted.
     * For more details refer to the device specific datasheet and the following
     * E2E post:
     *  http://e2e.ti.com/support/microcontrollers/tiva_arm/f/908/t/354826.aspx
     */
    if (((duty == period) && (object->duty != period)) ||
        ((duty != period) && (object->duty == period))) {
        HWREG(timerBaseAddr + TIMER_O_CTL) ^=
            (halfTimer & (TIMER_CTL_TAPWML | TIMER_CTL_TBPWML));
    }

    /*
     * Set & store the new duty.  IMPORTANT: this must be saved after output
     * inversion is determined and before the duty = 0 corner case.
     */
    object->duty = duty;

    /*
     * Special corner case, if duty is 0 we set it to the period without
     * inverting output
     */
    if (duty == 0) {
        duty = period;
    }

    MAP_TimerPrescaleMatchSet(timerBaseAddr, halfTimer,
        duty / PWM_MAX_MATCH_REG_VALUE);
    MAP_TimerMatchSet(timerBaseAddr, halfTimer,
        duty % PWM_MAX_MATCH_REG_VALUE);

    HwiP_restore(key);

    DebugP_log2("PWM:(%p) duty set to: %d", (uintptr_t) handle, dutyValue);

    return (PWM_STATUS_SUCCESS);
}

/*
 *  ======== PWMTimerCC32XX_setPeriod ========
 *  @pre    Function assumes that handle is not NULL
 */
int_fast16_t PWMTimerCC32XX_setPeriod(PWM_Handle handle, uint32_t periodValue)
{
    uintptr_t                       key;
    uint32_t                        duty;
    uint32_t                        period;
    PWMTimerCC32XX_Object          *object = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    uint32_t timerBaseAddr;
    uint16_t halfTimer;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    halfTimer = timerHalves[PinConfigTimerHalf(hwAttrs->pwmPin)];

    key = HwiP_disable();

    duty = object->duty;
    period = getPeriodCounts(object->periodUnits, periodValue);

    if (period == PWM_INVALID_VALUE) {
        HwiP_restore(key);

        DebugP_log1("PWM:(%p) period units could not be determined.",
            (uintptr_t) handle);

        return (PWM_STATUS_ERROR);
    }

    if ((period == 0) || (period <= duty) || (period > PWM_MAX_PERIOD_COUNT)) {
        HwiP_restore(key);

        DebugP_log1("PWM:(%p) period is out of range.", (uintptr_t) handle);

        return (PWM_STATUS_INVALID_PERIOD);
    }

    /* Set the new period */
    object->period = period;
    MAP_TimerPrescaleSet(timerBaseAddr, halfTimer,
        period / PWM_MAX_MATCH_REG_VALUE);
    MAP_TimerLoadSet(timerBaseAddr, halfTimer,
        period % PWM_MAX_MATCH_REG_VALUE);

    HwiP_restore(key);

    DebugP_log2("PWM:(%p) period set to: %d", (uintptr_t) handle, periodValue);

    return (PWM_STATUS_SUCCESS);
}

/*
 *  ======== PWMTimerCC32XX_start ========
 *  @pre    Function assumes that handle is not NULL
 */
void PWMTimerCC32XX_start(PWM_Handle handle)
{
    uintptr_t                       key;
    PWMTimerCC32XX_Object          *object = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    uint32_t timerBaseAddr;
    uint16_t halfTimer;
    uint16_t pin;
    uint16_t mode;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    halfTimer = timerHalves[PinConfigTimerHalf(hwAttrs->pwmPin)];
    pin = PinConfigPin(hwAttrs->pwmPin);
    mode = PinConfigPinMode(hwAttrs->pwmPin);

    key = HwiP_disable();

    /*
     * GP timer ticks only in Active mode.  Cannot be used in HIB or LPDS.
     * Set constraint to disallow LPDS.
     */
    if (!(object->pwmStarted)) {
        Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
        object->pwmStarted = true;
    }

    /* Start the timer & set pinmux to PWM mode */
    MAP_TimerEnable(timerBaseAddr, halfTimer);

    MAP_PinTypeTimer((unsigned long)pin, (unsigned long)mode);

    HwiP_restore(key);

    DebugP_log1("PWM:(%p) started.", (uintptr_t) handle);
}

/*
 *  ======== PWMTimerCC32XX_stop ========
 *  @pre    Function assumes that handle is not NULL
 */
void PWMTimerCC32XX_stop(PWM_Handle handle)
{
    uintptr_t                       key;
    uint8_t                         output;
    PWMTimerCC32XX_Object          *object = handle->object;
    PWMTimerCC32XX_HWAttrsV2 const *hwAttrs = handle->hwAttrs;
    uint32_t timerBaseAddr;
    uint16_t halfTimer;
    uint32_t gpioBaseAddr;
    uint8_t gpioPinIndex;
    uint16_t pin;

    timerBaseAddr = timerBaseAddresses[PinConfigTimerPort(hwAttrs->pwmPin)];
    halfTimer = timerHalves[PinConfigTimerHalf(hwAttrs->pwmPin)];
    pin = PinConfigPin(hwAttrs->pwmPin);

    /*
     * Some PWM pins may not have GPIO capability; in these cases gpioBaseAddr
     * is set to 0 & the GPIO power dependencies are not set.
     */
    gpioBaseAddr = (PinConfigGPIOPort(hwAttrs->pwmPin) == 0xF) ?
        0 : gpioBaseAddresses[PinConfigGPIOPort(hwAttrs->pwmPin)];
    gpioPinIndex = (PinConfigGPIOPinIndex(hwAttrs->pwmPin) == 0xF) ?
        0 : gpioPinIndexes[PinConfigGPIOPinIndex(hwAttrs->pwmPin)];

    key = HwiP_disable();

    /* Remove the dependency to allow LPDS */
    if (object->pwmStarted) {
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
        object->pwmStarted = false;
    }

    /* Set pin as GPIO with IdleLevel value & stop the timer */
    output = (object->idleLevel) ? gpioPinIndex : 0;
    MAP_PinTypeGPIO((unsigned long)pin, PIN_MODE_0, false);

    /* Only configure the pin as GPIO if the pin is GPIO capable */
    if (gpioBaseAddr) {
        MAP_GPIODirModeSet(gpioBaseAddr, gpioPinIndex, GPIO_DIR_MODE_OUT);
        MAP_GPIOPinWrite(gpioBaseAddr, gpioPinIndex, output);
    }

    /* Stop the Timer */
    MAP_TimerDisable(timerBaseAddr, halfTimer);

    HwiP_restore(key);

    DebugP_log1("PWM:(%p) stopped.", (uintptr_t) handle);
}
