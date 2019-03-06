/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
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
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/ClockP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/watchdog/WatchdogCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/wdt.h>

/* Function prototypes */
void WatchdogCC32XX_clear(Watchdog_Handle handle);
void WatchdogCC32XX_close(Watchdog_Handle handle);
int_fast16_t WatchdogCC32XX_control(Watchdog_Handle handle,
    uint_fast16_t cmd, void *arg);
void WatchdogCC32XX_init(Watchdog_Handle handle);
Watchdog_Handle WatchdogCC32XX_open(Watchdog_Handle handle, Watchdog_Params *params);
int_fast16_t WatchdogCC32XX_setReload(Watchdog_Handle handle,
    uint32_t value);
uint32_t WatchdogCC32XX_convertMsToTicks(Watchdog_Handle handle,
    uint32_t milliseconds);

/* Internal functions */
static void WatchdogCC32XX_initHardware(Watchdog_Handle handle);
static int WatchdogCC32XX_postNotifyFxn(unsigned int eventType,
    uintptr_t eventArg, uintptr_t clientArg);

/* Watchdog function table for CC32XX implementation */
const Watchdog_FxnTable WatchdogCC32XX_fxnTable = {
    WatchdogCC32XX_clear,
    WatchdogCC32XX_close,
    WatchdogCC32XX_control,
    WatchdogCC32XX_init,
    WatchdogCC32XX_open,
    WatchdogCC32XX_setReload,
    WatchdogCC32XX_convertMsToTicks
};

/* Maximum allowable setReload value */
#define MAX_RELOAD_VALUE        0xFFFFFFFF

/* Millisecond to second ratio */
#define MS_RATIO                1000

/*
 *  ======== WatchdogCC32XX_initHardware ========
 */
static void WatchdogCC32XX_initHardware(Watchdog_Handle handle)
{
    WatchdogCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    WatchdogCC32XX_Object  const *object  = handle->object;

    MAP_WatchdogUnlock(hwAttrs->baseAddr);
    MAP_WatchdogReloadSet(hwAttrs->baseAddr, object->reloadValue);
    MAP_WatchdogIntClear(hwAttrs->baseAddr);

    /* Set debug stall mode */
    if (object->debugMode == Watchdog_DEBUG_STALL_ON) {
        MAP_WatchdogStallEnable(hwAttrs->baseAddr);
    }
    else {
        MAP_WatchdogStallDisable(hwAttrs->baseAddr);
    }

    MAP_WatchdogEnable(hwAttrs->baseAddr);

    MAP_WatchdogLock(hwAttrs->baseAddr);
}

/*
 *  ======== WatchdogCC32XX_postNotifyFxn ========
 *  This functions is called when a transition from LPDS mode is made.
 *  clientArg is a handle of a previously opened Watchdog instance.
 */
static int WatchdogCC32XX_postNotifyFxn(unsigned int eventType,
    uintptr_t eventArg, uintptr_t clientArg)
{
    WatchdogCC32XX_initHardware((Watchdog_Handle) clientArg);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== WatchdogCC32XX_clear ========
 */
void WatchdogCC32XX_clear(Watchdog_Handle handle)
{
    WatchdogCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    MAP_WatchdogIntClear(hwAttrs->baseAddr);
}

/*
 *  ======== WatchdogCC32XX_close ========
 */
void WatchdogCC32XX_close(Watchdog_Handle handle)
{
    /*
     *  Not supported for CC32XX - Once the INTEN bit of the WDTCTL
     *  register has been set, it can only be cleared by a hardware
     *  reset.
     */
}

/*
 *  ======== WatchdogCC32XX_control ========
 *  @pre    Function assumes that the handle is not NULL
 */
int_fast16_t WatchdogCC32XX_control(Watchdog_Handle handle, uint_fast16_t cmd,
        void *arg)
{
    WatchdogCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    switch (cmd) {
        /* Specific Watchdog CMDs */
        case (WatchdogCC32XX_CMD_IS_TIMER_ENABLE):
            *(bool *)arg = MAP_WatchdogRunning(hwAttrs->baseAddr);
            return (Watchdog_STATUS_SUCCESS);

        case (WatchdogCC32XX_CMD_GET_TIMER_VALUE):
            *(uint32_t *)arg = MAP_WatchdogValueGet(hwAttrs->baseAddr);
            return (Watchdog_STATUS_SUCCESS);

        case (WatchdogCC32XX_CMD_IS_TIMER_LOCKED):
            *(bool *)arg = MAP_WatchdogLockState(hwAttrs->baseAddr);
            return (Watchdog_STATUS_SUCCESS);

        case (WatchdogCC32XX_CMD_GET_TIMER_RELOAD_VALUE):
            *(uint32_t *)arg = MAP_WatchdogReloadGet(hwAttrs->baseAddr);
            return (Watchdog_STATUS_SUCCESS);

        default:
            return (Watchdog_STATUS_UNDEFINEDCMD);
    }
}

/*
 *  ======== WatchdogCC32XX_init ========
 */
void WatchdogCC32XX_init(Watchdog_Handle handle)
{
    WatchdogCC32XX_Object *object = handle->object;

    object->isOpen = false;
}

/*
 *  ======== WatchdogCC32XX_open ========
 */
Watchdog_Handle WatchdogCC32XX_open(Watchdog_Handle handle, Watchdog_Params *params)
{
    uintptr_t                     key;
    HwiP_Handle                   hwiHandle;
    HwiP_Params                   hwiParams;
    WatchdogCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    WatchdogCC32XX_Object        *object  = handle->object;

    key = HwiP_disable();

    if (object->isOpen == true) {
        HwiP_restore(key);
        return (NULL);
    }

    object->isOpen = true;
    HwiP_restore(key);

    /* Register the hardware interrupt for this watchdog */
    if (params->callbackFxn) {
        HwiP_Params_init(&hwiParams);
        hwiParams.arg = (uintptr_t) handle;
        hwiParams.priority = hwAttrs->intPriority;
        hwiHandle = HwiP_create(hwAttrs->intNum, params->callbackFxn,
            &hwiParams);
        if (hwiHandle == NULL) {
            object->isOpen = false;
            return (NULL);
        }
    }

    Power_setDependency(PowerCC32XX_PERIPH_WDT);
    Power_registerNotify(&(object->notifyObj), PowerCC32XX_AWAKE_LPDS,
        WatchdogCC32XX_postNotifyFxn, (uintptr_t) handle);

    object->debugMode = params->debugStallMode;
    object->reloadValue = hwAttrs->reloadValue;

    WatchdogCC32XX_initHardware(handle);

    return (handle);
}

/*
 *  ======== WatchdogCC32XX_setReload ========
 */
int_fast16_t WatchdogCC32XX_setReload(Watchdog_Handle handle, uint32_t value)
{
    WatchdogCC32XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    WatchdogCC32XX_Object        *object  = handle->object;

    /* Set value */
    MAP_WatchdogUnlock(hwAttrs->baseAddr);
    MAP_WatchdogReloadSet(hwAttrs->baseAddr, value);
    MAP_WatchdogLock(hwAttrs->baseAddr);
    object->reloadValue = value;

    return (Watchdog_STATUS_SUCCESS);
}

/*
 *  ======== WatchdogCC32XX_convertMsToTicks ========
 *  This function converts the input value from milliseconds to
 *  Watchdog clock ticks.
 */
uint32_t WatchdogCC32XX_convertMsToTicks(Watchdog_Handle handle,
    uint32_t milliseconds)
{
    uint32_t        tickValue;
    uint32_t        convertRatio;
    uint32_t        maxConvertMs;
    ClockP_FreqHz   freq;

    /* Determine milliseconds to clock ticks conversion ratio */
    ClockP_getCpuFreq(&freq);

    /* Watchdog clock ticks/ms = CPU clock / MS_RATIO */
    convertRatio = freq.lo / MS_RATIO;
    maxConvertMs = MAX_RELOAD_VALUE / convertRatio;

    /* Convert milliseconds to watchdog timer ticks */
    /* Check if value exceeds maximum */
    if (milliseconds > maxConvertMs) {
        tickValue = 0;  /* Return zero to indicate overflow */
    }
    else {
        tickValue = (uint32_t)(milliseconds * convertRatio);
    }

    return(tickValue);
}
