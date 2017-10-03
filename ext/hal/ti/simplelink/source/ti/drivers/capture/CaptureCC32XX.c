/*
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
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/ClockP.h>

#include <ti/drivers/capture/CaptureCC32XX.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/timer/TimerCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_timer.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_apps_config.h>
#include <ti/devices/cc32xx/driverlib/timer.h>

#define getTimerBaseAddress(config) (TIMERA0_BASE | ((config >> 18) & 0x3000))
#define getSubTimer(config)         ((config >> 28) & 0x3)
#define getTimerIntNum(config)      ((config >> 20) & 0xFF)
#define getPadOffset(config)        ((config >> 4) & 0xFFF)
#define getPinMode(config)          (config & 0xF)
#define getGPIOBaseAddress(config)  (GPIOA0_BASE + ((config >> 4) & 0xF000))

#define PAD_RESET_STATE 0xC61

void CaptureCC32XX_close(Capture_Handle handle);
int_fast16_t CaptureCC32XX_control(Capture_Handle handle,
    uint_fast16_t cmd, void *arg);
void CaptureCC32XX_init(Capture_Handle handle);
Capture_Handle CaptureCC32XX_open(Capture_Handle handle, Capture_Params *params);
int32_t CaptureCC32XX_start(Capture_Handle handle);
void CaptureCC32XX_stop(Capture_Handle handle);

/* Internal static Functions */
static void CaptureCC32XX_hwiIntFunction(uintptr_t arg);
static uint32_t getPowerMgrId(uint32_t baseAddress);
static void initHw(Capture_Handle handle);
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg);

/* System Clock Frequency */
static ClockP_FreqHz clockFreq;

/* Function table for CaptureCC32XX implementation */
const Capture_FxnTable CaptureCC32XX_fxnTable = {
    .closeFxn    = CaptureCC32XX_close,
    .openFxn     = CaptureCC32XX_open,
    .startFxn    = CaptureCC32XX_start,
    .stopFxn     = CaptureCC32XX_stop,
    .initFxn     = CaptureCC32XX_init,
    .controlFxn  = CaptureCC32XX_control
};

/*
 *  ======== CaptureCC32XX_close ========
 */
void CaptureCC32XX_close(Capture_Handle handle)
{
    CaptureCC32XX_Object *object  = handle->object;
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_SubTimer subTimer;
    uint32_t baseAddress = getTimerBaseAddress(hwAttrs->capturePin);

    subTimer = (TimerCC32XX_SubTimer) getSubTimer(hwAttrs->capturePin);

    CaptureCC32XX_stop(handle);

    Power_unregisterNotify(&(object->notifyObj));
    Power_releaseDependency(getPowerMgrId(getGPIOBaseAddress(hwAttrs->capturePin)));

    if (object->hwiHandle) {

        HwiP_delete(object->hwiHandle);
        object->hwiHandle = NULL;
    }

    TimerCC32XX_freeTimerResource(baseAddress, subTimer);

    /* Restore the GPIO pad to its reset state */
    HWREG(OCP_SHARED_BASE + getPadOffset(hwAttrs->capturePin))
        = PAD_RESET_STATE;
}

/*
 *  ======== CaptureCC32XX_control ========
 */
int_fast16_t CaptureCC32XX_control(Capture_Handle handle,
        uint_fast16_t cmd, void *arg)
{
    return (Capture_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== CaptureCC32XX_hwiIntFunction ========
 */
void CaptureCC32XX_hwiIntFunction(uintptr_t arg)
{
    Capture_Handle handle = (Capture_Handle) arg;
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    CaptureCC32XX_Object *object = handle->object;
    uint32_t baseAddress = getTimerBaseAddress(hwAttrs->capturePin);
    uint32_t interruptMask;
    uint32_t interval, currentCount;

    /* Read the TXR register */
    currentCount = TimerValueGet(baseAddress, object->timer);

    /* Calculate the interval */
    if (currentCount < object->previousCount) {

        /* Calculate the difference if the timer rolled over */
        interval = currentCount + (0xFFFFFF - object->previousCount);
    }
    else if (currentCount > object->previousCount) {

        interval = currentCount - object->previousCount - 1;
    }
    else {
        interval = 1;
    }

    /* Store the interval for the next interrupt */
    object->previousCount = currentCount;

    /* Compensate for prescale register roll-over hardware issue */
    interval = interval - (interval / 0xFFFF);

    /* Clear the interrupts used by this driver instance */
    interruptMask = object->timer & (TIMER_CAPB_EVENT | TIMER_CAPA_EVENT);
    TimerIntClear(baseAddress, interruptMask);

    /* Need to convert the interval to periodUnits if microseconds or hertz */
    if (object->periodUnits == Capture_PERIOD_US) {

        interval = interval / (clockFreq.lo / 1000000);
    }
    else if (object->periodUnits == Capture_PERIOD_HZ) {

        interval = clockFreq.lo / interval;
    }

    /* Call the user callbackFxn */
    object->callBack(handle, interval);
}

/*
 *  ======== CaptureCC32XX_init ========
 */
void CaptureCC32XX_init(Capture_Handle handle)
{
    return;
}

/*
 *  ======== CaptureCC32XX_open ========
 */
Capture_Handle CaptureCC32XX_open(Capture_Handle handle, Capture_Params *params)
{
    CaptureCC32XX_Object        *object = handle->object;
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    HwiP_Params                  hwiParams;
    uint32_t                     powerId;

    /* Check parameters. This driver requires a callback function. */
    if (params == NULL ||
        params->callbackFxn == NULL) {

        return (NULL);
    }

    powerId = getPowerMgrId(getGPIOBaseAddress(hwAttrs->capturePin));

    if (powerId == (uint32_t) -1) {

        return (NULL);
    }

    /* Attempt to allocate timer hardware resource */
    if (!TimerCC32XX_allocateTimerResource(getTimerBaseAddress(hwAttrs->capturePin),
           (TimerCC32XX_SubTimer) getSubTimer(hwAttrs->capturePin))) {

        return (NULL);
    }

    /* Turn on Power to the GPIO */
    Power_setDependency(powerId);

    /* Function to re-initialize the timer after a low-power event */
    Power_registerNotify(&(object->notifyObj), PowerCC32XX_AWAKE_LPDS,
        postNotifyFxn, (uintptr_t) handle);

    /* Determine which timer half will be used. */
    if (getSubTimer(hwAttrs->capturePin) == TimerCC32XX_timer16A) {

        object->timer = TIMER_A;
    }
    else {

        object->timer = TIMER_B;
    }

    /* Set the mode */
    if (params->mode == Capture_RISING_EDGE) {

        object->mode = TIMER_EVENT_POS_EDGE;
    }
    else if (params->mode == Capture_FALLING_EDGE) {

        object->mode = TIMER_EVENT_NEG_EDGE;
    }
    else {

        object->mode = TIMER_EVENT_BOTH_EDGES;
    }

    object->isRunning = false;
    object->callBack = params->callbackFxn;
    object->periodUnits = params->periodUnit;

    /* Setup the hardware interrupt function to handle timer interrupts */
    HwiP_Params_init(&hwiParams);
    hwiParams.arg = (uintptr_t) handle;
    hwiParams.priority = hwAttrs->intPriority;

    object->hwiHandle = HwiP_create(getTimerIntNum(hwAttrs->capturePin),
        CaptureCC32XX_hwiIntFunction, &hwiParams);

    if (object->hwiHandle == NULL) {

        CaptureCC32XX_close(handle);

        return (NULL);
    }

    /* Static global storing the CPU frequency */
    ClockP_getCpuFreq(&clockFreq);

    /* Initialize the timer hardware */
    initHw(handle);

    return (handle);
}

/*
 *  ======== CaptureCC32XX_start ========
 */
int32_t CaptureCC32XX_start(Capture_Handle handle)
{
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    CaptureCC32XX_Object *object = handle->object;
    uint32_t baseAddress = getTimerBaseAddress(hwAttrs->capturePin);
    uint32_t interruptMask;
    uintptr_t key;

    interruptMask = object->timer & (TIMER_CAPB_EVENT | TIMER_CAPA_EVENT);

    key = HwiP_disable();

    if (object->isRunning) {

        HwiP_restore(key);

        return (Capture_STATUS_ERROR);
    }

    object->isRunning = true;
    object->previousCount = 0;

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    TimerIntClear(baseAddress, interruptMask);
    TimerIntEnable(baseAddress, interruptMask);
    TimerValueSet(baseAddress, object->timer, 0);
    TimerEnable(baseAddress, object->timer);

    HwiP_restore(key);

    return (Capture_STATUS_SUCCESS);
}

/*
 *  ======== CaptureCC32XX_stop ========
 */
void CaptureCC32XX_stop(Capture_Handle handle)
{
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    CaptureCC32XX_Object *object = handle->object;
    uint32_t baseAddress = getTimerBaseAddress(hwAttrs->capturePin);
    uint32_t interruptMask;
    uintptr_t key;

    interruptMask = object->timer & (TIMER_CAPB_EVENT | TIMER_CAPA_EVENT);

    key = HwiP_disable();

    if (object->isRunning) {

        object->isRunning = false;

        TimerDisable(baseAddress, object->timer);
        TimerIntDisable(baseAddress, interruptMask);
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    }

    HwiP_restore(key);
}


/*
 *  ======== getPowerMgrId ========
 */
static uint32_t getPowerMgrId(uint32_t baseAddress)
{
    switch (baseAddress) {

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

            return ((uint32_t) -1);
    }
}

/*
 *  ======== initHw ========
 */
static void initHw(Capture_Handle handle)
{
    CaptureCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    CaptureCC32XX_Object  const *object = handle->object;
    uint32_t baseAddress = getTimerBaseAddress(hwAttrs->capturePin);
    uintptr_t key;

    /* Enable external GPT trigger */
    HWREG(APPS_CONFIG_BASE + APPS_CONFIG_O_GPT_TRIG_SEL) = 0xFF;

    /* Route the GPIO pad for capture usage */
    HWREG(OCP_SHARED_BASE + getPadOffset(hwAttrs->capturePin))
        = getPinMode(hwAttrs->capturePin);

    /* Read/Write modifications for shared registers */
    key = HwiP_disable();

    /* Disable the timer */
    TimerDisable(baseAddress, object->timer);

    /* Set trigger event for the capture pin */
    TimerControlEvent(baseAddress, object->timer, object->mode);

    /* This function controls the stall response for the timer. When true,
     * the timer stops counting if the processor is halted. The
     * default setting for the hardware is false.
     */
    TimerControlStall(baseAddress, object->timer, true);

    HwiP_restore(key);

    /* Configure in 16-bit mode */
    HWREG(baseAddress + TIMER_O_CFG) = TIMER_CFG_16_BIT;

    /* Configure in capture time edge mode, counting up */
    if (object->timer == TIMER_A) {

        HWREG(baseAddress + TIMER_O_TAMR) = TIMER_CFG_A_CAP_TIME_UP;
    }
    else {

        HWREG(baseAddress + TIMER_O_TBMR) = TIMER_CFG_A_CAP_TIME_UP;
    }
}

/*
 *  ======== postNotifyFxn ========
 *  This function is called when a transition from LPDS mode is made.
 *  clientArg should be a handle of a previously opened Timer instance.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    initHw((Capture_Handle) clientArg);

    return (Power_NOTIFYDONE);
}
