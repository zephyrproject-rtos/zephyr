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
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/ClockP.h>

#include <ti/drivers/Timer.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/timer/TimerCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_timer.h>
#include <ti/devices/cc32xx/driverlib/timer.h>

/*
 * This macro is used to determine a logical shift value for the
 * timerState.bitMask. Each timer peripheral occupies two bits in
 * timerState.bitMask.
 *
 * The timer peripherals' base addresses have an offset of 0x1000 starting at
 * 0x40030000. That byte is masked using 0xF000 which can result in a value
 * ranging from 0x0000 to 0x3000 for this particular hardware instance. This
 * value is then shifted right by 12 into the LSB. Lastly, the value is
 * multiplied by two because there are two bits in the timerState.bitMask for
 * each timer. The value returned is used for the logical shift.
 */
#define timerMaskShift(baseAddress) ((((baseAddress) & 0XF000) >> 12) * 2)

void TimerCC32XX_close(Timer_Handle handle);
int_fast16_t TimerCC32XX_control(Timer_Handle handle,
     uint_fast16_t cmd, void *arg);
uint32_t TimerCC32XX_getCount(Timer_Handle handle);
void TimerCC32XX_init(Timer_Handle handle);
Timer_Handle TimerCC32XX_open(Timer_Handle handle, Timer_Params *params);
int32_t TimerCC32XX_start(Timer_Handle handle);
void TimerCC32XX_stop(Timer_Handle handle);

/* Internal static Functions */
static void initHw(Timer_Handle handle);
static void getPrescaler(Timer_Handle handle);
static uint32_t getPowerMgrId(uint32_t baseAddress);
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg);
static void TimerCC32XX_hwiIntFunction(uintptr_t arg);

/* Function table for TimerCC32XX implementation */
const Timer_FxnTable TimerCC32XX_fxnTable = {
    .closeFxn    = TimerCC32XX_close,
    .openFxn     = TimerCC32XX_open,
    .startFxn    = TimerCC32XX_start,
    .stopFxn     = TimerCC32XX_stop,
    .initFxn     = TimerCC32XX_init,
    .getCountFxn = TimerCC32XX_getCount,
    .controlFxn  = TimerCC32XX_control
};

/*
 * Internal Timer status structure
 *
 * bitMask: Each timer peripheral occupies two bits in the bitMask. The least
 * significant bit represents the first half width timer, TimerCC32XX_timer16A
 * and the most significant bit represents the second half width timer,
 * TimerCC32XX_timer16B. If the full width timer, TimerCC32XX_timer32, is used,
 * both bits are set to 1.

 *    31 - 8     7 - 6    5 - 4    3 - 2    1 - 0
 *  ------------------------------------------------
 *  | Reserved | Timer3 | Timer2 | Timer1 | Timer0 |
 *  ------------------------------------------------
 */
static struct {
    uint32_t bitMask;
} timerState;

/*
 *  ======== initHw ========
 */
static void initHw(Timer_Handle handle)
{
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_Object  const *object = handle->object;

    /* Ensure the timer is disabled */
    TimerDisable(hwAttrs->baseAddress, object->timer);

    if (object->timer == TIMER_A) {

        HWREG(hwAttrs->baseAddress + TIMER_O_TAMR) = TIMER_TAMR_TAMR_PERIOD;
    }
    else {

        HWREG(hwAttrs->baseAddress + TIMER_O_TBMR) = TIMER_TBMR_TBMR_PERIOD;
    }

    if (hwAttrs->subTimer == TimerCC32XX_timer32) {

        HWREG(hwAttrs->baseAddress + TIMER_O_CFG) = TIMER_CFG_32_BIT_TIMER;
    }
    else {

        HWREG(hwAttrs->baseAddress + TIMER_O_CFG) = TIMER_CFG_16_BIT;
    }

    /* Disable all interrupts */
    HWREG(hwAttrs->baseAddress + TIMER_O_IMR) = ~object->timer;

    /* Writing the PSR Register has no effect for full width 32-bit mode */
    TimerPrescaleSet(hwAttrs->baseAddress, object->timer, object->prescaler);
    TimerLoadSet(hwAttrs->baseAddress, object->timer, object->period);

    /* This function controls the stall response for the timer. When true,
     * the timer stops counting if the processor enters debug mode. The
     * default setting for the hardware is false.
     */
    TimerControlStall(hwAttrs->baseAddress, object->timer, true);
}

/*
 * ========= getPrescaler =========
 * This function calculates the prescaler and timer interval load register
 * for a half timer. The handle is assumed to contain a object->period which
 * represents the number of clock cycles in the desired period. The calling
 * function, TimerCC32XX_open() checks for overflow before calling this function.
 * Therefore, this function is guaranteed to never fail.
 */
static void getPrescaler(Timer_Handle handle)
{
    TimerCC32XX_Object        *object = handle->object;
    uint32_t                   bestDiff = ~0, bestPsr = 0, bestIload = 0;
    uint32_t                   diff, intervalLoad, prescaler;

    /* Loop over the 8-bit prescaler */
    for (prescaler = 1; prescaler < 256; prescaler++) {

        /* Calculate timer interval load */
        intervalLoad = object->period / (prescaler + 1);

        /* Will this fit in 16-bits? */
        if (intervalLoad > (uint16_t) ~0) {
            continue;
        }

        /* How close is the intervalLoad to what we actually want? */
        diff = object->period - intervalLoad * (prescaler + 1);

        /* If it is closer to what we want */
        if (diff <= bestDiff) {

            /* If its a perfect match */
            if (diff == 0) {
                object->period = intervalLoad;
                object->prescaler = prescaler;

                return;
            }

            /* Snapshot in case we don't find something better */
            bestDiff = diff;
            bestPsr = prescaler;
            bestIload = intervalLoad;
        }
    }

    /* Never found a perfect match, settle for the best */
    object->period = bestIload;
    object->prescaler = bestPsr;
}

/*
 *  ======== getPowerMgrId ========
 */
static uint32_t getPowerMgrId(uint32_t baseAddress)
{
    switch (baseAddress) {

        case TIMERA0_BASE:

            return (PowerCC32XX_PERIPH_TIMERA0);

        case TIMERA1_BASE:

            return (PowerCC32XX_PERIPH_TIMERA1);

        case TIMERA2_BASE:

            return (PowerCC32XX_PERIPH_TIMERA2);

        case TIMERA3_BASE:

            return (PowerCC32XX_PERIPH_TIMERA3);

        default:

            return ((uint32_t) -1);
    }
}

/*
 *  ======== postNotifyFxn ========
 *  This functions is called when a transition from LPDS mode is made.
 *  clientArg should be a handle of a previously opened Timer instance.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    initHw((Timer_Handle) clientArg);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== TimerCC32XX_allocateTimerResource ========
 */
bool TimerCC32XX_allocateTimerResource(uint32_t baseAddress,
    TimerCC32XX_SubTimer subTimer)
{
    uintptr_t key;
    uint32_t  mask;
    uint32_t  powerMgrId;
    bool      status;

    powerMgrId = getPowerMgrId(baseAddress);

    if (powerMgrId == (uint32_t) -1) {

        return (false);
    }

    mask = subTimer << timerMaskShift(baseAddress);

    key = HwiP_disable();

    if (timerState.bitMask & mask) {

        status = false;
    }
    else {

        Power_setDependency(powerMgrId);
        timerState.bitMask = timerState.bitMask | mask;
        status = true;
    }

    HwiP_restore(key);

    return (status);
}

/*
 *  ======== TimerCC32XX_close ========
 */
void TimerCC32XX_close(Timer_Handle handle)
{
    TimerCC32XX_Object        *object = handle->object;
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Stopping the Timer before closing it */
    TimerCC32XX_stop(handle);

    Power_unregisterNotify(&(object->notifyObj));

    if (object->hwiHandle) {

        HwiP_clearInterrupt(hwAttrs->intNum);
        HwiP_delete(object->hwiHandle);
        object->hwiHandle = NULL;
    }

    if (object->timerSem) {

        SemaphoreP_delete(object->timerSem);
        object->timerSem = NULL;
    }

    TimerCC32XX_freeTimerResource(hwAttrs->baseAddress, hwAttrs->subTimer);
}

/*
 *  ======== TimerCC32XX_control ========
 */
int_fast16_t TimerCC32XX_control(Timer_Handle handle,
        uint_fast16_t cmd, void *arg)
{
    return (Timer_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== TimerCC32XX_freeTimerResource ========
 */
void TimerCC32XX_freeTimerResource(uint32_t baseAddress,
    TimerCC32XX_SubTimer subTimer)
{
    uintptr_t key;
    uint32_t  mask;

    mask = subTimer << timerMaskShift(baseAddress);

    key = HwiP_disable();

    timerState.bitMask = (timerState.bitMask & ~mask);

    Power_releaseDependency(getPowerMgrId(baseAddress));

    HwiP_restore(key);
}

/*
 *  ======== TimerCC32XX_getCount ========
 */
uint32_t TimerCC32XX_getCount(Timer_Handle handle)
{
    TimerCC32XX_HWAttrs const *hWAttrs = handle->hwAttrs;
    TimerCC32XX_Object  const *object = handle->object;
    uint32_t                   count;

    if (object->timer == TIMER_A) {
        count = HWREG(hWAttrs->baseAddress + TIMER_O_TAR);
    }
    else {
        count = HWREG(hWAttrs->baseAddress + TIMER_O_TBR);
    }

    /* Virtual up counter */
    count = object->period - count;

    return (count);
}

/*
 *  ======== TimerCC32XX_hwiIntFunction ========
 */
void TimerCC32XX_hwiIntFunction(uintptr_t arg)
{
    Timer_Handle handle = (Timer_Handle) arg;
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_Object  const *object  = handle->object;
    uint32_t                   interruptMask;

    /* Only clear the interrupt for this->object->timer */
    interruptMask = object->timer & (TIMER_TIMA_TIMEOUT | TIMER_TIMB_TIMEOUT);
    TimerIntClear(hwAttrs->baseAddress, interruptMask);

    /* Hwi is not created when using Timer_FREE_RUNNING */
    if (object->mode != Timer_CONTINUOUS_CALLBACK) {
        TimerCC32XX_stop(handle);
    }

    if (object-> mode != Timer_ONESHOT_BLOCKING) {
        object->callBack(handle);
    }
}

/*
 *  ======== TimerCC32XX_init ========
 */
void TimerCC32XX_init(Timer_Handle handle)
{
    return;
}

/*
 *  ======== TimerCC32XX_open ========
 */
Timer_Handle TimerCC32XX_open(Timer_Handle handle, Timer_Params *params)
{
    TimerCC32XX_Object        *object = handle->object;
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    SemaphoreP_Params          semParams;
    HwiP_Params                hwiParams;
    ClockP_FreqHz              clockFreq;

    /* Check for valid parameters */
    if (((params->timerMode == Timer_ONESHOT_CALLBACK ||
          params->timerMode == Timer_CONTINUOUS_CALLBACK) &&
          params->timerCallback == NULL) ||
          params->period == 0) {

        return (NULL);
    }

    if (!TimerCC32XX_allocateTimerResource(hwAttrs->baseAddress,
        hwAttrs->subTimer)) {

        return (NULL);
    }

    Power_registerNotify(&(object->notifyObj), PowerCC32XX_AWAKE_LPDS,
            postNotifyFxn, (uintptr_t) handle);

    object->mode = params->timerMode;
    object->isRunning = false;
    object->callBack = params->timerCallback;
    object->period = params->period;
    object->prescaler = 0;

    if (hwAttrs->subTimer == TimerCC32XX_timer16B) {

        object->timer = TIMER_B;
    }
    else {

        object->timer = TIMER_A;
    }

    if (object->mode != Timer_FREE_RUNNING) {

        HwiP_Params_init(&hwiParams);
        hwiParams.arg = (uintptr_t) handle;
        hwiParams.priority = hwAttrs->intPriority;
        object->hwiHandle = HwiP_create(hwAttrs->intNum,
            TimerCC32XX_hwiIntFunction, &hwiParams);

        if (object->hwiHandle == NULL) {

            TimerCC32XX_close(handle);

            return (NULL);
        }

    }

    /* Creating the semaphore if mode is blocking */
    if (params->timerMode == Timer_ONESHOT_BLOCKING) {

        SemaphoreP_Params_init(&semParams);
        semParams.mode = SemaphoreP_Mode_BINARY;
        object->timerSem = SemaphoreP_create(0, &semParams);

        if (object->timerSem == NULL) {

            TimerCC32XX_close(handle);

            return (NULL);
        }
    }

    /* Formality; CC32XX System Clock fixed to 80.0 MHz */
    ClockP_getCpuFreq(&clockFreq);

    if (params->periodUnits == Timer_PERIOD_US) {

        /* Checks if the calculated period will fit in 32-bits */
        if (object->period >= ((uint32_t) ~0) / (clockFreq.lo / 1000000)) {

            TimerCC32XX_close(handle);

            return (NULL);
        }

        object->period = object->period * (clockFreq.lo / 1000000);
    }
    else if (params->periodUnits == Timer_PERIOD_HZ) {

        /* If (object->period) > clockFreq */
        if ((object->period = clockFreq.lo / object->period) == 0) {

            TimerCC32XX_close(handle);

            return (NULL);
        }
    }

    /* If using a half timer */
    if (hwAttrs->subTimer != TimerCC32XX_timer32) {

        if (object->period > 0xFFFF) {

            /* 24-bit resolution for the half timer */
            if (object->period >= (1 << 24)) {

                TimerCC32XX_close(handle);

                return (NULL);
            }

            getPrescaler(handle);
        }
    }

    initHw(handle);

    return (handle);
}

/*
 *  ======== TimerCC32XX_start ========
 */
int32_t TimerCC32XX_start(Timer_Handle handle)
{
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_Object        *object = handle->object;
    uint32_t                   interruptMask;
    uintptr_t                  key;

    interruptMask = object->timer & (TIMER_TIMB_TIMEOUT | TIMER_TIMA_TIMEOUT);

    key = HwiP_disable();

    if (object->isRunning) {

        HwiP_restore(key);

        return (Timer_STATUS_ERROR);
    }

    object->isRunning = true;

    if (object->hwiHandle) {

        TimerIntEnable(hwAttrs->baseAddress, interruptMask);
    }

    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    /* Reload the timer */
    if (object->timer == TIMER_A) {
        HWREG(hwAttrs->baseAddress + TIMER_O_TAMR) |= TIMER_TAMR_TAILD;
    }
    else {
        HWREG(hwAttrs->baseAddress + TIMER_O_TBMR) |= TIMER_TBMR_TBILD;
    }

    TimerEnable(hwAttrs->baseAddress, object->timer);

    HwiP_restore(key);

    if (object->mode == Timer_ONESHOT_BLOCKING) {

        /* Pend forever, ~0 */
        SemaphoreP_pend(object->timerSem, ~0);
    }

    return (Timer_STATUS_SUCCESS);
}

/*
 *  ======== TimerCC32XX_stop ========
 */
void TimerCC32XX_stop(Timer_Handle handle)
{
    TimerCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    TimerCC32XX_Object        *object = handle->object;
    uint32_t                   interruptMask;
    uintptr_t                  key;

    interruptMask = object->timer & (TIMER_TIMB_TIMEOUT | TIMER_TIMA_TIMEOUT);

    key = HwiP_disable();

    if (object->isRunning) {

        object->isRunning = false;

        /* Post the Semaphore when called from the Hwi */
        if (object->mode == Timer_ONESHOT_BLOCKING) {
            SemaphoreP_post(object->timerSem);
        }

        TimerDisable(hwAttrs->baseAddress, object->timer);
        TimerIntDisable(hwAttrs->baseAddress, interruptMask);
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
    }

    HwiP_restore(key);
}
